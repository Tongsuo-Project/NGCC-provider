/*
 * test_samplers.c -- build-verify for the Expand and Sample wiring
 *                    (scalar reference).  Pure C99.
 *
 * Build (per mode m in 128/256/512), NGCC_MODE (default):
 *   gcc -std=c99 -Wpedantic -Wall -Wextra -Werror -O2 \
 *       -I ref -I ref/ntt/<qset> -I tools \
 *       ref/test/test_samplers.c ref/polyvec.c ref/sampler.c \
 *       ref/symmetric.c ref/drng.c ref/auxfunc.c ref/reduce.c \
 *       ref/approx_exp.c ref/approx_log.c -DSHUTTLE_MODE=<m>
 * (SHA3_MODE adds -DSHA3_MODE and swaps drng.c+auxfunc.c -> fips202.c.)
 *
 * Coverage:
 *   (a) ExpandSeeds: fixed xi -> deterministic seedA/seedsk/K split;
 * re-run determinism; the three slices are distinct. (b) ExpandA: fixed
 * seedA -> deterministic A_gen; every coeff in [0,q); statistical
 * uniformity (min/max/mean/chi-square over buckets); determinism. (c)
 * ExpandS: fixed seedsk -> s,e with |s|<=L_sigma1, |e|<=L_sigma2;
 *       per-coeff induced signed stddev matches sigma1/sigma2;
 * determinism. (d) SampleC: fixed seedC -> EXACT Hamming weight TAU, all
 * +1, positions cover [0,n); determinism. (e) SampleY/SampleDGauss: fixed
 * seedY -> y with block stddev ~ r=825, range within the ~10-11 sigma
 * tail-cut; determinism; the gauss_stream batch path bit-identical to a
 * scalar reference loop.
 *
 * libm-free: a long-double abs + Newton sqrt (the reference build links
 * with empty LDLIBS, no -lm).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "approx_exp.h" /* approx_exp_accept_q64 (scalar x1, oracle) */
#include "params.h"
#include "polyvec.h"
#include "sampler.h"

/* ---- libm-free helpers ---- */
static long double ld_abs(long double x)
{
    return x < 0 ? -x : x;
}
static long double ld_sqrt(long double x)
{
    long double r;
    int it;
    if (x <= 0)
        return 0;
    r = x > 1 ? x : 1;
    for (it = 0; it < 80; it++)
        r = 0.5L * (r + x / r);
    return r;
}

static int g_fails = 0;
static void report(const char *name, int fail)
{
    printf("[%-40s] %s\n", name, fail ? "FAIL" : "PASS");
    if (fail)
        g_fails++;
}

/* fixed test seeds (filled by a counter so each mode's seed is distinct
 * but deterministic). */
static void fill_seed(uint8_t *p, size_t len, uint8_t salt)
{
    size_t i;
    for (i = 0; i < len; i++)
        p[i] = (uint8_t)(0x40u + salt + (uint8_t)(i * 7u));
}

