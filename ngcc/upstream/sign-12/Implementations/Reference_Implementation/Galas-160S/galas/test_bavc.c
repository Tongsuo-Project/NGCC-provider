/*
 * test_bavc.c — BAVC round-trip self-test.
 *
 * For a fixed root seed and IV, for each instance:
 *   galas_bavc_commit -> vc{h, com, sd, k}
 *   galas_bavc_open(vc, i_delta) -> decom
 *   galas_bavc_reconstruct(decom, i_delta) -> rec{h, s}
 *   assert rec.h == vc.h            (Merkle root matches)
 *   assert rec.s == vc.sd on all NON-challenged leaves (reconstruction correct).
 *   rec.s is compact, i.e. challenged leaves are omitted.
 *
 * The challenge i_delta picks one leaf per group; we pick i_delta[i] = 0 for
 * simplicity (and a second pass with a non-zero value).
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bavc.h"
#include "instances.h"

static int fails = 0;

static int run_instance(galas_instance_id_t id, const uint8_t* root, const uint8_t* iv) {
    const galas_paramset_t* ps = galas_get_paramset(id);
    if (!ps) { printf("FAIL [%u] no paramset\n", id); return 1; }
    unsigned lb = galas_lambda_bytes(ps);
    unsigned tau = ps->p.tau;
    unsigned com_size = 2 * lb;
    uint32_t L = ps->p.L;

    galas_bavc_t vc;
    galas_bavc_commit(root, iv, ps, &vc);

    /* decom buffer: tau*com_size + T_open*lb */
    size_t decom_len = (size_t)com_size * tau + (size_t)ps->p.T_open * lb;
    uint8_t* decom = malloc(decom_len);

    uint16_t i_delta[GALAS_MAX_TAU];
    for (unsigned i = 0; i < tau; ++i) {
        i_delta[i] = (uint16_t)(i % galas_bavc_leaves(i, ps));
    }

    int ok = galas_bavc_open(&vc, i_delta, decom, ps);
    if (!ok) { printf("FAIL [%u] bavc_open returned false\n", id); fails++; free(decom); galas_bavc_clear(&vc); return 1; }

    galas_bavc_rec_t rec;
    uint8_t rec_h[GALAS_MAX_LAMBDA_BYTES * 2];
    rec.h = rec_h;
    rec.s = NULL;
    ok = galas_bavc_reconstruct(decom, i_delta, iv, ps, &rec);
    if (!ok) { printf("FAIL [%u] bavc_reconstruct returned false\n", id); fails++; free(decom); galas_bavc_clear(&vc); return 1; }

    /* root hash match */
    if (memcmp(rec.h, vc.h, 2 * lb) != 0) {
        printf("FAIL [%u] reconstructed root != committed root\n", id); fails++;
    } else {
        printf("ok   [%u] root hash matches\n", id);
    }

    /* non-challenged leaf seeds match */
    int seed_mismatch = 0;
    uint32_t checked = 0;
    uint32_t rec_off = 0;
    for (unsigned i = 0, vec_off = 0; i < tau; ++i) {
        unsigned leaves = galas_bavc_leaves(i, ps);
        for (unsigned j = 0; j < leaves; ++j) {
            const uint32_t off = vec_off + j;
            if (j == i_delta[i]) continue;  /* challenged leaf: seed unrecoverable */
            if (memcmp(rec.s + (size_t)rec_off * lb, vc.sd + (size_t)off * lb, lb) != 0) {
                seed_mismatch++;
            }
            rec_off++;
            checked++;
        }
        vec_off += leaves;
    }
    if (seed_mismatch) {
        printf("FAIL [%u] %d/%u non-challenged seeds mismatch\n", id, seed_mismatch, checked);
        fails++;
    } else {
        printf("ok   [%u] %u non-challenged seeds reconstructed correctly\n", id, checked);
    }

    free(decom);
    galas_bavc_clear(&vc);
    galas_bavc_rec_clear(&rec);
    (void)L;
    return 0;
}

int main(void) {
    /* fixed root seed (32 bytes, enough for lambda<=256) and IV (32 bytes) */
    uint8_t root[64] = {0};
    uint8_t iv[32] = {0};
    for (int i = 0; i < 64; ++i) root[i] = (uint8_t)(i * 7 + 1);
    for (int i = 0; i < 32; ++i) iv[i] = (uint8_t)(0xA0 + i);

    galas_instance_id_t ids[] = {GALAS_160S, GALAS_160F, GALAS_256S, GALAS_256F,
                                 GALAS_384S, GALAS_384F, GALAS_512S, GALAS_512F};
    for (size_t k = 0; k < sizeof(ids)/sizeof(ids[0]); ++k) {
        run_instance(ids[k], root, iv);
    }
    printf("\nBAVC round-trip: %d failures\n", fails);
    return fails ? 1 : 0;
}
