/*
 * m4_dump.c -- empirical rANS-validation harness for SHUTTLE (P12, M4).
 *
 * Signs N messages with the real C ref (rANS packing), and for EACH signature
 * dumps:
 *   - "RLEN <n>"            : the realized rANS com length (the uint16 LE
 *                            field at offset CHALLENGESEEDBYTES).
 *   - "SYMS <q0..> | <qs..> | <h..>" : the gathered Q0/Qs/h symbols (the
 *                            block-adaptive heads z0>>b0, zs>>bs, and the
 *                            hint), so Python builds empirical histograms and
 *                            compares to the model PMFs (rans_model.py).
 *
 * This is the M4 evidence: over N real signatures, does the rANS reserve
 * (RANS_RESERVED_BYTES) hold (max RLEN <= reserve)?  Do the empirical Q0/Qs/h
 * laws match the theoretical PMFs the rANS generators assumed?
 *
 * Build (per mode, DISABLE_NAMESPACE, NO -DSIG_RAW so the rANS path is live):
 *   gcc -O2 -std=c99 -I../../ref -I../../ref/test -I../../tools \
 *       -I../../ref/ntt/<qset> -DDISABLE_NAMESPACE=1 -DSHUTTLE_MODE=<m> \
 *       m4_dump.c ../../ref/<full sign source list> -o m4_dump_<m>
 *
 * Run:  ./m4_dump_<m> <N>     (default N = 2000)
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "params.h"
#include "packing.h"
#include "poly.h"
#include "rans.h"

int crypto_sign_keypair_xi(uint8_t *pk, uint8_t *sk,
                           const uint8_t xi[SEEDBYTES]);
int crypto_sign_signature_rnd(uint8_t *sig, size_t *siglen, const uint8_t *m,
                              size_t mlen, const uint8_t *sk,
                              const uint8_t rnd[]);

/* unpack_sig is in packing.c (rANS sigDecode); gives z1[Z1LEN], h[EM]. */
int unpack_sig(uint8_t seedC[CHALLENGESEEDBYTES], poly z1[Z1LEN], poly h[EM],
               const uint8_t *sig);

static uint64_t XS = 0x9E3779B97F4A7C15ULL;
static uint64_t xs(void)
{
    uint64_t x = XS;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    XS = x;
    return x;
}
static void fill(uint8_t *p, size_t n)
{
    for (size_t i = 0; i < n; ++i)
        p[i] = (uint8_t)xs();
}

int main(int argc, char **argv)
{
    long NSIG = (argc > 1) ? atol(argv[1]) : 2000;
    XS ^= (uint64_t)SHUTTLE_MODE;

    uint8_t *pk = malloc(CRYPTO_PUBLICKEYBYTES);
    uint8_t *sk = malloc(CRYPTO_SECRETKEYBYTES);
    uint8_t *sig = malloc(CRYPTO_BYTES);
    uint8_t xi[SEEDBYTES], rnd[SEEDBYTES], msg[64];

    printf("MODE %d RESERVED %d B0 %d BS %d NSIG %ld N %d ELL %d EM %d\n",
           (int)SHUTTLE_MODE, (int)RANS_RESERVED_BYTES, (int)RANS_B0,
           (int)RANS_BS, NSIG, (int)N, (int)ELL, (int)EM);

    /* fixed key; many messages (the response law is per-signature). */
    fill(xi, sizeof xi);
    if (crypto_sign_keypair_xi(pk, sk, xi) != 0) {
        printf("KEYGEN_FAIL\n");
        return 1;
    }

    for (long it = 0; it < NSIG; ++it) {
        size_t slen = 0;
        fill(rnd, sizeof rnd);
        fill(msg, sizeof msg);
        if (crypto_sign_signature_rnd(sig, &slen, msg, sizeof msg, sk, rnd)
            != 0)
            continue;
        /* rlen field at offset CHALLENGESEEDBYTES (uint16 LE). */
        size_t off = CHALLENGESEEDBYTES;
        size_t rlen = (size_t)sig[off] | ((size_t)sig[off + 1] << 8);
        printf("RLEN %zu\n", rlen);

        /* recover the symbols by decoding the signature. */
        uint8_t seedC[CHALLENGESEEDBYTES];
        poly z1[Z1LEN], h[EM];
        if (unpack_sig(seedC, z1, h, sig) != 0) {
            printf("UNPACK_FAIL\n");
            continue;
        }
        /* Q0 = z1[0] >> b0 ; Qs = z1[1..ELL] >> bs ; h = h[0..EM-1]. */
        printf("SYMS Q0");
        for (int k = 0; k < N; ++k)
            printf(" %d", (int)(z1[0].coeffs[k] >> RANS_B0));
        printf(" QS");
        for (int i = 0; i < ELL; ++i)
            for (int k = 0; k < N; ++k)
                printf(" %d", (int)(z1[i + 1].coeffs[k] >> RANS_BS));
        printf(" H");
        for (int i = 0; i < EM; ++i)
            for (int k = 0; k < N; ++k)
                printf(" %d", (int)h[i].coeffs[k]);
        printf("\n");
    }
    free(pk);
    free(sk);
    free(sig);
    return 0;
}
