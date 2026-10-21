/* test_bf.c — verify the bf layer against Python vectors.
   - bf_mul (reduced) matches gf_mul output
   - bf_mul_unreduced matches Python a*b polynomial product */
#include <stdio.h>
#include <string.h>
#include "bf.h"
#include "bf_vectors.h"

static const gf_ctx* ctx_for(unsigned n) {
    switch (n) {
        case 64: return bf_ctx_64();
        case 160: return bf_ctx_160();
        case 256: return bf_ctx_256();
        case 384: return bf_ctx_384();
        case 512: return bf_ctx_512();
        default: return NULL;
    }
}

static int fails = 0;

int main(void) {
    for (size_t v = 0; v < BFV_LEN; ++v) {
        unsigned n = BFV[v].n;
        unsigned nb = n / 8;
        const gf_ctx* c = ctx_for(n);
        if (!c) { printf("FAIL [n=%u no ctx]\n", n); fails++; continue; }
        gf_limb_t a[GF_LIMBS(512)], b[GF_LIMBS(512)], red[GF_LIMBS(512)];
        gf_limb_t unred[GF_LIMBS(1024)];
        gf_from_bytes(c, a, BFV[v].a);
        gf_from_bytes(c, b, BFV[v].b);

        /* reduced mul */
        bf_mul(c, red, a, b);
        uint8_t out[128];
        gf_to_bytes(c, out, red);
        if (memcmp(out, BFV[v].red, nb) != 0) {
            printf("FAIL [n=%u bf_mul reduced mismatch]\n", n); fails++;
        } else printf("ok   [n=%u bf_mul reduced]\n", n);

        /* unreduced mul */
        bf_mul_unreduced(unred, a, b, n);
        uint8_t uo[256] = {0};
        for (unsigned i = 0; i < (2 * n + 63) / 64; ++i)
            for (unsigned j = 0; j < 8; ++j)
                uo[i * 8 + j] = (uint8_t)(unred[i] >> (8 * j));
        if (memcmp(uo, BFV[v].unred, 2 * nb) != 0) {
            printf("FAIL [n=%u bf_mul_unreduced mismatch]\n", n); fails++;
        } else printf("ok   [n=%u bf_mul_unreduced]\n", n);
    }
    printf("\nbf layer: %d failures\n", fails);
    return fails ? 1 : 0;
}
