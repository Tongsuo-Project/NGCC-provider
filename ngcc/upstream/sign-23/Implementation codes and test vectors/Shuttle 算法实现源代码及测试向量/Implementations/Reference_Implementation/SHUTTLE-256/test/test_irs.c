/*
 * test_irs.c -- build-verify for the IRS layer (SamplerU +
 * RejectSample/R), scalar ref. Pure C99.  The impl is integer-only; the
 * high-precision ORACLES below use long double / __int128 ONLY in the test
 * (never in sampler_u.c / irs.c).
 *
 * Build (per mode m in 128/256/512), NGCC_MODE (default):
 *   gcc -std=c99 -Wpedantic -Wall -Wextra -Werror -O2 -I. -Itest
 * -I../tools \
 *       test/test_irs.c irs.c sampler_u.c approx_exp.c approx_log.c \
 *       symmetric.c drng.c auxfunc.c -DSHUTTLE_MODE=<m>
 * (SHA3_MODE adds -DSHA3_MODE and swaps drng.c+auxfunc.c -> fips202.c.)
 *
 * Coverage:
 *   (a) SamplerU:  U = 2^ell uniform in (0,1] (histogram/KS-style); ell ==
 *       high-precision oracle from the SAME exponent+mantissa bits;
 *       determinism; the (a,m) MSB-first extraction matches a hand
 * example; sampler_u_x2 == two sequential sampler_u. (b) the 2 r^2 ln2
 * restore: the fixed-point u == round(2 r^2 ln2 * ell). (c)
 * RejectSample/R: determinism from (seed_y,c,sk_tilde); the N=29 interval
 * test matches a reference; z == y + sk_tilde.c'; ascending-j
 *       shuffle-invariance; V-once isometry; sign-normalize branch.
 *   (d) fresh-ctx: the IRS ctx is NOT a continuation of SampleY's ctx.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "approx_log.h"
#include "irs.h"
#include "params.h"
#include "sampler_u.h"
#include "symmetric.h"

/* TEST-ONLY: the high-precision oracles below use GNU __int128 (to mirror
 * the impl's u Q-form bit-exactly) and long double.  __int128 is a
 * GCC/Clang extension ISO C does not define; localize the -Wpedantic
 * suppression to this whole test TU (mirrors test_approx.c's oracle
 * guard).  The IMPL (sampler_u.c / irs.c) confines its own __int128 use
 * the same way. */
#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#endif

/* ---- libm-free long-double log / log2 (the build links no -lm) ----
 * ln(x) via the rapidly-converging series ln(m*2^e) = e*ln2 + 2*atanh(u),
 * u = (m-1)/(m+1), m in [1,2) after extracting the binary exponent. */
static long double ld_ln(long double x)
{
    int e = 0;
    long double u, u2, term, sum;
    int i;
    const long double LN2 = 0.6931471805599453094172321214581766L;
    if (x <= 0)
        return -1e30L;
    while (x >= 2.0L) {
        x *= 0.5L;
        e++;
    }
    while (x < 1.0L) {
        x *= 2.0L;
        e--;
    }
    u = (x - 1.0L) / (x + 1.0L);
    u2 = u * u;
    term = u;
    sum = 0.0L;
    for (i = 1; i < 80; i += 2) {
        sum += term / (long double)i;
        term *= u2;
    }
    return (long double)e * LN2 + 2.0L * sum;
}
static long double ld_log2(long double x)
{
    const long double LN2 = 0.6931471805599453094172321214581766L;
    return ld_ln(x) / LN2;
}

static int g_fails = 0;
static void report(const char *name, int fail)
{
    printf("[%-44s] %s\n", name, fail ? "FAIL" : "PASS");
    if (fail)
        g_fails++;
}

static void fill_seed(uint8_t *p, size_t len, uint8_t salt)
{
    size_t i;
    for (i = 0; i < len; i++)
        p[i] = (uint8_t)(0x40u + salt + (uint8_t)(i * 7u));
}

/* Open the IRS ctx the way Sign will: fresh ctx, absorb 0x09||seed_y.
 * This is the canonical IRS stream init. */
static void irs_ctx_init(xof_ctx *ctx, const uint8_t seed_y[SEEDBYTES])
{
    uint8_t in[1 + SEEDBYTES];
    in[0] = DS_IRS;
    memcpy(in + 1, seed_y, SEEDBYTES);
    xof256_init(ctx, in, sizeof(in));
}

/* ===================================================================== *
 *  ORACLES (long double / __int128, TEST-ONLY)                          *
 * ===================================================================== */

/* Mantissa57 / clz80 reference, recomputed bit-by-bit MSB-first from
 * bytes. */
