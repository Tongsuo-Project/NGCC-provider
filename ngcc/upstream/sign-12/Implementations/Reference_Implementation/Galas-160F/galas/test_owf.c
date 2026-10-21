/*
 * test_owf.c — cross-check the C Gala OWF against the Python reference vectors.
 *
 * For each vector in owf_vectors.h (20 vectors, 4 per n in {128,160,256,384,512}),
 * compute Gala_k(x) in C and assert it equals the recorded y byte-for-byte.
 */
#include <stdio.h>
#include <string.h>
#include "gf2n.h"
#include "owf.h"
#include "owf_vectors.h"

static int fails = 0;

int main(void) {
    int total = 0;
    for (size_t v = 0; v < OWFV_LEN; ++v) {
        unsigned n = OWFV[v].n;
        gf_ctx ctx;
        const galas_owf_params* P = galas_owf_params_for(n);
        if (!P || gf_init(&ctx, n) != 0) {
            printf("FAIL [n=%u setup]\n", n); fails++; continue;
        }
        uint8_t y[OWF_MAX_BYTES];
        int rc = galas_owf_eval(P, &ctx, y, OWFV[v].x, OWFV[v].k);
        if (rc != 0) {
            printf("FAIL [n=%u owf rejected input]\n", n); fails++; continue;
        }
        if (memcmp(y, OWFV[v].y, OWFV[v].nb) != 0) {
            printf("FAIL [n=%u output mismatch]\n  C : ", n);
            for (size_t i = 0; i < OWFV[v].nb; ++i) printf("%02X", y[i]);
            printf("\n  Py: ");
            for (size_t i = 0; i < OWFV[v].nb; ++i) printf("%02X", OWFV[v].y[i]);
            printf("\n");
            fails++;
        } else {
            printf("ok   [n=%u OWF vector %zu]\n", n, v);
            total++;
        }
    }
    printf("\nOWF cross-check: %d ok, %d fail\n", total, fails);
    return fails ? 1 : 0;
}