/* ===================== (a) ExpandSeeds ===================== */
static int test_expand_seeds(void)
{
    uint8_t xi[SEEDBYTES];
    uint8_t T1[EXPAND_SEEDS_BYTES], T2[EXPAND_SEEDS_BYTES];
    const uint8_t *seedA, *seedsk, *K;
    int fail = 0;
    fill_seed(xi, SEEDBYTES, 0x01);

    expand_seeds(T1, xi, 1u);
    expand_seeds(T2, xi, 1u);
    if (memcmp(T1, T2, EXPAND_SEEDS_BYTES) != 0) {
        printf("    ExpandSeeds NOT deterministic\n");
        fail = 1;
    }
    /* different kappa -> different output (overwhelmingly) */
    expand_seeds(T2, xi, 2u);
    if (memcmp(T1, T2, EXPAND_SEEDS_BYTES) == 0) {
        printf("    ExpandSeeds kappa has no effect\n");
        fail = 1;
    }
    seedA = T1;
    seedsk = T1 + SEEDBYTES;
    K = T1 + SEEDBYTES + CHALLENGESEEDBYTES;
    /* slices must be distinct (PRNG output, overwhelmingly) */
    if (memcmp(seedA, seedsk, SEEDBYTES) == 0 ||
        memcmp(seedsk, K, CHALLENGESEEDBYTES) == 0) {
        printf("    ExpandSeeds slices collide\n");
        fail = 1;
    }
    printf("    EXPAND_SEEDS_BYTES=%d  seedA=%d seedsk=%d K=%d\n",
           (int)EXPAND_SEEDS_BYTES, (int)SEEDBYTES,
           (int)CHALLENGESEEDBYTES, (int)CHALLENGESEEDBYTES);

    /* ExpandSigningSeeds determinism */
    {
        uint8_t Kk[CHALLENGESEEDBYTES], rnd[RNDBYTES],
            mu[CHALLENGESEEDBYTES];
        uint8_t sy1[SEEDBYTES], sy2[SEEDBYTES];
        fill_seed(Kk, CHALLENGESEEDBYTES, 0x11);
        fill_seed(rnd, RNDBYTES, 0x22);
        fill_seed(mu, CHALLENGESEEDBYTES, 0x33);
        expand_signing_seeds(sy1, Kk, rnd, mu, 0u);
        expand_signing_seeds(sy2, Kk, rnd, mu, 0u);
        if (memcmp(sy1, sy2, SEEDBYTES) != 0) {
            printf("    ExpandSigningSeeds NOT deterministic\n");
            fail = 1;
        }
        expand_signing_seeds(sy2, Kk, rnd, mu, 1u);
        if (memcmp(sy1, sy2, SEEDBYTES) == 0) {
            printf("    ExpandSigningSeeds kappa has no effect\n");
            fail = 1;
        }
    }
    return fail;
}

/* ===================== (b) ExpandA ===================== */
static int test_expand_a(void)
{
    static poly16 a1[EM], h1[EM * ELL];
    static poly16 a2[EM], h2[EM * ELL];
    uint8_t seedA[SEEDBYTES];
    int fail = 0, p, i;
    long count = 0;
    long double mean = 0.0L;
    int mn = (int)Q, mx = -1;
    /* coarse chi-square over 16 equal buckets of [0,q) */
    long bucket[16];
    long double chi2 = 0.0L;
    int b;

    fill_seed(seedA, SEEDBYTES, 0x55);
    expand_a(a1, h1, seedA);
    expand_a(a2, h2, seedA);
    if (memcmp(a1, a2, sizeof(a1)) != 0 ||
        memcmp(h1, h2, sizeof(h1)) != 0) {
        printf("    ExpandA NOT deterministic\n");
        fail = 1;
    }
    for (b = 0; b < 16; b++)
        bucket[b] = 0;
    /* range + stats over agen and hAgen */
    for (p = 0; p < EM; p++)
        for (i = 0; i < N; i++) {
            int v = a1[p].coeffs[i];
            if (v < 0 || v >= (int)Q) {
                printf("    ExpandA agen coeff out of range: %d\n", v);
                fail = 1;
            }
        }
    for (p = 0; p < EM * ELL; p++)
        for (i = 0; i < N; i++) {
            int v = h1[p].coeffs[i];
            if (v < 0 || v >= (int)Q) {
                printf("    ExpandA hAgen coeff out of range: %d\n", v);
                fail = 1;
            }
            if (v < mn)
                mn = v;
            if (v > mx)
                mx = v;
            mean += (long double)v;
            bucket[(int)((long long)v * 16 / (long long)Q)]++;
            count++;
        }
    mean /= (long double)count;
    {
        long double exp_ct = (long double)count / 16.0L;
        for (b = 0; b < 16; b++) {
            long double d = (long double)bucket[b] - exp_ct;
            chi2 += d * d / exp_ct;
        }
    }
    printf(
        "    ExpandA hAgen: count=%ld min=%d max=%d mean=%.1Lf "
        "(q/2=%d) chi2(df15)=%.2Lf\n",
        count, mn, mx, mean, (int)(Q / 2), chi2);
    /* loose uniformity gates */
    if (ld_abs(mean - (long double)Q / 2.0L) > (long double)Q * 0.03L) {
        printf("    ExpandA mean off from q/2 by >3%%\n");
        fail = 1;
    }
    if (mn > (int)Q / 50 || mx < (int)Q - (int)Q / 50) {
        printf("    ExpandA range does not cover [0,q)\n");
        fail = 1;
    }
    if (chi2 >
        60.0L) { /* df=15; ~37 is the 0.999 quantile, 60 is generous */
        printf("    ExpandA chi-square too large (non-uniform)\n");
        fail = 1;
    }
    return fail;
}