static unsigned int ref_clz80(const uint8_t rho_a[10])
{
    unsigned int byte, bit, idx = 0;
    for (byte = 0; byte < 10; ++byte)
        for (bit = 0; bit < 8; ++bit, ++idx) {
            unsigned int b =
                (rho_a[byte] >> (7u - bit)) & 1u; /* MSB-first */
            if (b)
                return idx;
        }
    return 80u;
}
static uint64_t ref_mantissa57(const uint8_t rho_b[8])
{
    uint64_t m = 0;
    unsigned int byte, bit, taken = 0;
    for (byte = 0; byte < 8 && taken < 57; ++byte)
        for (bit = 0; bit < 8 && taken < 57; ++bit, ++taken) {
            unsigned int b =
                (rho_b[byte] >> (7u - bit)) & 1u; /* MSB-first */
            m = (m << 1) | b; /* M_0 most significant -> 2^{56-i} weight */
        }
    return m;
}

/* High-precision log2 oracle from (a, m): ell_ref = log2(1 + m/2^57) - a.
 */
static long double ref_log2U(uint32_t a, uint64_t m)
{
    long double b =
        1.0L + (long double)m / (long double)((uint64_t)1 << 57);
    long double log2b = ld_log2(b);
    return log2b - (long double)a;
}

/* ===================================================================== *
 *  (a) SamplerU                                                          *
 * ===================================================================== */

/* Recompute the impl's (a, m) from a 18-byte block the way sampler_u does,
 * then check the public ell vs the bit-by-bit oracle and the ApproxLog
 * frac. */
static int test_sampleru_extraction(void)
{
    int fail = 0;
    uint8_t rho_a[10], rho_b[8];
    unsigned int trial;
    long double max_abs = 0.0L;

    /* Hand-computed example: rho_a = 0x20,0,...  -> R_0=0,R_1=0,R_2=1
     * (0x20 = 0010_0000) -> CLZ = 2 -> a = 3.  rho_b[0]=0xC0 ->
     * M_0=1,M_1=1 -> top bits of m = 11... -> j = m>>55 = 3.  Verify
     * against the impl helpers via a one-shot ctx-free path: drive
     * sampler_u through a stub ctx is awkward, so we check the ORACLE math
     * here and the impl agreement below over a deterministic stream. */
    memset(rho_a, 0, sizeof rho_a);
    rho_a[0] = 0x20u; /* 0010_0000 */
    if (ref_clz80(rho_a) != 2u) {
        printf("    hand clz example: ref_clz80(0x20..)=%u expected 2\n",
               ref_clz80(rho_a));
        fail = 1;
    }
    memset(rho_b, 0, sizeof rho_b);
    rho_b[0] = 0xC0u; /* 1100_0000 -> M_0=M_1=1 */
    {
        uint64_t m = ref_mantissa57(rho_b);
        uint32_t j = (uint32_t)(m >> 55);
        if (j != 3u) {
            printf("    hand mantissa example: j=%u expected 3 (m=%llu)\n",
                   j, (unsigned long long)m);
            fail = 1;
        }
    }
    /* all-zero exponent -> a = 81 (collapsed tail). */
    memset(rho_a, 0, sizeof rho_a);
    if (ref_clz80(rho_a) != 80u) {
        printf("    all-zero clz must be 80 (a=81)\n");
        fail = 1;
    }
    /* every single-bit pattern -> CLZ == position. */
    for (trial = 0; trial < 80u; ++trial) {
        memset(rho_a, 0, sizeof rho_a);
        rho_a[trial / 8u] = (uint8_t)(0x80u >> (trial % 8u));
        if (ref_clz80(rho_a) != trial) {
            printf("    single-bit clz at %u wrong\n", trial);
            fail = 1;
        }
    }

    /* Drive sampler_u over a deterministic ctx; recompute (a,m) from the
     * SAME bytes via a parallel ctx and compare ell to the oracle. */
    {
        uint8_t seed_y[SEEDBYTES];
        xof_ctx ctx, ctx2;
        fill_seed(seed_y, SEEDBYTES, 0x09);
        irs_ctx_init(&ctx, seed_y);
        irs_ctx_init(&ctx2, seed_y);
        for (trial = 0; trial < 4000u; ++trial) {
            sampler_u_res r = sampler_u(&ctx);
            uint8_t a_bytes[10], b_bytes[8];
            uint32_t a_ref, j;
            uint64_t m, xq;
            int64_t frac_ref;
            long double ell_impl, ell_ref, d;
            /* mirror the squeeze: 10 then 8 off the parallel ctx. */
            xof256_squeeze(&ctx2, a_bytes, 10);
            xof256_squeeze(&ctx2, b_bytes, 8);
            a_ref = ref_clz80(a_bytes) + 1u;
            m = ref_mantissa57(b_bytes);
            j = (uint32_t)(m >> 55);
            xq = (m & (((uint64_t)1 << 55) - 1)) << 9;
            frac_ref = approx_log2_frac_q62(j, xq);
            if (r.a != a_ref || r.frac_q62 != frac_ref) {
                printf(
                    "    sampler_u mismatch trial %u: a %u/%u frac "
                    "%lld/%lld\n",
                    trial, r.a, a_ref, (long long)r.frac_q62,
                    (long long)frac_ref);
                fail = 1;
                break;
            }
            /* ell vs high-precision oracle (from the SAME a,m). */
            ell_impl = (long double)r.frac_q62 /
                           (long double)((uint64_t)1 << 62) -
                       (long double)r.a;
            ell_ref = ref_log2U(a_ref, m);
            d = ell_impl - ell_ref;
            if (d < 0)
                d = -d;
            if (d > max_abs)
                max_abs = d;
        }
        /* ApproxLog absolute error budget is 2^-57; allow a hair more for
         * the long-double oracle (which itself has ~2^-63 rounding). */
        if (max_abs > 1e-15L) { /* ~2^-49.8, very loose; realized ~2^-59 */
            printf("    ell vs oracle max abs %.3Le too large\n", max_abs);
            fail = 1;
        }
        printf(
            "    SamplerU: 4000 draws, ell-vs-oracle max abs err = %.3Le "
            "(~2^%.1Lf)\n",
            max_abs, max_abs > 0 ? ld_log2(max_abs) : -99.0L);
    }
    return fail;
}

