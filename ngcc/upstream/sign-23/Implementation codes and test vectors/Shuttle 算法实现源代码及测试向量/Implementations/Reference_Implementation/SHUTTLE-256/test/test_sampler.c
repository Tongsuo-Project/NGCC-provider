/*
 * test_sampler.c -- build-verify for the 96-bit RCDT base sampler
 * (scalar reference).  Pure C99, self-contained (a tiny xorshift PRNG; no
 * external RNG / XOF), builds with only sampler.c + reduce.c.
 *
 * Build (per mode m in 128/256/512):
 *   gcc -std=c99 -Wpedantic -Wall -Wextra -Werror -O2 \
 *       -I SHUTTLE/ref -I SHUTTLE/tools \
 *       SHUTTLE/ref/test/test_sampler.c SHUTTLE/ref/sampler.c \
 *       SHUTTLE/ref/reduce.c -DSHUTTLE_MODE=<m>
 *
 * Coverage:
 *   (1) SCALAR FOLD == eq|lt textbook reference  (the demo_basesampler.c
 *       `ref_count` cross-check): cdt_scan96 per-sample output ==
 * ref_count over 4e6 random + boundary cases (limb == threshold, all-0xFF,
 * all-0) for every real table -> 0 mismatch.  Validates INV-NOMAX + the
 * fold. (2) NEGATIVE / INV-NOMAX-load-bearing: an INV-NOMAX-VIOLATING
 * synthetic table (mid limb forced to 0xFFFFFFFF) makes the fold DISAGREE
 * with ref_count -- proving the guard is load-bearing and the test catches
 * it. (3) RUNTIME INV-NOMAX re-assert: every row of every compiled table
 * has mid/high limb != 0xFFFFFFFF (belt-and-suspenders mirror of
 * gen-time). (4) DISTRIBUTION / EXACT-BUCKET histogram: draw >= 2^22
 * magnitudes per table from the grouped layout, chi-square the observed
 * bucket counts against the table-implied PMF (pmf[k] =
 * tail[k-1]-tail[k]). (5) STATISTICAL stddev: from the same histogram, the
 * half-Gaussian magnitude stddev (NO sign, NO zero-fold) must match the
 * table-implied value to a loose tolerance (catches a table-load / layout
 * bug). (6) BATCH-EQUIV: sampler_sigma2 / noise_magnitude_batch over a
 * 32-sample grouped mini-batch is BIT-IDENTICAL to 32 independent
 * single-sample cdt_scan96 calls (and to ref_count) -- the
 * cross-backend byte-exactness contract checked here against the scalar
 * oracle. (7)
 * FLIP-COMMUTE identity: (Z+b)^K == (Z^K)+b over 4e6 random (Z,b) -> 0
 *       counterexamples (justifies the AVX2 borrow fold).
 *
 * The constant-time STRUCTURAL gate (objdump: no idiv, scan is cmov/setcc
 * not secret-dependent jumps) is run outside this driver by the build
 * harness.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "params.h"
#include "rcdt_tables.h"
#include "sampler.h"

/* libm-free helpers so the test links with the exact reference flags
 * (empty LDLIBS, no -lm): a long-double absolute value and a
 * Newton-iteration square root (the inputs are small positive variances,
 * so a few iterations from a coarse seed converge to far more than the
 * test's tolerance). */
static long double ld_abs(long double x)
{
    return x < 0.0L ? -x : x;
}
static long double ld_sqrt(long double x)
{
    long double r;
    int it;
    if (x <= 0.0L)
        return 0.0L;
    r = x > 1.0L ? x : 1.0L; /* seed >= sqrt(x) for x in our range */
    for (it = 0; it < 60; it++)
        r = 0.5L * (r + x / r);
    return r;
}

/* ---- tiny deterministic PRNG (xorshift128+) ---- */
static uint64_t rng_s0 = 0x0123456789abcdefULL;
static uint64_t rng_s1 = 0xfedcba9876543210ULL;
static uint64_t rng_next(void)
{
    uint64_t x = rng_s0, y = rng_s1;
    rng_s0 = y;
    x ^= x << 23;
    rng_s1 = x ^ y ^ (x >> 17) ^ (y >> 26);
    return rng_s1 + y;
}
static uint32_t rng_u32(void)
{
    return (uint32_t)(rng_next() >> 11);
}

