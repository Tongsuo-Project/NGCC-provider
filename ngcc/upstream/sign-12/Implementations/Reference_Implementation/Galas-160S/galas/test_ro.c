/* test_ro.c — Fiat-Shamir oracle determinism + domain-separation check. */
#include <stdio.h>
#include <string.h>
#include "random_oracle.h"

static int fails = 0;
#define CHECK(c,m) do{ if(!(c)){printf("FAIL [%s]\n",m);fails++;}else printf("ok   [%s]\n",m);}while(0)

int main(void) {
    unsigned lambda = 256;
    uint8_t in[64]; for (int i = 0; i < 64; ++i) in[i] = (uint8_t)(i + 1);

    /* H2 determinism */
    galas_H2_ctx a, b;
    uint8_t d0a[32], d0b[32];
    galas_H2_init(&a, lambda); galas_H2_update(&a, in, 64); galas_H2_0_final(&a, d0a, 32);
    galas_H2_init(&b, lambda); galas_H2_update(&b, in, 64); galas_H2_0_final(&b, d0b, 32);
    CHECK(memcmp(d0a, d0b, 32) == 0, "H2_0 determinism");

    /* H2_0 vs H2_1 differ (domain sep) */
    uint8_t d1[32];
    galas_H2_init(&a, lambda); galas_H2_update(&a, in, 64); galas_H2_1_final(&a, d1, 32);
    CHECK(memcmp(d0a, d1, 32) != 0, "H2_0 != H2_1 (domain separation)");

    /* H3 produces digest + iv */
    uint8_t dg[64], iv[GALAS_IV_SIZE];
    galas_H3_init(&a, lambda); galas_H3_update(&a, in, 64);
    galas_H3_final(&a, dg, 64, iv);
    int nz = 0; for (int i = 0; i < 64; ++i) nz |= dg[i];
    CHECK(nz, "H3 non-trivial output");

    /* H1 determinism */
    uint8_t h1a[32], h1b[32];
    galas_H1_init(&a, lambda); galas_H1_update(&a, in, 64); galas_H1_final(&a, h1a, 32);
    galas_H1_init(&b, lambda); galas_H1_update(&b, in, 64); galas_H1_final(&b, h1b, 32);
    CHECK(memcmp(h1a, h1b, 32) == 0, "H1 determinism");

    printf("\nrandom_oracle: %d failures\n", fails);
    return fails ? 1 : 0;
}