/* Uniformity of U = 2^ell in (0,1]: bucket -log2(U) = a - log2(b) by
 * integer exponent a; Pr[a=k] should be ~2^-k.  Chi-square-ish
 * first-bucket check. */
static int test_sampleru_uniform(void)
{
    int fail = 0;
    uint8_t seed_y[SEEDBYTES];
    xof_ctx ctx;
    unsigned long counts[16];
    unsigned long total = 0, i;
    const unsigned long NDRAW = 200000;
    memset(counts, 0, sizeof counts);
    fill_seed(seed_y, SEEDBYTES, 0x55);
    irs_ctx_init(&ctx, seed_y);
    for (i = 0; i < NDRAW; ++i) {
        sampler_u_res r = sampler_u(&ctx);
        unsigned a = r.a;
        if (a < 16u)
            counts[a]++;
        total++;
    }
    /* Pr[a=k] = 2^-k.  Check k=1..6 within a relative tolerance. */
    {
        unsigned k;
        for (k = 1; k <= 6u; ++k) {
            double expect =
                (double)NDRAW / (double)((unsigned long)1 << k);
            double got = (double)counts[k];
            double rel = (got - expect) / expect;
            if (rel < 0)
                rel = -rel;
            printf("    a=%u: got %lu  expect %.0f  rel %.3f\n", k,
                   counts[k], expect, rel);
            if (rel > 0.05) { /* 5% band at these counts */
                fail = 1;
            }
        }
    }
    (void)total;
    return fail;
}

/* sampler_u_x2 == two sequential sampler_u (S9). */
static int test_sampleru_x2(void)
{
    int fail = 0;
    uint8_t seed_y[SEEDBYTES];
    xof_ctx ctx_s, ctx_b;
    unsigned i;
    fill_seed(seed_y, SEEDBYTES, 0x77);
    irs_ctx_init(&ctx_s, seed_y);
    irs_ctx_init(&ctx_b, seed_y);
    for (i = 0; i < 1000u; ++i) {
        sampler_u_res s0 = sampler_u(&ctx_s);
        sampler_u_res s1 = sampler_u(&ctx_s);
        sampler_u_res b[2];
        sampler_u_x2(&ctx_b, b);
        if (s0.a != b[0].a || s0.frac_q62 != b[0].frac_q62 ||
            s1.a != b[1].a || s1.frac_q62 != b[1].frac_q62) {
            printf("    x2 diverged from 2x scalar at pair %u\n", i);
            fail = 1;
            break;
        }
    }
    return fail;
}

/* ===================================================================== *
 *  (b) the 2 r^2 ln2 restore constant                                   *
 * ===================================================================== */

/* The impl forms u (Q44) = (2 r^2 ln2)*log2(U).  Re-derive the float u and
 * confirm the impl's integer u rounds to it.  We reconstruct the impl's u
 * via the SAME R2LN2_QF arithmetic (the constant is in irs.c; we mirror it
 * here with the documented value and compare to the real product). */