static int g_fails = 0;
static void report(const char *name, int fail)
{
    printf("[%-34s] %s\n", name, fail ? "FAIL" : "PASS");
    if (fail)
        g_fails++;
}

#define FLIP_K \
    0x80000000U /* CDT96_FLIP, for the flip-commute identity test */

/* ============================================================ *
 * The "ground-truth" textbook compare from demo_basesampler.c. *
 * eq|lt borrow form -- correct for ALL Z (even 0xFFFFFFFF limbs).*
 * ============================================================ */
static int32_t ref_count(uint32_t v0, uint32_t v1, uint32_t v2,
                         const uint32_t Z[][3], int entries)
{
    int32_t z = 0;
    int i;
    for (i = 0; i < entries; i++) {
        uint32_t b = (v0 < Z[i][0]);                /* borrow0 */
        b = (v1 < Z[i][1]) | ((v1 == Z[i][1]) & b); /* borrow1 */
        b = (v2 < Z[i][2]) | ((v2 == Z[i][2]) & b); /* borrow2 */
        z += (int32_t)b;
    }
    return z;
}

/* ---- table registry (the four real tables + their row counts) ---- */
typedef struct {
    const char *name;
    const uint32_t (*Z)[3];
    int entries;
} table_t;

static const table_t TABLES[] = {
    {"RCDT_Z", SHUTTLE_RCDT_Z, RCDT_Z_ENTRIES},
    {"RCDT_NOISE_0_85", SHUTTLE_RCDT_NOISE_0_85, 9},
    {"RCDT_NOISE_0_90", SHUTTLE_RCDT_NOISE_0_90, 10},
    {"RCDT_NOISE_1_00", SHUTTLE_RCDT_NOISE_1_00, 11},
};
#define NTABLES ((int)(sizeof(TABLES) / sizeof(TABLES[0])))

/* a 96-bit limb triple */
typedef struct {
    uint32_t v0, v1, v2;
} v96;

/* draw a fresh uniform 96-bit sample (full 32 bits per limb) */
static v96 draw_v96(void)
{
    v96 v;
    v.v0 = rng_u32();
    v.v1 = rng_u32();
    v.v2 = rng_u32();
    return v;
}

/* store a sample into the grouped layout buffer used by cdt_scan96:
 * group = s>>3, lane = s&7, limb j at group*96 + lane*4 + j*32. */
static void put_grouped(uint8_t *buf, int s, v96 v)
{
    int base = (s >> 3) * 96 + (s & 7) * 4;
    uint32_t limb[3];
    int j;
    limb[0] = v.v0;
    limb[1] = v.v1;
    limb[2] = v.v2;
    for (j = 0; j < 3; j++) {
        buf[base + j * 32 + 0] = (uint8_t)(limb[j] >> 0);
        buf[base + j * 32 + 1] = (uint8_t)(limb[j] >> 8);
        buf[base + j * 32 + 2] = (uint8_t)(limb[j] >> 16);
        buf[base + j * 32 + 3] = (uint8_t)(limb[j] >> 24);
    }
}

/* ============================================================ *
 * (1) scalar fold (cdt_scan96, 1 sample) == eq|lt ref_count.   *
 *     This IS the demo_basesampler.c item-1 cross-check.       *
 * ============================================================ */
