/*
 * test_vole.c — VOLE commit/reconstruct round-trip.
 *
 * Commits a VOLE (producing u, v[lambda], c, vc), then simulates the verifier
 * by running reconstruct with a chosen i_delta and checking that the
 * reconstructed q rows equal the prover's v rows for the NON-challenged tree
 * levels. (The VOLE correlation is q = v + Delta*u; with Delta=0 for this
 * structural test, q should equal v.)
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vole.h"
#include "instances.h"

static int fails = 0;

static int run(galas_instance_id_t id, const uint8_t* rootKey, const uint8_t* iv) {
    const galas_paramset_t* ps = galas_get_paramset(id);
    if (!ps) { printf("FAIL [%u] no paramset\n", id); return 1; }
    unsigned lambda = ps->p.lambda;
    unsigned lb = lambda / 8;
    unsigned tau = ps->p.tau;
    unsigned ellhat = ps->p.lambda;   /* witness bit-length per row (simplified) */
    unsigned eb = (ellhat + 7) / 8;

    /* prover side. v is a contiguous lambda*eb buffer; v_rows[i] points into it. */
    galas_bavc_t vc;
    uint8_t* c = malloc((size_t)(tau > 0 ? (tau - 1) : 1) * eb);
    uint8_t* u = malloc(eb);
    uint8_t* vbuf = malloc((size_t)lambda * eb);
    uint8_t** v = malloc(lambda * sizeof(uint8_t*));
    for (unsigned i = 0; i < lambda; ++i) v[i] = vbuf + (size_t)i * eb;

    galas_vole_commit(rootKey, iv, ellhat, ps, &vc, c, u, v);

    /* pick a challenge i_delta */
    uint16_t i_delta[GALAS_MAX_TAU];
    for (unsigned i = 0; i < tau; ++i) i_delta[i] = (uint16_t)(i % galas_bavc_leaves(i, ps));

    /* decommitment (BAVC open) */
    unsigned com_size = 2 * lb;
    size_t decom_len = (size_t)com_size * tau + (size_t)ps->p.T_open * lb;
    uint8_t* decom = malloc(decom_len);
    if (!galas_bavc_open(&vc, i_delta, decom, ps)) {
        printf("FAIL [%u] bavc_open\n", (unsigned)id); fails++;
        goto cleanup;
    }

    /* verifier reconstruct. q is contiguous too. */
    uint8_t* qbuf = malloc((size_t)lambda * eb);
    uint8_t** q = malloc(lambda * sizeof(uint8_t*));
    for (unsigned i = 0; i < lambda; ++i) q[i] = qbuf + (size_t)i * eb;
    uint8_t com_root[GALAS_MAX_LAMBDA_BYTES * 2];
    memcpy(com_root, vc.h, 2 * lb);   /* verifier knows the committed root */
    if (!galas_vole_reconstruct(com_root, q, iv, i_delta, decom, c, ellhat, ps)) {
        printf("FAIL [%u] vole_reconstruct\n", (unsigned)id); fails++;
        free(q); free(qbuf);
        goto cleanup;
    }

    unsigned row = 0;
    unsigned mismatches = 0;
    for (unsigned i = 0; i < tau; ++i) {
        unsigned depth = galas_bavc_depth(i, ps);
        for (unsigned d = 0; d < depth; ++d, ++row) {
            int bad = 0;
            for (unsigned b = 0; b < eb; ++b) {
                uint8_t expected = v[row][b] ^ (((i_delta[i] >> d) & 1u) ? u[b] : 0);
                if (q[row][b] != expected) bad = 1;
            }
            mismatches += bad;
        }
    }
    if (mismatches) {
        printf("FAIL [%u] %u VOLE rows violate q=v+Delta*u\n", (unsigned)id, mismatches);
        fails++;
    } else {
        printf("ok   [%u] VOLE relation q=v+Delta*u holds\n", (unsigned)id);
    }

    free(q); free(qbuf);
cleanup:
    free(decom); free(c); free(u);
    free(v); free(vbuf);
    galas_bavc_clear(&vc);
    return 0;
}

int main(void) {
    uint8_t root[64], iv[32];
    for (int i = 0; i < 64; ++i) root[i] = (uint8_t)(i * 7 + 1);
    for (int i = 0; i < 32; ++i) iv[i] = (uint8_t)(0xA0 + i);

    galas_instance_id_t ids[] = {GALAS_160S, GALAS_160F, GALAS_256S, GALAS_256F,
                                 GALAS_384S, GALAS_384F, GALAS_512S, GALAS_512F};
    for (size_t k = 0; k < sizeof(ids)/sizeof(ids[0]); ++k) run(ids[k], root, iv);
    printf("\nVOLE round-trip: %d failures\n", fails);
    return fails ? 1 : 0;
}