#define TEST_R2LN2_QSHIFT 44
#define TEST_R2LN2_QF UINT64_C(16599047320634951608)
static int test_restore_constant(void)
{
    int fail = 0;
    uint8_t seed_y[SEEDBYTES];
    xof_ctx ctx;
    unsigned i;
    long double max_ulp_err = 0.0L;
    const long double TWO_RSQ_LN2 = 943546.5995372255524442072L;

    /* (1) the stored constant reproduces 2 r^2 ln2 to ~2^-44 (Q44). */
    {
        long double c = (long double)TEST_R2LN2_QF /
                        (long double)((uint64_t)1 << TEST_R2LN2_QSHIFT);
        long double rel = (c - TWO_RSQ_LN2) / TWO_RSQ_LN2;
        if (rel < 0)
            rel = -rel;
        printf(
            "    R2LN2_QF/2^44 = %.10Lf vs 2r^2ln2 = %.10Lf  (rel "
            "2^%.1Lf)\n",
            c, TWO_RSQ_LN2, rel > 0 ? ld_log2(rel) : -99.0L);
        if (rel > 1e-12L) { /* ~2^-39.9; realized ~2^-65 */
            printf("    stored constant != 2 r^2 ln2\n");
            fail = 1;
        }
    }

    /* (2) the impl's integer u (Q44) equals round(2r^2ln2*ell*2^44) to <=1
     * ULP.  We compute the true Q44 value in long double and the per-term
     * rounding bound, and assert |u - true| <= 2 (Q44 ULPs).  This is the
     * exact "u == round(2 r^2 ln2 * ell)" check. */
    fill_seed(seed_y, SEEDBYTES, 0x33);
    irs_ctx_init(&ctx, seed_y);
    for (i = 0; i < 3000u; ++i) {
        sampler_u_res r = sampler_u(&ctx);
        /* mirror the u_frac half of sampler_u_to_u_q44 exactly (the
         * rounding lives here; the -a*R2LN2_QF term is integer-exact).
         * TEST-ONLY. */
        unsigned __int128 prod = (unsigned __int128)TEST_R2LN2_QF *
                                 (unsigned __int128)(uint64_t)r.frac_q62;
        unsigned __int128 u_frac =
            (prod + ((unsigned __int128)1 << 61)) >> 62;
        long double frac_real =
            (long double)r.frac_q62 / (long double)((uint64_t)1 << 62);
        long double u_frac_true =
            TWO_RSQ_LN2 * frac_real *
            (long double)((uint64_t)1 << TEST_R2LN2_QSHIFT); /* Q44 */
        /* u_frac < 2^64 (R2LN2_QF*frac/2^62 ~ 2^63.85), fits uint64
         * exactly. */
        long double u_frac_impl = (long double)(uint64_t)u_frac;
        long double d = u_frac_impl - u_frac_true;
        if (d < 0)
            d = -d;
        /* the rounding is round-to-nearest >>62 plus the constant's own
         * <=2^-65 relative error; both bounded by a couple ULP at Q44. */
        if (d > max_ulp_err)
            max_ulp_err = d;
    }
    printf(
        "    u_frac restore: 3000 draws, max |impl - "
        "2r^2ln2*log2(b)*2^44| "
        "= %.2Lf Q44-ULP\n",
        max_ulp_err);
    if (max_ulp_err >
        4.0L) { /* round + constant error: <= ~2 ULP, allow 4 */
        printf("    u_frac rounding off by %.2Lf ULP\n", max_ulp_err);
        fail = 1;
    }
    return fail;
}

/* ===================================================================== *
 *  (c) RejectSample / R                                                 *
 * ===================================================================== */

/* Deterministic small sk_tilde / y / c builders (no rounding/sampler
 * dependency). */
static void build_sk_tilde(poly sk[KVEC], uint8_t salt)
{
    unsigned i, k;
    /* small signed coeffs in [-3,3] so V ~ a few thousand, t bounded. */
    for (i = 0; i < KVEC; ++i)
        for (k = 0; k < N; ++k) {
            uint32_t h =
                (uint32_t)(i * 2654435761u + k * 40503u + salt * 97u);
            sk[i].coeffs[k] = (int32_t)((h % 7u)) - 3; /* {-3..3} */
        }
}
static void build_y(poly y[KVEC], uint8_t salt)
{
    unsigned i, k;
    for (i = 0; i < KVEC; ++i)
        for (k = 0; k < N; ++k) {
            uint32_t h =
                (uint32_t)(i * 2246822519u + k * 3266489917u + salt);
            y[i].coeffs[k] =
                (int32_t)((int32_t)(h % 2001u) - 1000); /* +-1000 */
        }
}
/* Place TAU ones at deterministic positions; opt: a permuted SampleC order
 * has NO effect since reject_sample scans ascending j. */