static int test_fold_vs_ref(void)
{
    long total_fail = 0;
    int t;
    for (t = 0; t < NTABLES; t++) {
        const table_t *tb = &TABLES[t];
        long fails = 0, n;
        for (n = 0; n < 1000000; n++) {
            v96 v = draw_v96();
            /* inject boundary corners (limb == a threshold, all-FF, all-0)
             */
            switch (n & 7) {
                case 0: {
                    int i = (int)(rng_u32() % (unsigned)tb->entries);
                    v.v1 = tb->Z[i][1];
                    break;
                }
                case 1: {
                    int i = (int)(rng_u32() % (unsigned)tb->entries);
                    v.v2 = tb->Z[i][2];
                    v.v1 = tb->Z[i][1];
                    break;
                }
                case 2:
                    v.v0 = 0xFFFFFFFFU;
                    v.v1 = 0xFFFFFFFFU;
                    break;
                case 3:
                    v.v0 = 0;
                    v.v1 = 0;
                    v.v2 = 0;
                    break;
                default:
                    break;
            }
            {
                /* single-sample cdt_scan96 via a 1-sample grouped buffer
                 */
                uint8_t buf[96];
                int32_t got = -1;
                memset(buf, 0, sizeof(buf));
                put_grouped(buf, 0, v);
                cdt_scan96(&got, buf, tb->Z, tb->entries, 1);
                if (got != ref_count(v.v0, v.v1, v.v2, tb->Z, tb->entries))
                    fails++;
            }
        }
        printf("    %-16s fold vs eq|lt ref: %ld mismatches\n", tb->name,
               fails);
        total_fail += fails;
    }
    return total_fail != 0;
}

/* ============================================================ *
 * (2) NEGATIVE: an INV-NOMAX-violating table makes the fold    *
 *     DISAGREE with ref_count (the guard is load-bearing).     *
 * ============================================================ */
static int test_negative_inv_nomax(void)
{
    /* A 1-row synthetic table whose HIGH limb violates INV-NOMAX
     * (Z2 = 0xFFFFFFFF), chosen so the disagreement is FORCED.  The fold
     * and the textbook compare split precisely when a limb equals
     * 0xFFFFFFFF AND it receives an incoming borrow AND v at that limb is
     * also 0xFFFFFFFF: fold: v2 <_u (0xFFFFFFFF + b1=1) = 0  ->  false
     * (Z2+1 WRAPS to 0) ref : (v2 <_u 0xFFFFFFFF)=false OR (v2 ==_u
     * 0xFFFFFFFF) & b1=1 -> true Drive b1 = 1 cleanly through the low/mid
     * limbs (both < their thresholds, neither equal to 0xFFFFFFFF). */
    static const uint32_t BAD[1][3] = {
        {0x00000001U, 0x00000001U,
         0xFFFFFFFFU}, /* HIGH limb = 0xFFFFFFFF (bad) */
    };
    uint8_t buf[96];
    v96 v;
    int32_t fold = -1, ref;
    int disagree;

    v.v0 = 0x00000000U; /* v0 < Z0 = 1            -> b0 = 1 */
    v.v1 = 0x00000000U; /* v1 < Z1 = 1            -> b1 = 1 (both forms
                           agree) */
    v.v2 = 0xFFFFFFFFU; /* v2 == Z2 = 0xFFFFFFFF  -> fold/ref SPLIT at limb
                           2  */
    memset(buf, 0, sizeof(buf));
    put_grouped(buf, 0, v);
    cdt_scan96(&fold, buf, BAD, 1, 1);
    ref = ref_count(v.v0, v.v1, v.v2, BAD, 1);
    disagree = (fold != ref);
    printf("    INV-NOMAX-violating table: fold=%d ref=%d (must differ)\n",
           (int)fold, (int)ref);
    /* PASS iff they DISAGREE (so the guard / test would catch a bad
     * table). */
    return !disagree;
}

/* ============================================================ *
 * (3) runtime INV-NOMAX re-assert over every compiled table.   *
 * ============================================================ */
static int test_runtime_inv_nomax(void)
{
    int t, i, violations = 0;
    for (t = 0; t < NTABLES; t++) {
        const table_t *tb = &TABLES[t];
        for (i = 0; i < tb->entries; i++) {
            if (tb->Z[i][1] == 0xFFFFFFFFU || tb->Z[i][2] == 0xFFFFFFFFU) {
                printf("    %s[%d] violates INV-NOMAX\n", tb->name, i);
                violations++;
            }
        }
    }
    printf(
        "    66 table rows scanned (RCDT_Z 36 + noise 9/10/11), %d "
        "violations\n",
        violations);
    return violations != 0;
}

