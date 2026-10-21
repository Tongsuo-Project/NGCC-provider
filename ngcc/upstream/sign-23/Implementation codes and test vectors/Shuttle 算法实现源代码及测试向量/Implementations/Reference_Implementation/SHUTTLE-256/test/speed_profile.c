/*
 * speed_profile.c -- per-component time + randomness breakdown of the
 * SHUTTLE keygen / sign / verify path.
 *
 * Build with the profiler enabled:  make profile
 *   (-DPROF_TIME -DPROF_RAND).  Runs NKG keygens / NSIG signs / NSIG
 * verifies (incl. rejection retries) and, per primitive, prints:
 *   - cycles per component as %% of the per-primitive total (the PT_*
 *     buckets: setup / expand_A / sample_y / commitment / highbits /
 *     challenge / irs / normcheck / makehint / rANS for sign; the KG_* /
 *     VF_* buckets for keygen / verify);
 *   - XOF bytes squeezed per randomness context (PC_*) and the fine
 *     sampler consumption (PU_*), to expose any PRNG buffer waste.
 * See prof.h / prof.c.
 *
 * Calls go through the NGCC sig_* interface (drng_algorithm-backed),
 * matching speed_sign.c; the DRBG is seeded once, deterministically.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include "params.h"
#include "prof.h"

#define MLEN 64
#ifndef NSIG
#    define NSIG 2000
#endif
#ifndef NKG
#    define NKG 500 /* keygen is costlier (norm-window retries); fewer */
#endif

DRNG_ctx drng_algorithm;

static void seed_drng(void)
{
    uint8_t nonce[64];
    int i;
    for (i = 0; i < 16; ++i)
        memcpy(nonce + 4 * i, "prof", 4);
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
    unsigned long long pkl, skl, snl;
    int i;

    if (!pk || !sk || !sn) {
        fprintf(stderr, "speed_profile: malloc failed\n");
        return 1;
    }
    seed_drng();
    memset(msg, 0xA5, sizeof msg);

#if defined(PROF_TIME) || defined(PROF_RAND)
    {
        char title[48];
        snprintf(title, sizeof title, "mode=%d %s", (int)LAMBDA,
#    if defined(SHA3_MODE)
                 "SHA3");
#    else
                 "NGCC");
#    endif

        /* --- keygen --- */
        for (i = 0; i < 16; i++) { /* warmup (caches, freq) */
            pkl = pk_len;
            skl = sk_len;
            if (sig_keygen(pk, &pkl, sk, &skl) != 0)
                return 1;
        }
        prof_reset();
        for (i = 0; i < NKG; i++) {
            pkl = pk_len;
            skl = sk_len;
            if (sig_keygen(pk, &pkl, sk, &skl) != 0)
                return 1;
        }
        prof_report(title, NKG);

        /* --- sign --- (fresh key, then NSIG signatures incl. retries) */
        pkl = pk_len;
        skl = sk_len;
        if (sig_keygen(pk, &pkl, sk, &skl) != 0)
            return 1;
        for (i = 0; i < 64; i++) { /* warmup */
            snl = sn_cap;
            if (sig_sign(sk, sk_len, msg, MLEN, sn, &snl) != 0)
                return 1;
        }
        prof_reset();
        for (i = 0; i < NSIG; i++) {
            snl = sn_cap;
            if (sig_sign(sk, sk_len, msg, MLEN, sn, &snl) != 0)
                return 1;
        }
        prof_report(title, NSIG);

        /* --- verify --- (one valid signature, verified NSIG times) */
        snl = sn_cap;
        if (sig_sign(sk, sk_len, msg, MLEN, sn, &snl) != 0)
            return 1;
        for (i = 0; i < 64; i++)
            (void)sig_verify(pk, pk_len, sn, snl, msg, MLEN);
        prof_reset();
        for (i = 0; i < NSIG; i++)
            (void)sig_verify(pk, pk_len, sn, snl, msg, MLEN);
        prof_report(title, NSIG);
    }
#else
    (void)pkl;
    (void)skl;
    (void)snl;
    (void)i;
    printf(
        "speed_profile: build with `make profile` "
        "(-DPROF_TIME -DPROF_RAND)\n");
#endif

    free(pk);
    free(sk);
    free(sn);
    return 0;
}