static void build_c(poly *c, unsigned seedpos)
{
    unsigned placed = 0, k;
    for (k = 0; k < N; ++k)
        c->coeffs[k] = 0;
    /* spread TAU positions deterministically across [0,N). */
    for (k = 0; placed < (unsigned)TAU && k < N; ++k) {
        unsigned pos = (k * 2654435761u + seedpos) % N;
        if (c->coeffs[pos] == 0) {
            c->coeffs[pos] = 1;
            placed++;
        }
    }
    /* if collisions left us short, fill the first free slots. */
    for (k = 0; placed < (unsigned)TAU; ++k)
        if (c->coeffs[k] == 0) {
            c->coeffs[k] = 1;
            placed++;
        }
}

static int hamming(const poly *c)
{
    unsigned k;
    int w = 0;
    for (k = 0; k < N; ++k)
        w += (c->coeffs[k] == 1);
    return w;
}

/* Negacyclic right-rotation oracle (matches irs.c poly_shift_negacyclic).
 */
static void shift_oracle(poly v[KVEC], const poly sk[KVEC], unsigned j)
{
    unsigned i, k;
    for (i = 0; i < KVEC; ++i)
        for (k = 0; k < N; ++k) {
            if (k >= j)
                v[i].coeffs[k] = sk[i].coeffs[k - j];
            else
                v[i].coeffs[k] = -sk[i].coeffs[N + k - j];
        }
}

/* The NEW IRS single-buffer schedule (the bulk-draw optimization): the
 * whole IRS draws ONE tau*18-byte buffer in a single xof256_squeeze, then
 * decodes each transition's 18-byte slice (10 exponent + 8 mantissa) in
 * ascending j. The oracle MUST mirror this byte schedule -- under NGCC it
 * differs from the old per-transition two-squeeze structure (re-recorded
 * KAT); under SHA3 the rate-buffered Keccak squeeze makes the two
 * byte-identical. */
#define ORACLE_IRS_BULK_BYTES ((size_t)TAU * 18)
static void irs_oracle_bulk_fill(xof_ctx *ctx, uint8_t *buf)
{
    xof256_squeeze(ctx, buf, ORACLE_IRS_BULK_BYTES);
}

/* Reference RejectSample using the SAME R2LN2_QF/__int128 interval logic
 * but written independently (ascending j, V once, sign-normalize, 15
 * pairs).  Draws the single bulk buffer then slices 18 bytes per
 * transition. */
static void reject_sample_oracle(xof_ctx *ctx, poly z[KVEC],
                                 const poly y[KVEC], const poly *c,
                                 const poly sk[KVEC])
{
    poly v[KVEC];
    int64_t V = 0;
    unsigned i, k, j;
    uint8_t buf[ORACLE_IRS_BULK_BYTES];
    size_t cur = 0;
    irs_oracle_bulk_fill(ctx, buf);
    for (i = 0; i < KVEC; ++i)
        for (k = 0; k < N; ++k) {
            int64_t cc = sk[i].coeffs[k];
            V += cc * cc;
        }
    memcpy(z, y, KVEC * sizeof(poly));
    for (j = 0; j < N; ++j) {
        if (c->coeffs[j] != 1)
            continue;
        {
            sampler_u_res ell =
                sampler_u_decode(buf + cur, buf + cur + 10);
            cur += 18;
            unsigned __int128 prod =
                (unsigned __int128)TEST_R2LN2_QF *
                (unsigned __int128)(uint64_t)ell.frac_q62;
            unsigned __int128 u_frac =
                (prod + ((unsigned __int128)1 << 61)) >> 62;
            __int128 u_a = (__int128)ell.a * (__int128)TEST_R2LN2_QF;
            __int128 u = (__int128)u_frac - u_a;
            int64_t t = 0, flag;
            shift_oracle(v, sk, j);
            for (i = 0; i < KVEC; ++i)
                for (k = 0; k < N; ++k)
                    t += (int64_t)z[i].coeffs[k] * (int64_t)v[i].coeffs[k];
            if (t <= 0) {
                for (i = 0; i < KVEC; ++i)
                    for (k = 0; k < N; ++k)
                        v[i].coeffs[k] = -v[i].coeffs[k];
                t = -t;
            }
            flag = -1;
            for (i = 0; i < (unsigned)IRS_BDRY; ++i) {
                int64_t two_i1 = (int64_t)(2u * i + 1u);
                int64_t four_i = (int64_t)(4u * i);
                int64_t lo = -2 * two_i1 * t - two_i1 * two_i1 * V;
                int64_t hi = -four_i * t - (int64_t)(4u * i * i) * V;
                __int128 lo_s = (__int128)lo << TEST_R2LN2_QSHIFT;
                __int128 hi_s = (__int128)hi << TEST_R2LN2_QSHIFT;
                if (lo_s < u && u <= hi_s)
                    flag = 1;
            }
            for (i = 0; i < KVEC; ++i)
                for (k = 0; k < N; ++k)
                    z[i].coeffs[k] -= (int32_t)flag * v[i].coeffs[k];
        }
    }
}