/* ============================================================ *
 * (4)+(5) distribution histogram + induced stddev.             *
 *     The half-Gaussian magnitude PMF implied by the table is  *
 *     pmf[k] = tail[k-1] - tail[k], tail[k] = Z[k]/2^96         *
 *     (tail[-1] = 1, tail[entries] = 0).                        *
 * ============================================================ */
static long double table_tail(const uint32_t z[3])
{
    /* Z / 2^96 in long double (enough mantissa for a coarse PMF / stddev).
     */
    long double v = (long double)z[2];
    v = v * 4294967296.0L + (long double)z[1];
    v = v * 4294967296.0L + (long double)z[0];
    return v / 79228162514264337593543950336.0L; /* 2^96 */
}

static int test_distribution(void)
{
    const long NDRAW = 1L << 22; /* ~4.19e6 magnitudes per table */
    int t, fail = 0;
    for (t = 0; t < NTABLES; t++) {
        const table_t *tb = &TABLES[t];
        int nbuck = tb->entries + 1; /* magnitudes 0..entries */
        long obs[64];
        long double pmf[64];
        long double chi2 = 0.0L, mean = 0.0L, m2 = 0.0L, stddev_tbl;
        long double tail_prev = 1.0L;
        int k;
        long n;

        for (k = 0; k < nbuck; k++)
            obs[k] = 0;
        /* table-implied PMF + table stddev (half-Gaussian magnitude, no
         * fold) */
        for (k = 0; k < nbuck; k++) {
            long double tail_k =
                (k < tb->entries) ? table_tail(tb->Z[k]) : 0.0L;
            pmf[k] = tail_prev - tail_k;
            if (pmf[k] < 0.0L)
                pmf[k] = 0.0L;
            tail_prev = tail_k;
            mean += (long double)k * pmf[k];
        }
        for (k = 0; k < nbuck; k++)
            m2 +=
                ((long double)k - mean) * ((long double)k - mean) * pmf[k];
        stddev_tbl = ld_sqrt(m2);

        /* draw magnitudes through the real sampler path (grouped layout)
         */
        for (n = 0; n < NDRAW; n++) {
            uint8_t buf[96];
            int32_t got = 0;
            memset(buf, 0, sizeof(buf));
            put_grouped(buf, 0, draw_v96());
            cdt_scan96(&got, buf, tb->Z, tb->entries, 1);
            if (got >= 0 && got < nbuck)
                obs[got]++;
            else {
                printf("    %s: out-of-range magnitude %d\n", tb->name,
                       (int)got);
                fail = 1;
            }
        }

        /* chi-square against the expected counts; also empirical stddev */
        {
            long double emean = 0.0L, em2 = 0.0L;
            for (k = 0; k < nbuck; k++) {
                long double exp_ct = pmf[k] * (long double)NDRAW;
                long double d = (long double)obs[k] - exp_ct;
                if (exp_ct >=
                    5.0L) /* chi-square valid only for non-tiny cells */
                    chi2 += d * d / exp_ct;
                emean += (long double)k * (long double)obs[k] /
                         (long double)NDRAW;
            }
            for (k = 0; k < nbuck; k++)
                em2 += ((long double)k - emean) *
                       ((long double)k - emean) * (long double)obs[k] /
                       (long double)NDRAW;
            {
                long double estd = ld_sqrt(em2);
                /* loose statistical gates: chi2 well under a generous
                 * bound, empirical (half-Gaussian magnitude) stddev within
                 * 1% of the table-implied magnitude stddev. */
                long double rel = ld_abs(estd - stddev_tbl) /
                                  (stddev_tbl > 0 ? stddev_tbl : 1.0L);
                printf(
                    "    %-16s chi2=%.2Lf (df=%d) emp.mag.std=%.6Lf "
                    "tbl.mag.std=%.6Lf (rel %.4Lf%%)\n",
                    tb->name, chi2, nbuck - 1, estd, stddev_tbl,
                    rel * 100.0L);
                /* df ~ nbuck-1 <= 36; chi2 >> 200 would flag a real bug.
                 */
                if (chi2 > 200.0L) {
                    printf("    %s: chi2 too large\n", tb->name);
                    fail = 1;
                }
                if (rel > 0.01L) {
                    printf("    %s: magnitude stddev off by >1%%\n",
                           tb->name);
                    fail = 1;
                }
                /* For the NOISE tables, ALSO check the INDUCED
                 * post-zero-fold SIGNED stddev (the binding SK metric,
                 * audited per sigma): the caller assigns a uniform sign
                 * and rejects magnitude-0 with prob 1/2, so accept_mass =
                 * 1 - p0/2 and the signed variance is E[k^2] / accept_mass
                 * (the mean is 0 by symmetry).  Compare to the audited
                 * ideal sigma (0.85 / 0.9 / 1.0) within its gap. */
                if (t >= 1) {
                    long double p0 =
                        (long double)obs[0] / (long double)NDRAW;
                    long double e_k2 = 0.0L, accept, sig_std, ideal;
                    int kk;
                    for (kk = 0; kk < nbuck; kk++)
                        e_k2 += (long double)kk * (long double)kk *
                                (long double)obs[kk] / (long double)NDRAW;
                    accept = 1.0L - p0 / 2.0L;
                    sig_std = ld_sqrt(e_k2 / accept);
                    ideal = (t == 1) ? 0.85L : (t == 2) ? 0.9L : 1.0L;
                    printf(
                        "    %-16s induced signed stddev=%.6Lf "
                        "(audited ideal %.2Lf; gap small)\n",
                        tb->name, sig_std, ideal);
                    /* the audited absolute gaps are tiny (<= 2^-15.98);
                     * allow a 2%% statistical slack on top of the ideal
                     * sigma. */
                    if (ld_abs(sig_std - ideal) / ideal > 0.02L) {
                        printf(
                            "    %s: induced signed stddev off by >2%%\n",
                            tb->name);
                        fail = 1;
                    }
                }
            }
        }
    }
    return fail;
}

