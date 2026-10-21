/*
 * speed_sign.c -- end-to-end cycle benchmark for keygen / sign / verify
 * over the NGCC SIG interface (sig_keygen / sig_sign / sig_verify).
 *
 * Calls go through the production NGCC contract (SIG_AlgorithmInstance.c),
 * NOT a Dilithium-style crypto_sign_* API.  keygen and sign use the tail
 * benchmark because their latency distributions are wide -- KeyGen has the
 * [B_k', B_k] norm-window rejection (accept ~34/47/38 %), and Sign, though
 * its IRS inner loop is rejection-free, still aborts on the B_v gate / rANS
 * out-of-support restart.  verify has no rejection and is reported as a
 * single median.  Both keygen+sign tail-bench and the verify median use the
 * mandated 64-byte to-be-signed message.
 *
 * A throughput figure (ops/s) is also emitted per primitive via
 * clock_gettime(CLOCK_MONOTONIC) over >= 100 iterations (the NGCC mandate).
 *
 * The driver is reproducible: the scheme-internal DRBG (drng_algorithm,
 * which the NGCC adapter draws xi/rnd from) is seeded once from a fixed
 * 64-byte nonce, exactly as the KAT harness does, so no real entropy is
 * needed and the run is deterministic.
 *
 * Build (per mode m), NGCC_MODE default:
 *   gcc -O2 -std=c99 -Wpedantic -Wall -Wextra -I. -Itest -I../tools \
 *       -Intt/<qset> -DSHUTTLE_MODE=<m> \
 *       test/speed_sign.c test/cpucycles.c SIG_AlgorithmInstance.c sign.c \
 *       polyvec.c sampler.c sampler_u.c irs.c rounding.c packing.c poly.c \
 *       poly_ntt.c reduce.c rans.c approx_exp.c approx_log.c symmetric.c \
 *       ntt/<qset>/ntt_ref.c drng.c auxfunc.c -o out/speed_sign_<m>
 */
/* clock_gettime / CLOCK_MONOTONIC are POSIX; request them under -std=c99
 * (which otherwise hides the POSIX surface). */
#if !defined(_POSIX_C_SOURCE) || (_POSIX_C_SOURCE < 199309L)
#    undef _POSIX_C_SOURCE
#    define _POSIX_C_SOURCE 199309L
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "SIG_AlgorithmInstance.h"
#include "cpucycles.h"
#include "drng.h"
#include "params.h" /* LAMBDA, CRYPTO_BYTES, SIG_PACKED_BYTES (typ size) */
#include "rans.h"   /* SIG_PACKED_BYTES / SIG_RAW_PACKED_BYTES        */

#define NV 10000
#define MLEN 64 /* NGCC mandated 64-byte to-be-signed message */
#ifndef NGCC_ITERS
#    define NGCC_ITERS 1000 /* throughput loop (>= 100 mandate) */
#endif

/* The scheme-internal DRBG the NGCC adapter draws xi/rnd from.  Provided
 * here (the KAT harness owns it in production); seeded once below. */
DRNG_ctx drng_algorithm;

/* The typical/realized signature length for the compiled packing path.
 * RAW path is fixed-length larger; rANS path is fixed SIG_PACKED_BYTES. */
#if defined(SIG_RAW)
#    define SIG_TYP_BYTES ((unsigned long long)SIG_RAW_PACKED_BYTES)
#else
#    define SIG_TYP_BYTES ((unsigned long long)SIG_PACKED_BYTES)
#endif

/* tail-bench contexts: one persistent key/buffer set reused across runs. */
typedef struct keygen_bench_ctx {
    uint8_t *pk;
    uint8_t *sk;
    unsigned long long pkl, skl;
} keygen_bench_ctx;

typedef struct sign_bench_ctx {
    uint8_t *sk;
    uint8_t *sn;
    unsigned long long skl, snl;
    uint8_t msg[MLEN];
} sign_bench_ctx;

static int keygen_once(void *vctx)
{
    keygen_bench_ctx *ctx = vctx;
    return sig_keygen(ctx->pk, &ctx->pkl, ctx->sk, &ctx->skl);
}

static int sign_once(void *vctx)
{
    sign_bench_ctx *ctx = vctx;
    return sig_sign(ctx->sk, ctx->skl, ctx->msg, MLEN, ctx->sn, &ctx->snl);
}

/* Re-seed drng_algorithm from a fixed nonce so the whole bench is
 * deterministic and reproducible (the bench draws from the same DRBG the
 * KAT harness seeds; we reseed once before the run). */
static void seed_drng(void)
{
    uint8_t nonce[64];
    int i;
    for (i = 0; i < 16; ++i)
        memcpy(nonce + 4 * i, "spdb", 4); /* "speed bench" deterministic */
    init_random_number(&drng_algorithm, nonce, 64);
}