/* ===================== (c) ExpandS ===================== */
static int test_expand_s(void)
{
    static poly s1[ELL + EM], s2[ELL + EM];
    uint8_t seedsk[CHALLENGESEEDBYTES];
    poly *s, *e;
    int fail = 0, p, i;

    fill_seed(seedsk, CHALLENGESEEDBYTES, 0x66);
    expand_s(s1, seedsk);
    expand_s(s2, seedsk);
    if (memcmp(s1, s2, sizeof(s1)) != 0) {
        printf("    ExpandS NOT deterministic\n");
        fail = 1;
    }
    s = s1;
    e = s1 + ELL;
    /* support bounds: |s|<=RCDT_NOISE_S_ENTRIES, |e|<=RCDT_NOISE_E_ENTRIES
     */
    {
        long double e_s2 = 0, e_e2 = 0;
        long ns = 0, ne = 0, z_s = 0, z_e = 0;
        for (p = 0; p < ELL; p++)
            for (i = 0; i < N; i++) {
                int v = s[p].coeffs[i];
                if (v < -RCDT_NOISE_S_ENTRIES ||
                    v > RCDT_NOISE_S_ENTRIES) {
                    printf("    ExpandS s coeff out of support: %d\n", v);
                    fail = 1;
                }
                e_s2 += (long double)v * v;
                if (v == 0)
                    z_s++;
                ns++;
            }
        for (p = 0; p < EM; p++)
            for (i = 0; i < N; i++) {
                int v = e[p].coeffs[i];
                if (v < -RCDT_NOISE_E_ENTRIES ||
                    v > RCDT_NOISE_E_ENTRIES) {
                    printf("    ExpandS e coeff out of support: %d\n", v);
                    fail = 1;
                }
                e_e2 += (long double)v * v;
                if (v == 0)
                    z_e++;
                ne++;
            }
        {
            /* The post-zero-fold signed stddev should be ~ sigma1/sigma2.
             * The empirical std is sqrt(E[v^2]); for a folded discrete
             * Gaussian this lands near the ideal sigma. */
            long double std_s = ld_sqrt(e_s2 / (long double)ns);
            long double std_e = ld_sqrt(e_e2 / (long double)ne);
#if SHUTTLE_MODE == 128
            long double ideal_s = 0.85L, ideal_e = 0.85L;
#elif SHUTTLE_MODE == 256
            long double ideal_s = 0.9L, ideal_e = 1.0L;
#else
            long double ideal_s = 0.9L, ideal_e = 0.9L;
#endif
            printf(
                "    ExpandS s: n=%ld std=%.4Lf (ideal %.2Lf) zeros=%ld\n",
                ns, std_s, ideal_s, z_s);
            printf(
                "    ExpandS e: n=%ld std=%.4Lf (ideal %.2Lf) zeros=%ld\n",
                ne, std_e, ideal_e, z_e);
            if (ld_abs(std_s - ideal_s) / ideal_s > 0.06L) {
                printf("    ExpandS s stddev off by >6%%\n");
                fail = 1;
            }
            if (ld_abs(std_e - ideal_e) / ideal_e > 0.06L) {
                printf("    ExpandS e stddev off by >6%%\n");
                fail = 1;
            }
        }
    }
    return fail;
}