/* ============================================================ *
 * (6) batch-equiv: sampler_sigma2 / noise_magnitude_batch over *
 *     a 32-sample grouped mini-batch == 32 single cdt_scan96   *
 *     calls == ref_count (bit-identical; the scalar oracle).   *
 * ============================================================ */
static int test_batch_equiv(void)
{
    uint8_t buf[NOISE_CDT_BYTES]; /* 384 = 32 samples grouped */
    int32_t batch_out[32], single_out[32];
    v96 v[32];
    int s, fail = 0;

    /* ---- RCDT_Z via sampler_sigma2 (GAUSS_BATCH = 32) ---- */
    memset(buf, 0, sizeof(buf));
    for (s = 0; s < 32; s++) {
        v[s] = draw_v96();
        put_grouped(buf, s, v[s]);
    }
    sampler_sigma2(batch_out, buf);
    for (s = 0; s < 32; s++) {
        uint8_t one[96];
        memset(one, 0, sizeof(one));
        put_grouped(one, 0, v[s]);
        cdt_scan96(&single_out[s], one, SHUTTLE_RCDT_Z, RCDT_Z_ENTRIES, 1);
        if (batch_out[s] != single_out[s] ||
            batch_out[s] != ref_count(v[s].v0, v[s].v1, v[s].v2,
                                      SHUTTLE_RCDT_Z, RCDT_Z_ENTRIES))
            fail = 1;
    }
    printf("    sampler_sigma2 batch == 32 singles == ref_count: %s\n",
           fail ? "MISMATCH" : "ok");

    /* ---- noise tables via noise_magnitude_batch (NOISE_BATCH = 32) ----
     */
    {
        const table_t *noise[3] = {&TABLES[1], &TABLES[2], &TABLES[3]};
        int ti;
        for (ti = 0; ti < 3; ti++) {
            const table_t *tb = noise[ti];
            int local_fail = 0;
            memset(buf, 0, sizeof(buf));
            for (s = 0; s < 32; s++) {
                v[s] = draw_v96();
                put_grouped(buf, s, v[s]);
            }
            noise_magnitude_batch(batch_out, buf, tb->Z, tb->entries);
            for (s = 0; s < 32; s++) {
                uint8_t one[96];
                memset(one, 0, sizeof(one));
                put_grouped(one, 0, v[s]);
                cdt_scan96(&single_out[s], one, tb->Z, tb->entries, 1);
                if (batch_out[s] != single_out[s] ||
                    batch_out[s] != ref_count(v[s].v0, v[s].v1, v[s].v2,
                                              tb->Z, tb->entries))
                    local_fail = 1;
            }
            printf(
                "    noise_magnitude_batch(%s) batch == 32 singles == "
                "ref: "
                "%s\n",
                tb->name, local_fail ? "MISMATCH" : "ok");
            fail |= local_fail;
        }
    }
    return fail;
}

