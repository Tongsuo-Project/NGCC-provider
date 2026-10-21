/*
 * xcheck_dump.c -- C-oracle byte dumper for the SHUTTLE Python reference
 * cross-check (P12, deliverable 2).
 *
 * Emits, on FIXED seeded inputs, tagged hex lines for the deterministic
 * codec + math + DRNG layers, so the Python ref (tools/pyref/*.py) can
 * assert byte-for-byte equality (Python == C, C is the oracle).
 *
 * Build (per mode, DISABLE_NAMESPACE so the plain symbol names resolve):
 *   gcc -O2 -std=c99 -I../../ref -I../../ref/test -I../../tools \
 *       -I../../ref/ntt/<qset> -DDISABLE_NAMESPACE=1 -DSHUTTLE_MODE=<m> \
 *       xcheck_dump.c ../../ref/{reduce,poly,poly_ntt,packing,rans,drng,\
 *       auxfunc}.c ../../ref/ntt/<qset>/ntt_ref.c -o xcheck_dump_<m>
 *
 * Output: lines "TAG <hex>" or "TAG <int>".  A deterministic xorshift RNG
 * (seed = 0xC0FFEE ^ mode) drives all "random" inputs; the Python side
 * mirrors that exact RNG, so both sides see identical inputs.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "params.h"
#include "poly.h"
#include "poly_ntt.h"
#include "packing.h"
#include "reduce.h"
#include "drng.h"
#include "rans.h"

/* ---- deterministic xorshift64 (mirrored in Python xcheck) ---- */
static uint64_t XS;
static uint64_t xs(void)
{
    uint64_t x = XS;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    XS = x;
    return x;
}
static void xs_seed(uint64_t s) { XS = s ? s : 0x123456789abcdef0ULL; }
static uint32_t xs32(void) { return (uint32_t)(xs() & 0xffffffffu); }

static void put_hex(const char *tag, const uint8_t *p, size_t n)
{
    printf("%s ", tag);
    for (size_t i = 0; i < n; ++i)
        printf("%02x", p[i]);
    printf("\n");
}

int main(void)
{
    xs_seed(0xC0FFEEULL ^ (uint64_t)SHUTTLE_MODE);
    printf("MODE %d\n", (int)SHUTTLE_MODE);
    printf("N %d Q %d ELL %d EM %d\n", (int)N, (int)Q, (int)ELL, (int)EM);

    /* ---- (1) DRNG: init from a fixed nonce, draw several blocks ---- */
    {
        uint8_t nonce[64];
        for (int i = 0; i < 64; ++i)
            nonce[i] = (uint8_t)(0x30 + (i % 16)); /* fixed pattern */
        DRNG_ctx d;
        init_random_number(&d, nonce, 64);
        /* a sequence of draws of varying byte length (all whole-byte) */
        static const int draws[] = {16, 32, 33, 64, 80, 100, 1, 7};
        uint8_t buf[256];
        for (unsigned k = 0; k < sizeof draws / sizeof draws[0]; ++k) {
            int nb = draws[k];
            memset(buf, 0, sizeof buf);
            get_random_number(&d, buf, (unsigned long long)nb * 8);
            char tag[32];
            sprintf(tag, "DRNG[%d]", nb);
            put_hex(tag, buf, (size_t)nb);
        }
    }

    /* ---- (2) reduce32 / freeze / reduce_mod_2q on fixed int32 probes ---- */
    {
        int32_t probes[16];
        for (int i = 0; i < 16; ++i)
            probes[i] = (int32_t)xs32();
        for (int i = 0; i < 16; ++i) {
            printf("RED32 %d %d\n", (int)probes[i], (int)reduce32(probes[i]));
            printf("FREEZE %d %d\n", (int)probes[i], (int)freeze(probes[i]));
            printf("RED2Q %d %d\n", (int)probes[i],
                   (int)reduce_mod_2q(probes[i]));
        }
    }

    /* ---- (3) NTT: forward of a fixed [0,q) poly (canonical order) ---- */
    {
        poly16 a;
        for (int k = 0; k < N; ++k)
            a.coeffs[k] = (uint16_t)(xs32() % (uint32_t)Q);
        /* dump input then forward-NTT output (both uint16 LE). */
        uint8_t in[2 * N], out[2 * N];
        for (int k = 0; k < N; ++k) {
            in[2 * k] = (uint8_t)(a.coeffs[k] & 0xff);
            in[2 * k + 1] = (uint8_t)(a.coeffs[k] >> 8);
        }
        put_hex("NTTIN", in, 2 * N);
        poly16 af = a;
        poly_ntt_canonical(&af);
        for (int k = 0; k < N; ++k) {
            out[2 * k] = (uint8_t)(af.coeffs[k] & 0xff);
            out[2 * k + 1] = (uint8_t)(af.coeffs[k] >> 8);
        }
        put_hex("NTTOUT", out, 2 * N);
        /* invntt_tomont of the forward output (== a*R mod q) */
        poly16 ai = af;
        poly_invntt_tomont(&ai);
        for (int k = 0; k < N; ++k) {
            out[2 * k] = (uint8_t)(ai.coeffs[k] & 0xff);
            out[2 * k + 1] = (uint8_t)(ai.coeffs[k] >> 8);
        }
        put_hex("INVNTT", out, 2 * N);
    }

    /* ---- (4) packing: pack_pk / pack_sk / pack_com on fixed inputs ---- */
    {
        uint8_t seedA[SEEDBYTES];
        for (int i = 0; i < SEEDBYTES; ++i)
            seedA[i] = (uint8_t)xs32();
        poly b[EM];
        for (int i = 0; i < EM; ++i)
            for (int k = 0; k < N; ++k)
                b[i].coeffs[k] =
                    (int32_t)((xs32() % (uint32_t)(((Q) + ALPHA_B - 1) / ALPHA_B))
                              * ALPHA_B);
        uint8_t *pk = malloc(CRYPTO_PUBLICKEYBYTES);
        pack_pk(pk, seedA, b);
        put_hex("PK", pk, CRYPTO_PUBLICKEYBYTES);

        uint8_t masterSeed[CHALLENGESEEDBYTES], tr[CHALLENGESEEDBYTES];
        for (int i = 0; i < CHALLENGESEEDBYTES; ++i) {
            masterSeed[i] = (uint8_t)xs32();
            tr[i] = (uint8_t)xs32();
        }
        poly s[ELL], ep[EM];
        for (int i = 0; i < ELL; ++i)
            for (int k = 0; k < N; ++k)
                s[i].coeffs[k] =
                    (int32_t)(xs32() % (2 * BS_ENC + 1)) - BS_ENC;
        for (int i = 0; i < EM; ++i)
            for (int k = 0; k < N; ++k)
                ep[i].coeffs[k] =
                    (int32_t)(xs32() % (2 * BE_ENC + 1)) - BE_ENC;
        uint8_t *sk = malloc(CRYPTO_SECRETKEYBYTES);
        pack_sk(sk, seedA, b, masterSeed, tr, s, ep);
        put_hex("SK", sk, CRYPTO_SECRETKEYBYTES);

        /* EncodeCom single-poly: comY_h in [0,H_h), comY_0 in {0,1} */
        poly comH, com0;
        for (int k = 0; k < N; ++k) {
            comH.coeffs[k] = (int32_t)(xs32() % (uint32_t)HH);
            com0.coeffs[k] = (int32_t)(xs32() & 1u);
        }
        uint8_t com[ENCODECOM_BYTES];
        pack_com(com, &comH, &com0);
        put_hex("COM", com, ENCODECOM_BYTES);

        free(pk);
        free(sk);
    }

    return 0;
}