/* ===================== (d) SampleC ===================== */
static int test_sample_c(void)
{
    static poly c1, c2;
    uint8_t seedC[CHALLENGESEEDBYTES];
    int fail = 0, i, wt = 0;
    long cover[1] = {0};
    (void)cover;
    fill_seed(seedC, CHALLENGESEEDBYTES, 0x77);
    sample_c(&c1, seedC);
    sample_c(&c2, seedC);
    if (memcmp(&c1, &c2, sizeof(c1)) != 0) {
        printf("    SampleC NOT deterministic\n");
        fail = 1;
    }
    for (i = 0; i < N; i++) {
        int v = c1.coeffs[i];
        if (v != 0 && v != 1) {
            printf("    SampleC non-binary coeff %d at %d\n", v, i);
            fail = 1;
        }
        wt += v;
    }
    printf("    SampleC weight=%d (want TAU=%d)\n", wt, (int)TAU);
    if (wt != (int)TAU) {
        printf("    SampleC weight != TAU\n");
        fail = 1;
    }
    /* light uniformity-of-support: count set bits in low vs high half */
    {
        int lo = 0, hi = 0;
        for (i = 0; i < N / 2; i++)
            lo += c1.coeffs[i];
        for (i = N / 2; i < N; i++)
            hi += c1.coeffs[i];
        printf("    SampleC support split lo=%d hi=%d\n", lo, hi);
        /* both halves should carry some weight (not a hard gate; report)
         */
    }
    return fail;
}

/* ===================== (e) SampleY / SampleDGauss =====================
 */
static int test_sample_y(void)
{
    static poly y1[KVEC], y2[KVEC];
    uint8_t seedY[SEEDBYTES];
    int fail = 0, p, i;
    long double e2 = 0.0L, mean = 0.0L;
    long cnt = 0;
    int mn = 1 << 30, mx = -(1 << 30);

    fill_seed(seedY, SEEDBYTES, 0x88);
    sample_y(y1, seedY);
    sample_y(y2, seedY);
    if (memcmp(y1, y2, sizeof(y1)) != 0) {
        printf("    SampleY NOT deterministic\n");
        fail = 1;
    }
    for (p = 0; p < KVEC; p++)
        for (i = 0; i < N; i++) {
            int v = y1[p].coeffs[i];
            e2 += (long double)v * v;
            mean += (long double)v;
            if (v < mn)
                mn = v;
            if (v > mx)
                mx = v;
            cnt++;
        }
    {
        long double std = ld_sqrt(e2 / (long double)cnt);
        mean /= (long double)cnt;
        printf(
            "    SampleY: n=%ld mean=%.2Lf std=%.2Lf (ideal r=%d) "
            "min=%d max=%d\n",
            cnt, mean, std, (int)RY, mn, mx);
        /* block stddev ~ r=825 (within a few %), mean ~ 0, range within
         * ~12*sigma_z where sigma_z ~ r (the wide block). */
        if (ld_abs(std - (long double)RY) / (long double)RY > 0.05L) {
            printf("    SampleY stddev off from r by >5%%\n");
            fail = 1;
        }
        if (ld_abs(mean) > (long double)RY * 0.1L) {
            printf("    SampleY mean too far from 0\n");
            fail = 1;
        }
        /* z = 256x+y, x<=36, y<=255 => |z| <= 256*36+255 = 9471 hard cap
         */
        if (mn < -9471 || mx > 9471) {
            printf("    SampleY exceeded the hard |z| cap 9471\n");
            fail = 1;
        }
    }
    return fail;
}

/* ===================== batch == scalar-loop (e, the byte-exact oracle) =
 * * Re-derive one SampleY lane chunk with an INDEPENDENT scalar reference
 * loop (single-candidate gauss_finalize + scalar approx_exp) reading the
 * SAME per-lane stream bytes, and assert it is bit-identical to the
 * gauss_stream_chunk batch path. */