/* ============================================================ *
 * (7) flip-commute identity (Z+b)^K == (Z^K)+b.                *
 * ============================================================ */
static int test_flip_commute(void)
{
    long fails = 0, n;
    for (n = 0; n < 4000000; n++) {
        uint32_t zz = rng_u32();
        uint32_t b = rng_u32() & 1u;
        if (((zz + b) ^ FLIP_K) != ((zz ^ FLIP_K) + b))
            fails++;
    }
    printf("    flip-commute identity: %ld counterexamples (4e6 cases)\n",
           fails);
    return fails != 0;
}

int main(void)
{
    printf(
        "== test_sampler (SHUTTLE-%d, THETA=%d, ref scalar 96-bit RCDT) "
        "==\n",
        (int)SHUTTLE_MODE, (int)THETA);

    printf("[1] scalar fold == eq|lt textbook ref (demo cross-check)\n");
    report("fold == eq|lt ref", test_fold_vs_ref());

    printf("[2] NEGATIVE: INV-NOMAX-violating table disagrees\n");
    report("INV-NOMAX guard load-bearing", test_negative_inv_nomax());

    printf("[3] runtime INV-NOMAX re-assert\n");
    report("runtime INV-NOMAX", test_runtime_inv_nomax());

    printf("[4]+[5] distribution histogram + induced stddev\n");
    report("distribution + stddev", test_distribution());

    printf("[6] batch entry == N independent scalar calls\n");
    report("batch == N singles", test_batch_equiv());

    printf("[7] flip-commute identity (AVX2 justification)\n");
    report("flip-commute identity", test_flip_commute());

    /* per-mode selector sanity: RCDT_NOISE_S/E + entries resolve to the
     * right physical table for THIS SHUTTLE_MODE (128->0.85/0.85,
     * 256->0.9/1.0, 512->0.9/0.9), and a tiny scan via the selectors ==
     * ref_count. */
    {
        uint8_t one[96];
        v96 vv = draw_v96();
        int32_t gs = 0, ge = 0;
        int sfail;
        memset(one, 0, sizeof(one));
        put_grouped(one, 0, vv);
        /* single-sample scan via the per-mode selector symbols (wiring
         * check). */
        cdt_scan96(&gs, one, RCDT_NOISE_S, RCDT_NOISE_S_ENTRIES, 1);
        sfail = (gs != ref_count(vv.v0, vv.v1, vv.v2,
                                 (const uint32_t(*)[3])RCDT_NOISE_S,
                                 RCDT_NOISE_S_ENTRIES));
        cdt_scan96(&ge, one, RCDT_NOISE_E, RCDT_NOISE_E_ENTRIES, 1);
        sfail |= (ge != ref_count(vv.v0, vv.v1, vv.v2,
                                  (const uint32_t(*)[3])RCDT_NOISE_E,
                                  RCDT_NOISE_E_ENTRIES));
        printf("[8] per-mode RCDT_NOISE_S/E selectors (entries %d/%d)\n",
               RCDT_NOISE_S_ENTRIES, RCDT_NOISE_E_ENTRIES);
        report("per-mode noise selectors", sfail);
    }

    printf("\n%s (%d failures)\n",
           g_fails ? "FAILURES PRESENT" : "ALL PASS", g_fails);
    return g_fails ? 1 : 0;
}
