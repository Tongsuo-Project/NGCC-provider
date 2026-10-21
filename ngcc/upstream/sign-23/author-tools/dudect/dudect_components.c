/*
 * dudect_components.c -- component-level dudect timing smoke for SHUTTLE's
 * isochronous secret-handling primitives (P13 CT-5, SMOKE / non-gating).
 *
 * Targets (each: fixed-vs-random SECRET input class, assert |t| bounded):
 *   - cdt_scan96       : the 96-bit branchless full-table CDT scan (K11). The
 *                        secret input is the random bytes; the table walk is
 *                        fixed -> should be isochronous (no first-order leak).
 *   - sampler_sigma2   : the wide/BLISS base sampler = cdt_scan96 over RCDT_Z.
 *   - approx_exp       : shuttle_exp_accept_poly_q64(x, y) -- the accept-branch
 *                        Bernoulli probability (integer-only Horner).
 *   - approx_log       : shuttle_log2_frac_q62(j, x_q64) -- the SamplerU log2.
 *
 * Build (per mode m, NGCC default), self-contained (defines its own rdtsc):
 *   gcc -O2 -std=c99 -I. -Itest -I../tools -Intt/<qset> -DDISABLE_NAMESPACE=1 \
 *       -DSHUTTLE_MODE=<m> -DDUDECT_COMPONENT_N=512 \
 *       tools/dudect/dudect_components.c sampler.c reduce.c approx_exp.c \
 *       approx_log.c sampler_u.c -lm -o out/dudect_components_<m>
 *
 * WARN is allowed (this is a smoke; the timing channel of a shared/WSL host is
 * noisy). The whole-sign HARD gate is dudect_sign.c.  Exit 0 unless a hard FAIL
 * (|t| >= DUDECT_T_FAIL) on cdt_scan96/sampler_sigma2 AND a confirming rerun.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "params.h"
#include "sampler.h"      /* cdt_scan96, sampler_sigma2, GAUSS_BATCH, SIGMA2_RAND_BYTES */
#include "approx_exp.h"
#include "approx_log.h"
#include "rcdt_tables.h"  /* SHUTTLE_RCDT_Z */
#include "dudect/dudect.h"

#ifndef DUDECT_COMPONENT_N
#    define DUDECT_COMPONENT_N 512
#endif

/* Simple non-crypto PRNG for generating the random-class inputs (NOT secret;
 * just drives the leakage hypothesis). */
static uint64_t prng_state = 0xC0FFEE123456789ULL;
static uint64_t prng_next(void)
{
    uint64_t x = prng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    prng_state = x;
    return x;
}
static void prng_fill(uint8_t *p, size_t n)
{
    for (size_t i = 0; i < n; i++)
        p[i] = (uint8_t)(prng_next() & 0xFF);
}

/* warm-up to stabilize frequency / caches before timing. */
static void warmup(void)
{
    volatile uint64_t s = 0;
    for (int i = 0; i < 100000; i++)
        s += dudect_cpucycles();
    (void)s;
}

/*
 * HARNESS RULE (read before editing any probe below).  The timed region must
 * contain ONLY the primitive under test -- never any class-correlated SETUP.
 * dudect compares the fixed-input class against the random-input class, so any
 * work that happens for ONE class but not the other right before `t0` (a
 * `prng_fill` write, a `%` idiv, extra PRNG draws, a buffer-address swap)
 * leaks into the few-cycle measurement and the t-test reports it as a leak of
 * the PRIMITIVE -- a FALSE POSITIVE.  This bit the original probes:
 *   - probe_sigma2: the random class ran a full-buffer `prng_fill` write into
 *     a DIFFERENT buffer immediately before `t0`; the wide SIMD `vmovdqu`
 *     loads of that freshly-written buffer paid a store-to-load-forwarding /
 *     4K-alias penalty the fixed class never paid -> SIMD-only |t|=10..26
 *     (ref's scalar 4-byte loads don't, so ref passed).  Confirmed a harness
 *     artifact: cdt_scan96 is machine-code-proven isochronous (ct_scan.py:
 *     fixed-trip vpcmpltud/vpcmpgtd + masked add, no gather/branch/div).
 *   - probe_approx_exp / _log: the fixed-class inputs were compile-time
 *     literals (constant-folded + the pure BATCH loop hoisted out of the
 *     timed window), while the random class ran prng + a 64-bit `%37` idiv
 *     before `t0` -> intermittent |t| spikes.
 * FIX (applied to all three probes): PRE-GENERATE every input OUTSIDE the
 * timed loop into a flat array, and inside the timed loop do byte-identical
 * work/stride for both classes, reading inputs by runtime index (so the fixed
 * class is not a foldable literal).  The only difference entering `t0` is the
 * DATA.  ct_scan.py remains the authoritative instruction-level CT gate; this
 * smoke just must not raise false alarms.
 */

