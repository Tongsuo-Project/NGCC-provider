/*
 * sign_dump.c -- end-to-end C oracle dumper for the SHUTTLE Python
 * cross-check.  Calls the reference KeyGen / Sign / Verify on a FIXED
 * (xi, msg, rnd) and prints pk / sk / sig as hex, plus the verify result.
 * The Python side (xcheck_sign.py) reproduces these BYTE-FOR-BYTE.
 *
 * Build (per mode, DISABLE_NAMESPACE, default rANS sig path):
 *   gcc -O2 -std=c99 -DSHUTTLE_MODE=<m> -DDISABLE_NAMESPACE=1 [-DSHA3_MODE]
 *       -I../../ref -I../../tools -I../../ref/ntt/<qset>
 *       sign_dump.c <scheme .c files> -o sign_dump_<m>
 * (add -DSIG_RAW to exercise the fixed-length raw path.)
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "params.h"
#include "rans.h" /* SIG_RAW_PACKED_BYTES / SIG_PACKED_BYTES */

int crypto_sign_keypair_xi(uint8_t *pk, uint8_t *sk,
                           const uint8_t xi[SEEDBYTES]);
int crypto_sign_signature_rnd(uint8_t *sig, size_t *siglen, const uint8_t *m,
                              size_t mlen, const uint8_t *sk,
                              const uint8_t rnd[/*RNDBYTES*/]);
int crypto_sign_verify(const uint8_t *sig, size_t siglen, const uint8_t *m,
                       size_t mlen, const uint8_t *pk);

#if defined(SIG_RAW)
#    define SIG_BYTES SIG_RAW_PACKED_BYTES
#else
#    define SIG_BYTES SIG_PACKED_BYTES
#endif

static void puthex(const char *tag, const uint8_t *b, size_t n)
{
    printf("%s ", tag);
    for (size_t i = 0; i < n; ++i)
        printf("%02x", b[i]);
    printf("\n");
}

int main(void)
{
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t sig[SIG_BYTES];
    size_t siglen = 0;

    /* fixed deterministic inputs (must match the Python driver). */
    uint8_t xi[SEEDBYTES];
    uint8_t rnd[SEEDBYTES]; /* RNDBYTES == SEEDBYTES */
    uint8_t msg[33];
    int i;

    for (i = 0; i < (int)SEEDBYTES; ++i)
        xi[i] = (uint8_t)(0x10 + i);
    for (i = 0; i < (int)SEEDBYTES; ++i)
        rnd[i] = (uint8_t)(0xA0 + i);
    for (i = 0; i < 33; ++i)
        msg[i] = (uint8_t)(0x30 + i);

    printf("MODE %d\n", (int)SHUTTLE_MODE);

    if (crypto_sign_keypair_xi(pk, sk, xi) != 0) {
        printf("ERR keygen\n");
        return 1;
    }
    puthex("PK", pk, CRYPTO_PUBLICKEYBYTES);
    puthex("SK", sk, CRYPTO_SECRETKEYBYTES);

    if (crypto_sign_signature_rnd(sig, &siglen, msg, sizeof msg, sk, rnd) != 0) {
        printf("ERR sign\n");
        return 1;
    }
    printf("SIGLEN %zu\n", siglen);
    puthex("SIG", sig, siglen);

    {
        int rc = crypto_sign_verify(sig, siglen, msg, sizeof msg, pk);
        printf("VERIFY %d\n", rc);
    }
    return 0;
}
