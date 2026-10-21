/*
 * timecop_smoke.c -- SHUTTLE dynamic variable-latency (KyberSlash/TIMECOP)
 * smoke. Backend-agnostic; built against each backend's signing sources via
 * -I../ref/test (run_sign_smoke.sh). The GATE is the variable-latency property
 * ONLY: FAIL iff the patched Memcheck reports "Variable-latency instruction
 * operand ...". Secret-dependent CONTROL FLOW is out of scope here (owned by
 * ct_scan + dudect-sign); its intentional constant-time cmov/select idioms
 * (Memcheck:Cond) are suppressed via tools/timecop/shuttle-ct.supp.
 *
 * Taint scope (SHUTTLE skEncode re-key): the secret region of the packed sk is
 *   seedA(SEEDBYTES) + EM*POLYPK_PACKEDBYTES (b body, PUBLIC/declassified)
 *   + masterSeed(CHALLENGESEEDBYTES)            <-- SECRET
 *   + tr(CHALLENGESEEDBYTES)                    <-- public hash, tainted conservatively
 *   + ELL*(secret s) + EM*(secret e')           <-- SECRET
 * so we mark [SEEDBYTES + EM*POLYPK_PACKEDBYTES, CRYPTO_SECRETKEYBYTES)
 * UNDEFINED, keep the seedA + b body DEFINED, and mark the signing randomness
 * `rnd` (NGCC: drawn from the SM3 Hash-DRBG; here injected via _rnd) SECRET.
 * Taint propagates through normal signing data flow into the samplers / IRS.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <valgrind/memcheck.h>
#include <valgrind/valgrind.h>

#include "api.h"
#include "params.h"

int crypto_sign_keypair_xi(uint8_t *pk, uint8_t *sk, const uint8_t xi[SEEDBYTES]);
int crypto_sign_signature_rnd(uint8_t *sig, size_t *siglen, const uint8_t *m,
                              size_t mlen, const uint8_t *sk,
                              const uint8_t rnd[SEEDBYTES]);

static void mark_secret_sk(uint8_t *sk)
{
    if (!RUNNING_ON_VALGRIND)
        return;
    /* Public/declassified prefix: seedA + b body. */
    size_t pub_prefix = (size_t)SEEDBYTES + (size_t)EM * POLYPK_PACKEDBYTES;
    VALGRIND_MAKE_MEM_DEFINED(sk, pub_prefix);
    /* Secret tail: masterSeed + tr + s + e' (everything else). */
    VALGRIND_MAKE_MEM_UNDEFINED(sk + pub_prefix,
                                (size_t)CRYPTO_SECRETKEYBYTES - pub_prefix);
}

int main(void)
{
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
#if defined(SIG_RAW)
    uint8_t sig[SIG_RAW_PACKED_BYTES];
#else
    uint8_t sig[CRYPTO_BYTES];
#endif
    uint8_t xi[SEEDBYTES];
    uint8_t rnd[SEEDBYTES];
    uint8_t msg[33];
    size_t siglen = 0;

    for (size_t i = 0; i < sizeof xi; i++)
        xi[i] = (uint8_t)(0x5a ^ (uint8_t)(17 * i));
    for (size_t i = 0; i < sizeof rnd; i++)
        rnd[i] = (uint8_t)(0xa5 ^ (uint8_t)(31 * i));
    for (size_t i = 0; i < sizeof msg; i++)
        msg[i] = (uint8_t)(0xa0u + i);

    if (crypto_sign_keypair_xi(pk, sk, xi) != 0) {
        puts("FAIL timecop_smoke keypair");
        return 1;
    }

    /* Mark the secret key region and the signing randomness as secret. */
    mark_secret_sk(sk);
    if (RUNNING_ON_VALGRIND)
        VALGRIND_MAKE_MEM_UNDEFINED(rnd, sizeof rnd);

    if (crypto_sign_signature_rnd(sig, &siglen, msg, sizeof msg, sk, rnd) != 0) {
        puts("FAIL timecop_smoke sign");
        return 1;
    }

    /* Declassify the produced signature + public inputs before verify. */
    if (RUNNING_ON_VALGRIND) {
        VALGRIND_MAKE_MEM_DEFINED(sig, siglen);
        VALGRIND_MAKE_MEM_DEFINED(&siglen, sizeof siglen);
        VALGRIND_MAKE_MEM_DEFINED(pk, sizeof pk);
        VALGRIND_MAKE_MEM_DEFINED(msg, sizeof msg);
    }

    if (crypto_sign_verify(sig, siglen, msg, sizeof msg, pk) != 0) {
        puts("FAIL timecop_smoke verify");
        return 1;
    }

    printf("PASS timecop_smoke mode=%d siglen=%zu\n", (int)LAMBDA, siglen);
    return 0;
}