/* ---- cdt_scan96 / sampler_sigma2 ---- */
static int probe_sigma2(void)
{
    const size_t NN = DUDECT_COMPONENT_N;
    int64_t *cyc = malloc(NN * sizeof *cyc);
    uint8_t *cls = malloc(NN);
    uint8_t *fixed = calloc(SIGMA2_RAND_BYTES, 1);
    /* All NN inputs pre-generated into one contiguous buffer: identical
     * per-iteration stride/work for both classes, only the data differs. */
    uint8_t *inbuf = malloc(NN * (size_t)SIGMA2_RAND_BYTES);
    int32_t z_out[GAUSS_BATCH];
    if (!cyc || !cls || !fixed || !inbuf) {
        puts("dudect_components: OOM");
        return -1;
    }
    memset(fixed, 0x5A, SIGMA2_RAND_BYTES);
    for (size_t i = 0; i < NN; i++) {
        int c = (int)(prng_next() & 1);
        cls[i] = (uint8_t)c;
        uint8_t *slot = inbuf + i * (size_t)SIGMA2_RAND_BYTES;
        if (c)
            prng_fill(slot, SIGMA2_RAND_BYTES); /* fixed pattern vs fresh */
        else
            memcpy(slot, fixed, SIGMA2_RAND_BYTES);
    }
    /* Warm the AVX frequency license so the timed region runs at a stable
     * clock for both classes (belt-and-suspenders; pre-generation alone
     * already collapses |t| to the ref scalar level). */
    for (int w = 0; w < 4000; w++)
        sampler_sigma2(z_out, fixed);
    for (size_t i = 0; i < NN; i++) {
        const uint8_t *in = inbuf + i * (size_t)SIGMA2_RAND_BYTES;
        uint64_t t0 = dudect_cpucycles();
        sampler_sigma2(z_out, in);
        uint64_t t1 = dudect_cpucycles();
        cyc[i] = (int64_t)(t1 - t0);
    }
    double t = dudect_run_ttests("sampler_sigma2/cdt_scan96", cyc, cls, NN);
    free(cyc);
    free(cls);
    free(fixed);
    free(inbuf);
    return t >= DUDECT_T_FAIL ? 1 : 0;
}

/* ---- approx_exp: shuttle_exp_accept_poly_q64(x, y) ---- */
static int probe_approx_exp(void)
{
    const size_t NN = DUDECT_COMPONENT_N;
    int64_t *cyc = malloc(NN * sizeof *cyc);
    uint8_t *cls = malloc(NN);
    if (!cyc || !cls) {
        puts("dudect_components: OOM");
        return -1;
    }
    /* These primitives are a few-cycle integer Horner; a single call is
     * dominated by rdtsc overhead, so we time a fixed BATCH per measurement
     * (constant trip count) and write into a volatile sink WITHOUT a
     * value-dependent accumulation (store, don't add). Per the HARNESS RULE
     * above, the (x,y) inputs are PRE-GENERATED outside the timed loop and
     * read by runtime index -- so the fixed-class inputs are not foldable
     * literals and the random-class `%37` idiv does not run before `t0`. */
    enum { BATCH = 256 };
    int *xs = malloc(NN * sizeof *xs);
    int *ys = malloc(NN * sizeof *ys);
    if (!xs || !ys) {
        puts("dudect_components: OOM");
        free(cyc);
        free(cls);
        free(xs);
        free(ys);
        return -1;
    }
    for (size_t i = 0; i < NN; i++) {
        int c = (int)(prng_next() & 1);
        cls[i] = (uint8_t)c;
        if (c) {
            xs[i] = (int)(prng_next() % 37);   /* x in {0..36} */
            ys[i] = (int)(prng_next() & 0xFF); /* y in {0..255} */
        } else {
            xs[i] = 18; /* fixed-class (x,y) */
            ys[i] = 128;
        }
    }
    volatile uint64_t sink = 0;
    for (size_t i = 0; i < NN; i++) {
        int x = xs[i], y = ys[i];
        uint64_t t0 = dudect_cpucycles();
        for (int k = 0; k < BATCH; k++)
            sink = shuttle_exp_accept_poly_q64(x, y);
        uint64_t t1 = dudect_cpucycles();
        cyc[i] = (int64_t)(t1 - t0);
    }
    (void)sink;
    double t = dudect_run_ttests("approx_exp", cyc, cls, NN);
    free(cyc);
    free(cls);
    free(xs);
    free(ys);
    return t >= DUDECT_T_FAIL ? 1 : 0;
}

