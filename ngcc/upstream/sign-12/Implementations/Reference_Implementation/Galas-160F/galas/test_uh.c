/* test_uh.c — smoke test for universal_hashing (vole_hash / zk_hash / leaf_hash).
 *
 * These FAEST hash functions use a 2*lambda extension ring whose reduction
 * FAEST hard-codes via CLMUL; without an independent Python reference for that
 * exact ring, we verify the properties that must hold regardless:
 *   - determinism (same input -> same output)
 *   - domain sensitivity (different input -> different output)
 *   - runs without memory errors (ASan)
 * A byte-exact cross-check against FAEST's C reference will be added once we
 * link against it for a single lambda (256).
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "universal_hashing.h"
#include "bf.h"

static int fails = 0;
#define CHECK(c, m) do { if(!(c)){ printf("FAIL [%s]\n", m); fails++; } else { printf("ok   [%s]\n", m);} } while(0)

static void rand_bytes(uint8_t* b, size_t n, uint8_t seed) {
    for (size_t i = 0; i < n; ++i) b[i] = (uint8_t)(seed * 31 + i * 7 + 1);
}

int main(void) {
    unsigned lambdas[] = {160, 256, 384, 512};
    for (size_t li = 0; li < 4; ++li) {
        unsigned lambda = lambdas[li];
        unsigned lb = lambda / 8;
        char label[64];

        /* ---- leaf_hash ---- */
        uint8_t u[128] = {0}, x0[64] = {0}, x1[64] = {0};
        rand_bytes(u, 2 * lb, 1); rand_bytes(x0, lb, 2); rand_bytes(x1, lb, 3);
        uint8_t h1[128], h2[128];
        galas_leaf_hash(h1, u, x0, x1, lambda);
        galas_leaf_hash(h2, u, x0, x1, lambda);
        snprintf(label, sizeof(label), "leaf_hash n=%u determinism", lambda);
        CHECK(memcmp(h1, h2, 2 * lb) == 0, label);
        x0[0] ^= 1;
        galas_leaf_hash(h2, u, x0, x1, lambda);
        snprintf(label, sizeof(label), "leaf_hash n=%u sensitivity", lambda);
        CHECK(memcmp(h1, h2, 2 * lb) != 0, label);

        /* ---- vole_hash ----
           FAEST x buffer layout: witness (ell bits) || x0-region (2*lambda) ||
           x1-region (lambda + B bits) that the hash reads. Total =
           (ell + 3*lambda + B) bits. */
        unsigned ell = 5 * lambda;
        size_t xb = (ell + 3 * lambda + GALAS_UNIVERSAL_HASH_B_BITS) / 8;
        uint8_t* x = malloc(xb);
        uint8_t sd[6 * 64];
        rand_bytes(x, xb, 4); rand_bytes(sd, 6 * lb, 5);
        uint8_t vh1[128], vh2[128];
        galas_vole_hash(vh1, sd, x, ell, lambda);
        galas_vole_hash(vh2, sd, x, ell, lambda);
        snprintf(label, sizeof(label), "vole_hash n=%u determinism", lambda);
        CHECK(memcmp(vh1, vh2, lb + GALAS_UNIVERSAL_HASH_B) == 0, label);
        x[0] ^= 1;
        galas_vole_hash(vh2, sd, x, ell, lambda);
        snprintf(label, sizeof(label), "vole_hash n=%u sensitivity", lambda);
        CHECK(memcmp(vh1, vh2, lb + GALAS_UNIVERSAL_HASH_B) != 0, label);
        free(x);

        /* ---- zk_hash ---- */
        galas_zk_hash_ctx zc;
        uint8_t zsd[6 * 64]; rand_bytes(zsd, 6 * lb, 6);
        galas_zk_hash_init(&zc, lambda, zsd);
        const gf_ctx* fc = (lambda == 160) ? bf_ctx_160() : (lambda == 256) ? bf_ctx_256() :
                            (lambda == 384) ? bf_ctx_384() : bf_ctx_512();
        gf_limb_t v[GF_LIMBS(512)];
        for (unsigned i = 0; i < 5; ++i) {
            memset(v, 0, sizeof(v)); v[0] = i + 1;
            galas_zk_hash_update(&zc, v);
        }
        gf_limb_t zx1[GF_LIMBS(512)]; memset(zx1, 0, sizeof(zx1)); zx1[0] = 0xABCDEF;
        uint8_t zh[64]; galas_zk_hash_final(zh, &zc, zx1);
        snprintf(label, sizeof(label), "zk_hash n=%u ran", lambda);
        CHECK(zh[0] != 0 || zh[1] != 0, label);   /* non-trivial output */
        (void)fc;
    }
    printf("\nuniversal_hashing smoke: %d failures\n", fails);
    return fails ? 1 : 0;
}