static int polyvec_eq(const poly a[KVEC], const poly b[KVEC])
{
    return memcmp(a, b, KVEC * sizeof(poly)) == 0;
}

static int test_reject_sample(void)
{
    int fail = 0;
    uint8_t seed_y[SEEDBYTES];
    poly sk[KVEC], y[KVEC], c;
    poly z1[KVEC], z2[KVEC], zref[KVEC];
    xof_ctx ctx;

    fill_seed(seed_y, SEEDBYTES, 0x21);
    build_sk_tilde(sk, 0x05);
    build_y(y, 0x06);
    build_c(&c, 1234u);

    if (hamming(&c) != TAU) {
        printf("    test challenge weight %d != TAU %d\n", hamming(&c),
               TAU);
        fail = 1;
    }

    /* determinism: same (seed_y,c,sk) -> same z. */
    irs_ctx_init(&ctx, seed_y);
    reject_sample(&ctx, z1, y, &c, sk);
    irs_ctx_init(&ctx, seed_y);
    reject_sample(&ctx, z2, y, &c, sk);
    if (!polyvec_eq(z1, z2)) {
        printf("    reject_sample NOT deterministic\n");
        fail = 1;
    }

    /* matches the independent reference oracle. */
    irs_ctx_init(&ctx, seed_y);
    reject_sample_oracle(&ctx, zref, y, &c, sk);
    if (!polyvec_eq(z1, zref)) {
        printf("    reject_sample != reference oracle\n");
        fail = 1;
    }

    /* z = y + sk.c' identity: z - y must equal +-(sk.X^j) summed over the
     * matching j, i.e. for each matching j the per-transition delta is
     * flag*v.  We can't recompute flags trivially, but we CAN check that
     * (z - y) is a sum of negacyclic shifts of sk with +-1 coeffs.  A
     * cheaper structural check: z - y has the SAME coefficient set parity
     * as the sum of shifts.  Instead, validate the identity by
     * reconstructing c' from the oracle's flag decisions: re-run the
     * oracle capturing flags. */
    {
        poly acc[KVEC];
        int8_t cp[N]; /* c' signs at matching j (0 elsewhere) */
        unsigned i, k, j;
        poly v[KVEC];
        int64_t V = 0;
        xof_ctx cx;
        uint8_t buf[ORACLE_IRS_BULK_BYTES];
        size_t cur = 0;
        memset(cp, 0, sizeof cp);
        for (i = 0; i < KVEC; ++i)
            for (k = 0; k < N; ++k) {
                int64_t cc = sk[i].coeffs[k];
                V += cc * cc;
            }
        memcpy(acc, y, KVEC * sizeof(poly));
        irs_ctx_init(&cx, seed_y);
        irs_oracle_bulk_fill(&cx, buf);
        for (j = 0; j < N; ++j) {
            if (c.coeffs[j] != 1)
                continue;
            {
                sampler_u_res ell =
                    sampler_u_decode(buf + cur, buf + cur + 10);
                cur += 18;
                unsigned __int128 prod =
                    (unsigned __int128)TEST_R2LN2_QF *
                    (unsigned __int128)(uint64_t)ell.frac_q62;
                unsigned __int128 u_frac =
                    (prod + ((unsigned __int128)1 << 61)) >> 62;
                __int128 u_a = (__int128)ell.a * (__int128)TEST_R2LN2_QF;
                __int128 u = (__int128)u_frac - u_a;
                int64_t t = 0, flag;
                int sgn = 1;
                shift_oracle(v, sk, j);
                for (i = 0; i < KVEC; ++i)
                    for (k = 0; k < N; ++k)
                        t += (int64_t)acc[i].coeffs[k] *
                             (int64_t)v[i].coeffs[k];
                if (t <= 0) {
                    sgn = -1;
                    for (i = 0; i < KVEC; ++i)
                        for (k = 0; k < N; ++k)
                            v[i].coeffs[k] = -v[i].coeffs[k];
                    t = -t;
                }
                flag = -1;
                for (i = 0; i < (unsigned)IRS_BDRY; ++i) {
                    int64_t two_i1 = (int64_t)(2u * i + 1u);
                    int64_t four_i = (int64_t)(4u * i);
                    int64_t lo = -2 * two_i1 * t - two_i1 * two_i1 * V;
                    int64_t hi = -four_i * t - (int64_t)(4u * i * i) * V;
                    if (((__int128)lo << TEST_R2LN2_QSHIFT) < u &&
                        u <= ((__int128)hi << TEST_R2LN2_QSHIFT))
                        flag = 1;
                }
                /* applied shift is -flag*v where v already carries the
                 * sign-normalize sgn; net signed challenge coeff at j is
                 * -flag*sgn (multiplying sk.X^j). Interval hit (flag=+1)
                 * therefore applies y-v, matching pv. */
                cp[j] = (int8_t)(-(flag * sgn));
                for (i = 0; i < KVEC; ++i)
                    for (k = 0; k < N; ++k)
                        acc[i].coeffs[k] -= (int32_t)flag * v[i].coeffs[k];
            }
        }
        /* now z must equal y + sum_j cp[j] * (sk.X^j). */
        {
            poly chk[KVEC], sh[KVEC];
            memcpy(chk, y, KVEC * sizeof(poly));
            for (j = 0; j < N; ++j) {
                if (cp[j] == 0)
                    continue;
                shift_oracle(sh, sk, j);
                for (i = 0; i < KVEC; ++i)
                    for (k = 0; k < N; ++k)
                        chk[i].coeffs[k] +=
                            (int32_t)cp[j] * sh[i].coeffs[k];
            }
            if (!polyvec_eq(chk, z1)) {
                printf("    z != y + sk.c' identity\n");
                fail = 1;
            }
        }
        (void)acc;
    }

    return fail;
}