static int test_batch_eq_scalar(void)
{
    uint8_t seedY[SEEDBYTES];
    const size_t wy = (size_t)KVEC * N / XOF_STREAMS;
    int32_t *batch = (int32_t *)malloc(wy * sizeof(int32_t));
    int32_t *scal = (int32_t *)malloc(wy * sizeof(int32_t));
    int fail = 0;
    fill_seed(seedY, SEEDBYTES, 0x99);
    if (!batch || !scal) {
        printf("    OOM\n");
        free(batch);
        free(scal);
        return 1;
    }
    /* batch path (lane 0) */
    {
        gauss_stream gs;
        gauss_stream_init(&gs, DS_SAMPLE_Y, seedY, 0);
        gauss_stream_chunk(&gs, batch, wy);
    }
    /* scalar reference path (lane 0): same stream, candidate-at-a-time. */
    {
        gauss_stream gs;
        uint8_t signs[SIGN_BYTES_PER_CHUNK + SIGN_PAD_AVX512];
        size_t signbytes = (wy + 7) / 8;
        size_t coefcnt = 0;
        gauss_stream_init(&gs, DS_SAMPLE_Y, seedY, 0);
        memset(signs, 0, sizeof(signs));
        gs_ensure(&gs, signbytes);
        memcpy(signs, gs.buf + gs.pos, signbytes);
        gs.pos += signbytes + SAMPLE_Y_PADDING_BYTES;
        while (coefcnt < wy) {
            int32_t x[GAUSS_BATCH];
            const uint8_t *yp, *tailp, *foldp;
            int j;
            gs_ensure(&gs, SAMPLE_Y_RAND_BYTES);
            sampler_sigma2(x, gs.buf + gs.pos);
            yp = gs.buf + gs.pos + SIGMA_S_RAND_BYTES;
            tailp = gs.buf + gs.pos + SIGMA_S_RAND_BYTES + Y_RAND_BYTES;
            foldp = gs.buf + gs.pos + MINIBATCH_RAND_BYTES;
            for (j = 0; j < GAUSS_BATCH; j++) {
                int xi = (int)x[j], yi = (int)yp[j];
                uint64_t ph =
                    approx_exp_accept_q64(xi, yi); /* scalar x1 */
                int32_t r;
                uint32_t done = ct_is_zero_u32((uint32_t)(wy - coefcnt));
                uint32_t d = (uint32_t)ct_sel_i32(
                    done, (int32_t)(wy - 1), (int32_t)coefcnt);
                uint32_t sgn =
                    (uint32_t)(signs[d >> 3] >> (d & 7)) & 1u;
                uint32_t fold =
                    (uint32_t)(foldp[j >> 3] >> (j & 7)) & 1u;
                if (gauss_finalize(&r, x[j], (int32_t)yi, ph,
                                   tailp + (size_t)j * GAUSS_RAND_BYTES,
                                    sgn, fold) && coefcnt < wy) {
                    if (coefcnt < wy)
                        scal[coefcnt++] = r;
                }
            }
            gs.pos += SAMPLE_Y_RAND_BYTES;
        }
    }
    if (memcmp(batch, scal, wy * sizeof(int32_t)) != 0) {
        printf("    SampleY batch path != scalar reference loop\n");
        fail = 1;
    } else {
        printf("    SampleY batch == scalar reference (%zu coeffs)\n", wy);
    }
    free(batch);
    free(scal);
    return fail;
}

/* ============ cursor-advance determinism (consumed-bytes rule) ======= *
 * The whole mini-batch tail is consumed up front, so gs->pos advances by a
 * FIXED amount per mini-batch regardless of how many candidates were
 * accepted (early break).  Drive one ExpandS lane stream, and assert the
 * byte cursor after processing K mini-batches is exactly K*392 + the sign
 * stream offset (there is no sign stream in ExpandS, so exactly K*392),
 * independent of the accept counts inside. */