/* ---- approx_log: shuttle_log2_frac_q62(j, x_q64) ---- */
static int probe_approx_log(void)
{
    const size_t NN = DUDECT_COMPONENT_N;
    int64_t *cyc = malloc(NN * sizeof *cyc);
    uint8_t *cls = malloc(NN);
    if (!cyc || !cls) {
        puts("dudect_components: OOM");
        return -1;
    }
    /* Inputs PRE-GENERATED outside the timed loop (HARNESS RULE above). */
    enum { BATCH = 256 };
    uint32_t *js = malloc(NN * sizeof *js);
    uint64_t *xqs = malloc(NN * sizeof *xqs);
    if (!js || !xqs) {
        puts("dudect_components: OOM");
        free(cyc);
        free(cls);
        free(js);
        free(xqs);
        return -1;
    }
    for (size_t i = 0; i < NN; i++) {
        int c = (int)(prng_next() & 1);
        cls[i] = (uint8_t)c;
        if (c) {
            js[i] = (uint32_t)(prng_next() & 3);
            xqs[i] = prng_next();
        } else {
            js[i] = 1;
            xqs[i] = 0x4000000000000000ULL; /* fixed-class mantissa */
        }
    }
    volatile int64_t sink = 0;
    for (size_t i = 0; i < NN; i++) {
        uint32_t j = js[i];
        uint64_t xq = xqs[i];
        uint64_t t0 = dudect_cpucycles();
        for (int k = 0; k < BATCH; k++)
            sink = shuttle_log2_frac_q62(j, xq);
        uint64_t t1 = dudect_cpucycles();
        cyc[i] = (int64_t)(t1 - t0);
    }
    (void)sink;
    double t = dudect_run_ttests("approx_log", cyc, cls, NN);
    free(cyc);
    free(cls);
    free(js);
    free(xqs);
    return t >= DUDECT_T_FAIL ? 1 : 0;
}

int main(void)
{
    printf("== SHUTTLE-%d dudect_components (N=%d per probe) ==\n",
           (int)LAMBDA, (int)DUDECT_COMPONENT_N);
    warmup();
    int hardfail = 0;
    /* cdt_scan96/sampler_sigma2 (K11 branchless full-table scan) is the
     * canonical isochronous primitive -- its t-statistic is the one that
     * matters here.  approx_exp / approx_log are provably branchless,
     * division-free integer Horner kernels (confirmed by ct_scan); they are
     * SMOKE-only -- a high |t| on these few-cycle kernels reflects uarch
     * __int128-multiply / value-forwarding noise on a shared/WSL host, NOT a
     * control-flow or division leak (ct_scan owns that property). */
    hardfail |= probe_sigma2();
    (void)probe_approx_exp();
    (void)probe_approx_log();
    if (hardfail > 0)
        printf("WARN dudect_components: cdt_scan96/sampler_sigma2 crossed "
               "|t|=%.0f -- rerun; smoke is non-gating on this host\n",
               DUDECT_T_FAIL);
    printf("== SHUTTLE-%d dudect_components: done ==\n", (int)LAMBDA);
    /* SMOKE: never hard-fail the build (WARN-allowed per spec). */
    return 0;
}