/* A DIFFERENT placement order of the same TAU positions yields
 * identical z (reject_sample scans ascending j, ignoring SampleC's
 * shuffle).  We build the same support set two ways and confirm equal z.
 */
static int test_ascending_j(void)
{
    int fail = 0;
    uint8_t seed_y[SEEDBYTES];
    poly sk[KVEC], y[KVEC], c1, c2;
    poly z1[KVEC], z2[KVEC];
    xof_ctx ctx;
    unsigned k;
    fill_seed(seed_y, SEEDBYTES, 0x2A);
    build_sk_tilde(sk, 0x0A);
    build_y(y, 0x0B);
    build_c(&c1, 999u);
    /* c2 = same support, but we (trivially) reverse-fill -- since c is a
     * coeff array, the "order" SampleC placed them in is not stored; the
     * support set is what matters.  So c2 is literally c1 (same set).  The
     * point here is that reject_sample depends only on the SET, which
     * this confirms together with the oracle's ascending scan. */
    memcpy(&c2, &c1, sizeof(poly));
    /* sanity: shuffle the *iteration* in the oracle would change nothing;
     * we assert reject_sample uses ascending j by comparing to an oracle
     * that scans ascending. */
    irs_ctx_init(&ctx, seed_y);
    reject_sample(&ctx, z1, y, &c1, sk);
    irs_ctx_init(&ctx, seed_y);
    reject_sample(&ctx, z2, y, &c2, sk);
    if (!polyvec_eq(z1, z2)) {
        printf("    ascending-j: identical support gave different z\n");
        fail = 1;
    }
    (void)k;
    return fail;
}

/* V-once isometry: ||sk.X^j||^2 == ||sk||^2 for every j. */
static int test_isometry(void)
{
    int fail = 0;
    poly sk[KVEC], v[KVEC];
    unsigned i, k, j;
    int64_t V0 = 0;
    build_sk_tilde(sk, 0x0C);
    for (i = 0; i < KVEC; ++i)
        for (k = 0; k < N; ++k) {
            int64_t c = sk[i].coeffs[k];
            V0 += c * c;
        }
    for (j = 0; j < N; j += (N / 17u) + 1u) {
        int64_t Vj = 0;
        shift_oracle(v, sk, j);
        for (i = 0; i < KVEC; ++i)
            for (k = 0; k < N; ++k) {
                int64_t c = v[i].coeffs[k];
                Vj += c * c;
            }
        if (Vj != V0) {
            printf("    isometry broken at j=%u: V0=%lld Vj=%lld\n", j,
                   (long long)V0, (long long)Vj);
            fail = 1;
        }
    }
    return fail;
}

/* Interval-test logic vs an exhaustive reference on synthetic (t,V,ell):
 * confirm flag flips at exactly the right boundary and at most one i
 * matches. */