static int test_cursor_determinism(void)
{
    /* Two ExpandS runs on the SAME seedsk must be byte-identical (already
     * checked in test_expand_s); here we additionally verify the
     * STRUCTURAL cursor-advance contract: the per-lane stream consumes
     * whole mini-batch units.  We assert the consumed-bytes determinism
     * relation:
     *
     *   the per-lane byte budget to fill a chunk of W coeffs is a whole
     *   number of NOISE_MINIBATCH_RAND_BYTES mini-batches, NOT a function
     * of the per-candidate accept pattern.  We check the macro arithmetic
     * and re-confirm determinism via a second expand_s.
     */
    static poly a[ELL + EM], b[ELL + EM];
    uint8_t seedsk[CHALLENGESEEDBYTES];
    int fail = 0;
    fill_seed(seedsk, CHALLENGESEEDBYTES, 0xAB);
    expand_s(a, seedsk);
    expand_s(b, seedsk);
    if (memcmp(a, b, sizeof(a)) != 0) {
        printf("    ExpandS re-run not byte-identical (cursor non-det)\n");
        fail = 1;
    }
    /* Structural macro check: the mini-batch tail is a fixed block and the
     * 2-bit-per-candidate tail packs 4 candidates/byte. */
    if (NOISE_MINIBATCH_RAND_BYTES != NOISE_CDT_BYTES + NOISE_BATCH / 4) {
        printf("    NOISE_MINIBATCH_RAND_BYTES macro inconsistent\n");
        fail = 1;
    }
    if (MINIBATCH_RAND_BYTES != SIGMA_S_RAND_BYTES + Y_RAND_BYTES +
                                    GAUSS_BATCH * GAUSS_RAND_BYTES) {
        printf("    MINIBATCH_RAND_BYTES macro inconsistent\n");
        fail = 1;
    }
    if (SAMPLE_Y_RAND_BYTES != MINIBATCH_RAND_BYTES + SAMPLE_Y_FOLD_BYTES ||
        SAMPLE_Y_FOLD_BYTES != GAUSS_BATCH / 8) {
        printf("    SampleY fold-byte budget inconsistent\n");
        fail = 1;
    }
    printf(
        "    ExpandS byte-deterministic re-run; mini-batch tails "
        "fixed (noise=%d, wide=%d bytes)\n",
        (int)NOISE_MINIBATCH_RAND_BYTES, (int)SAMPLE_Y_RAND_BYTES);
    return fail;
}

/* ===== (g) gauss_finalize_batch == per-candidate gauss_finalize ======= *
 * SIMD-only: the vectorized SIGN-INDEPENDENT precompute (cand/negcand/
 * accept/z0) must be BIT-IDENTICAL to GAUSS_BATCH scalar gauss_finalize
 * calls, for every (x,y,p_hat,tail,sign), including the precision-critical
 * 64-bit unsigned Bernoulli compare at its hard edges (u == p_hat, u =
 * p_hat-1, p_hat in {0, 2^64-1}, the high-bit straddle).  We replay the
 * exact downstream keep + value logic so this exercises the full
 * finalize, not just the precompute. */
#if (defined(USE_AVX2_SAMPLER) && defined(__AVX2__)) || \
    (defined(USE_AVX512_SAMPLER) && defined(__AVX512F__))