int main(void)
{
    const unsigned long long pk_len = sig_get_pk_len_bytes();
    const unsigned long long sk_len = sig_get_sk_len_bytes();
    const unsigned long long sn_cap = sig_get_sn_len_bytes();

    uint8_t *pk = malloc((size_t)pk_len);
    uint8_t *sk = malloc((size_t)sk_len);
    uint8_t *sn = malloc((size_t)sn_cap);
    uint8_t msg[MLEN];
    static uint64_t cv[NV];
    keygen_bench_ctx kctx;
    sign_bench_ctx sctx;
    tail_sign_result keygen_result, sign_result;
    int i, rc;

    if (!pk || !sk || !sn) {
        fprintf(stderr, "speed_sign: malloc failed\n");
        return 1;
    }

    seed_drng();
    memset(msg, 0xA5, sizeof msg); /* fixed 64-byte message */

    /* ---- keygen tail-bench (wide: norm-window rejection) ---- */
    kctx.pk = pk;
    kctx.sk = sk;
    kctx.pkl = pk_len;
    kctx.skl = sk_len;
    if (bench_tail_sign("keygen", &keygen_result, keygen_once, &kctx) != 0) {
        fprintf(stderr, "speed_sign: keygen returned nonzero\n");
        return 1;
    }

    /* ---- sign tail-bench (rare B_v/rANS restart tail) ---- */
    sctx.sk = sk; /* reuse the last keygen's sk */
    sctx.sn = sn;
    sctx.skl = sk_len;
    sctx.snl = sn_cap;
    memcpy(sctx.msg, msg, MLEN);
    if (bench_tail_sign("sign", &sign_result, sign_once, &sctx) != 0) {
        fprintf(stderr, "speed_sign: sign returned nonzero\n");
        return 1;
    }

    /* ---- verify median ---- */
    {
        unsigned long long snl = sn_cap;
        rc = sig_sign(sk, sk_len, msg, MLEN, sn, &snl);
        if (rc != 0) {
            fprintf(stderr, "speed_sign: sign-for-verify returned %d\n", rc);
            return 1;
        }
        for (i = 0; i < NV; i++) {
            uint64_t a = cpucycles();
            (void)sig_verify(pk, pk_len, sn, snl, msg, MLEN);
            cv[i] = cpucycles() - a;
        }

        /* sig is the exact realized length: SIG_TYP_BYTES == sn_cap
         * (sig_get_sn_len_bytes() == CRYPTO_BYTES == SIG_PACKED_BYTES). */
        printf(
            "mode=%d, pk=%lluB, sk=%lluB, sig=%lluB, mlen=%dB\n",
            (int)LAMBDA, pk_len, sk_len, SIG_TYP_BYTES, MLEN);
        bench_print_tail(&keygen_result);
        bench_print_tail(&sign_result);
        printf("verify median (%d runs): %llu cycles\n", NV,
               (unsigned long long)cpucycles_median(cv, NV));
    }

    /* ---- throughput (ops/s) via wall clock over >= 100 iters ---- */
    {
        struct timespec t0, t1;
        double el_ns;
        unsigned long long snl;

        clock_gettime(CLOCK_MONOTONIC, &t0);
        for (i = 0; i < NGCC_ITERS; i++) {
            kctx.pkl = pk_len;
            kctx.skl = sk_len;
            if (sig_keygen(pk, &kctx.pkl, sk, &kctx.skl) != 0)
                return 1;
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        el_ns = (double)(t1.tv_sec - t0.tv_sec) * 1e9 +
                (double)(t1.tv_nsec - t0.tv_nsec);
        printf("keygen throughput: %.1f ops/s (%d iters)\n",
               (double)NGCC_ITERS * 1e9 / el_ns, NGCC_ITERS);

        clock_gettime(CLOCK_MONOTONIC, &t0);
        for (i = 0; i < NGCC_ITERS; i++) {
            snl = sn_cap;
            if (sig_sign(sk, sk_len, msg, MLEN, sn, &snl) != 0)
                return 1;
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        el_ns = (double)(t1.tv_sec - t0.tv_sec) * 1e9 +
                (double)(t1.tv_nsec - t0.tv_nsec);
        printf("sign   throughput: %.1f ops/s (%d iters)\n",
               (double)NGCC_ITERS * 1e9 / el_ns, NGCC_ITERS);

        snl = sn_cap;
        if (sig_sign(sk, sk_len, msg, MLEN, sn, &snl) != 0)
            return 1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        for (i = 0; i < NGCC_ITERS; i++)
            (void)sig_verify(pk, pk_len, sn, snl, msg, MLEN);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        el_ns = (double)(t1.tv_sec - t0.tv_sec) * 1e9 +
                (double)(t1.tv_nsec - t0.tv_nsec);
        printf("verify throughput: %.1f ops/s (%d iters)\n",
               (double)NGCC_ITERS * 1e9 / el_ns, NGCC_ITERS);
    }

    free(pk);
    free(sk);
    free(sn);
    return 0;
}