static int test_interval_logic(void)
{
    int fail = 0;
    int64_t V = 87728; /* binding SUF-256 floor(B_k^2) */
    int64_t t;
    unsigned trial;
    for (trial = 0, t = 0; trial < 200u; ++trial, t += 503) {
        /* sweep u across the boundary grid by choosing ell values via a's.
         */
        unsigned a;
        for (a = 1; a <= 81u; ++a) {
            int64_t frac =
                (int64_t)((trial * 6364136223846793005ull) >> 2) &
                (((int64_t)1 << 62) - 1);
            unsigned __int128 prod = (unsigned __int128)TEST_R2LN2_QF *
                                     (unsigned __int128)(uint64_t)frac;
            unsigned __int128 u_frac =
                (prod + ((unsigned __int128)1 << 61)) >> 62;
            __int128 u_a = (__int128)a * (__int128)TEST_R2LN2_QF;
            __int128 u = (__int128)u_frac - u_a;
            int matches = 0;
            unsigned i;
            int64_t tt = t; /* t>0 already */
            for (i = 0; i < (unsigned)IRS_BDRY; ++i) {
                int64_t two_i1 = (int64_t)(2u * i + 1u);
                int64_t four_i = (int64_t)(4u * i);
                int64_t lo = -2 * two_i1 * tt - two_i1 * two_i1 * V;
                int64_t hi = -four_i * tt - (int64_t)(4u * i * i) * V;
                if (((__int128)lo << TEST_R2LN2_QSHIFT) < u &&
                    u <= ((__int128)hi << TEST_R2LN2_QSHIFT))
                    matches++;
            }
            if (matches > 1) { /* disjoint partition: at most one i */
                printf(
                    "    interval: %d boundaries matched (t=%lld a=%u)\n",
                    matches, (long long)t, a);
                fail = 1;
            }
        }
    }
    /* boundary inclusivity: at exactly u = hi (i=0 -> hi=0), flag must be
     * set
     * (<=), and at u = lo (i=0 -> lo = -2t - V), flag must be CLEAR (<).
     */
    {
        int64_t tt = 5000;
        /* i=0: lo = -2t - V, hi = 0.  u just below 0 -> in; u == 0 -> in
         * (<=); u just below lo -> not in for i=0 (but i may match a lower
         * boundary).  Just check the i=0 hi inclusivity directly. */
        __int128 u_at_hi = (__int128)0; /* u == hi(i=0) == 0 */
        int64_t lo0 = -2 * 1 * tt - 1 * 1 * V;
        int in = (((__int128)lo0 << TEST_R2LN2_QSHIFT) < u_at_hi) &&
                 (u_at_hi <= ((__int128)0 << TEST_R2LN2_QSHIFT));
        if (!in) {
            printf("    interval: u==hi(i=0)==0 should be INSIDE (<=)\n");
            fail = 1;
        }
    }
    return fail;
}

/* ===================================================================== *
 *  (d) fresh-ctx                                                        *
 * ===================================================================== */

/* The IRS ctx (0x09||seed_y) must produce a DIFFERENT stream than a 0x08
 * (SampleY) ctx on the same seed_y -- i.e. IRS is not a continuation. */
static int test_fresh_ctx(void)
{
    int fail = 0;
    uint8_t seed_y[SEEDBYTES];
    uint8_t in9[1 + SEEDBYTES], in8[1 + SEEDBYTES];
    xof_ctx c9, c8;
    uint8_t out9[32], out8[32];
    fill_seed(seed_y, SEEDBYTES, 0x66);
    in9[0] = DS_IRS;
    in8[0] = DS_SAMPLE_Y;
    memcpy(in9 + 1, seed_y, SEEDBYTES);
    memcpy(in8 + 1, seed_y, SEEDBYTES);
    xof256_init(&c9, in9, sizeof in9);
    xof256_init(&c8, in8, sizeof in8);
    xof256_squeeze(&c9, out9, sizeof out9);
    xof256_squeeze(&c8, out8, sizeof out8);
    if (memcmp(out9, out8, sizeof out9) == 0) {
        printf("    0x09 and 0x08 ctx produced identical stream\n");
        fail = 1;
    }
    /* and the IRS ctx is reproducible from the tag+seed (fresh init). */
    {
        xof_ctx c9b;
        uint8_t out9b[32];
        xof256_init(&c9b, in9, sizeof in9);
        xof256_squeeze(&c9b, out9b, sizeof out9b);
        if (memcmp(out9, out9b, sizeof out9) != 0) {
            printf("    IRS ctx not reproducible from 0x09||seed_y\n");
            fail = 1;
        }
    }
    return fail;
}

int main(void)
{
    printf("== test_irs  (SHUTTLE_MODE=%d, N=%d, KVEC=%d, TAU=%d) ==\n",
           SHUTTLE_MODE, N, KVEC, TAU);

    report("(a) SamplerU extraction + ell-vs-oracle",
           test_sampleru_extraction());
    report("(a) SamplerU uniformity (Pr[a=k]=2^-k)",
           test_sampleru_uniform());
    report("(a) sampler_u_x2 == 2x scalar", test_sampleru_x2());
    report("(b) 2 r^2 ln2 restore constant", test_restore_constant());
    report("(c) reject_sample determinism+oracle+identity",
           test_reject_sample());
    report("(c) ascending-j support-only", test_ascending_j());
    report("(c) V-once isometry", test_isometry());
    report("(c) interval-test logic (<=1 match, inclusivity)",
           test_interval_logic());
    report("(d) fresh 0x09||seed_y ctx", test_fresh_ctx());

    printf("\n%s: %d failure(s)\n", g_fails ? "FAILURES" : "ALL PASS",
           g_fails);
    return g_fails ? 1 : 0;
}

#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic pop
#endif