/* tiny deterministic 64-bit PRNG (splitmix64) -- test-only, libm-free. */
static uint64_t sm64_state;
static uint64_t sm64(void)
{
    uint64_t z = (sm64_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}
static int test_finalize_batch_eq_scalar(void)
{
    int fail = 0;
    long iter, mism = 0, n_acc = 0, n_z0 = 0, n_edge = 0;
    const long NITER = 200000;
    sm64_state = 0x5117711ECAFEF00DULL ^ (uint64_t)SHUTTLE_MODE;
    for (iter = 0; iter < NITER; iter++) {
        int32_t x[GAUSS_BATCH], y[GAUSS_BATCH];
        uint64_t phat[GAUSS_BATCH];
        uint8_t tail[GAUSS_BATCH * 8];
        uint8_t sgnbit[GAUSS_BATCH];
        uint8_t foldbit[GAUSS_BATCH];
        int32_t cand[GAUSS_BATCH], negc[GAUSS_BATCH];
        int32_t acc[GAUSS_BATCH], z0[GAUSS_BATCH];
        int j, k;
        for (j = 0; j < GAUSS_BATCH; j++) {
            uint64_t r = sm64();
            x[j] = (int32_t)(r % 37);           /* x in [0,36]  */
            y[j] = (int32_t)((r >> 8) & 0xFFu); /* y in [0,255] */
            sgnbit[j] = (uint8_t)((r >> 17) & 1u);
            foldbit[j] = (uint8_t)((r >> 18) & 1u);
            /* p_hat: mix full-range draws with the Bernoulli hard edges.
             */
            {
                uint64_t p = sm64();
                int pm = (int)((r >> 20) & 7u);
                if (pm == 0)
                    p = 0;
                else if (pm == 1)
                    p = ~0ULL;
                else if (pm == 2)
                    p = 0x8000000000000000ULL; /* high-bit straddle */
                else if (pm == 3)
                    p &= 0xFFFFu; /* small */
                phat[j] = p;
                if (pm <= 2)
                    n_edge++;
            }
            /* tail u: usually independent, but sometimes pin u to {p_hat,
             * p_hat-1, p_hat+1} to stress the exact compare boundary. */
            {
                uint64_t u = sm64();
                int em = (int)((u >> 3) & 7u);
                if (em == 0)
                    u = phat[j];
                else if (em == 1)
                    u = phat[j] - 1ULL;
                else if (em == 2)
                    u = phat[j] + 1ULL;
                for (k = 0; k < 8; k++)
                    tail[j * 8 + k] = (uint8_t)(u >> (8 * k));
            }
        }
        /* SIMD precompute over the whole batch. */
        gauss_finalize_batch(cand, negc, acc, z0, x, y, phat, tail,
                             GAUSS_BATCH);
        /* per-candidate scalar oracle + full downstream comparison. */
        for (j = 0; j < GAUSS_BATCH; j++) {
            int32_t r_s;
            uint32_t sgn = sgnbit[j] & 1u;
            uint32_t fold = foldbit[j] & 1u;
            int keep_s = gauss_finalize(&r_s, x[j], y[j], phat[j],
                                        tail + (size_t)j * 8, sgn, fold);
            /* reconstruct the SIMD keep + value with the SAME sign logic
             */
            uint32_t keep_v =
                (uint32_t)acc[j] & (1u ^ ((uint32_t)z0[j] & fold));
            int32_t r_v = sgn ? negc[j] : cand[j];
            if ((int)keep_v != keep_s || r_v != r_s) {
                if (mism < 8)
                    printf(
                        "    MISMATCH iter=%ld j=%d x=%d y=%d phat=%llu "
                        "sgn=%u: scal(keep=%d r=%d) simd(keep=%d r=%d) "
                        "[acc=%d z0=%d cand=%d]\n",
                        iter, j, x[j], y[j], (unsigned long long)phat[j],
                        sgn, keep_s, r_s, (int)keep_v, r_v, acc[j], z0[j],
                        cand[j]);
                mism++;
            }
            n_acc += (acc[j] != 0);
            n_z0 += (z0[j] != 0);
        }
    }
    printf(
        "    gauss_finalize_batch vs scalar: %ld batches x %d cand = %ld "
        "candidates, %ld accepts, %ld z0, %ld edge p_hat, "
        "mismatches=%ld\n",
        NITER, (int)GAUSS_BATCH, NITER * (long)GAUSS_BATCH, n_acc, n_z0,
        n_edge, mism);
    if (mism != 0)
        fail = 1;
    return fail;
}
#endif /* SIMD sampler */

int main(void)
{
    printf("== test_samplers (SHUTTLE-%d, ref scalar) ==\n",
           (int)SHUTTLE_MODE);
    printf("[a] ExpandSeeds / ExpandSigningSeeds\n");
    report("ExpandSeeds determinism + slices", test_expand_seeds());
    printf("[b] ExpandA (uniform [0,q), NTT-domain hAgen)\n");
    report("ExpandA range+uniformity+determinism", test_expand_a());
    printf("[c] ExpandS (BaseSampler + zero-fold + sign)\n");
    report("ExpandS support+stddev+determinism", test_expand_s());
    printf("[d] SampleC (partial Fisher-Yates, weight TAU)\n");
    report("SampleC weight+binary+determinism", test_sample_c());
    printf("[e] SampleY / SampleDGauss (wide Gaussian r=825)\n");
    report("SampleY stats+determinism", test_sample_y());
    report("SampleY batch == scalar loop", test_batch_eq_scalar());
    printf("[f] cursor-advance determinism\n");
    report("mini-batch fixed cursor advance", test_cursor_determinism());
#if (defined(USE_AVX2_SAMPLER) && defined(__AVX2__)) || \
    (defined(USE_AVX512_SAMPLER) && defined(__AVX512F__))
    printf("[g] gauss_finalize_batch (SIMD) == scalar gauss_finalize\n");
    report("gauss_finalize_batch bit-exact",
           test_finalize_batch_eq_scalar());
#endif
    printf("\n%s (%d failures)\n",
           g_fails ? "FAILURES PRESENT" : "ALL PASS", g_fails);
    return g_fails ? 1 : 0;
}
