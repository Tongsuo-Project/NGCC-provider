/*
 * vole.c — subfield-VOLE commit/reconstruct for GALAS.
 *
 * Faithful port of RefCodes/ref-FAEST/faest_128f/vole.c. The only substitution
 * is prg(seed, iv, tweak, out, lambda, nbytes) -> an xof_oneshot over
 * XOF_DOMAIN_TREE_PRG (NGCC pseudoXOF). Tree geometry comes from the GALAS
 * per-tree BAVC layout: tree i has galas_bavc_leaves(i, ps) leaf seeds.
 */
#include "vole.h"
#include "xof.h"
#include <stdlib.h>
#include <string.h>

static const uint32_t TWEAK_OFFSET = 0x80000000u;

/* SHAKE PRG used by the optimized Galas small-VOLE code.  It emits one
   block128 at a time from seed || iv || tweak || uint32 counter. */
static void prg(const uint8_t* seed, const uint8_t* iv, uint32_t tweak,
                uint8_t* out, unsigned lambda, unsigned nbytes) {
    uint8_t block[16];
    unsigned done = 0;
    uint32_t counter = 0;
    while (done < nbytes) {
        xof_ctx ctx;
        xof_init(&ctx, lambda, XOF_DOMAIN_TREE_PRG);
        xof_update(&ctx, seed, lambda / 8);
        xof_update(&ctx, iv, GALAS_IV_SIZE);
        xof_update_u32(&ctx, tweak);
        xof_update_u32(&ctx, counter);
        xof_final(&ctx);
        xof_squeeze(&ctx, block, sizeof(block));
        xof_clear(&ctx);

        unsigned take = nbytes - done;
        if (take > sizeof(block)) take = sizeof(block);
        memcpy(out + done, block, take);
        done += take;
        ++counter;
    }
}

static void xor_bytes(uint8_t* dst, const uint8_t* a, const uint8_t* b, size_t n) {
    for (size_t i = 0; i < n; ++i) dst[i] = a[i] ^ b[i];
}

int galas_convert_to_vole(const uint8_t* iv, const uint8_t* sd, int sd0_bot,
                          unsigned i, unsigned ellhat_bytes,
                          uint8_t* u, uint8_t* v,
                          const galas_paramset_t* ps) {
    unsigned lambda = ps->p.lambda;
    unsigned lb = lambda / 8;
    unsigned Ni = galas_bavc_leaves(i, ps);
    unsigned depth = galas_bavc_depth(i, ps);

    /* r: 2 x Ni rows of ellhat_bytes (two rows reused) */
    uint8_t* r = calloc((size_t)2 * Ni, ellhat_bytes);
    #define R(row, col) (r + (((row) & 1) * Ni + (col)) * ellhat_bytes)
    #define V(idx)      (v + (size_t)(idx) * ellhat_bytes)

    uint32_t tweak = i ^ TWEAK_OFFSET;

    /* expand each seed into an ellhat_bytes PRG output */
    if (!sd0_bot) {
        prg(sd, iv, tweak, R(0, 0), lambda, ellhat_bytes);
    }
    for (unsigned j = 1; j < Ni; ++j) {
        prg(sd + (size_t)j * lb, iv, tweak, R(0, j), lambda, ellhat_bytes);
    }

    /* fold the Ni leaves into `depth` v-rows via XOR reduction */
    memset(v, 0, (size_t)depth * ellhat_bytes);
    for (unsigned j = 0; j < depth; ++j) {
        unsigned depthloop = Ni >> (j + 1);
        for (unsigned idx = 0; idx < depthloop; ++idx) {
            xor_bytes(V(j), R(j, 2 * idx + 1), V(j), ellhat_bytes);
            xor_bytes(R(j + 1, idx), R(j, 2 * idx), R(j, 2 * idx + 1), ellhat_bytes);
        }
    }
    if (!sd0_bot && u != NULL) {
        memcpy(u, R(depth, 0), ellhat_bytes);
    }
    free(r);
    return (int)depth;
}

void galas_vole_commit(const uint8_t* rootKey, const uint8_t* iv, unsigned ellhat,
                       const galas_paramset_t* ps, galas_bavc_t* vc,
                       uint8_t* c, uint8_t* u, uint8_t** v) {
    unsigned lb = ps->p.lambda / 8;
    unsigned ellhat_bytes = (ellhat + 7) / 8;
    unsigned tau = ps->p.tau;

    galas_bavc_commit(rootKey, iv, ps, vc);

    uint8_t* ui = malloc((size_t)tau * ellhat_bytes);

    unsigned v_idx = 0;
    uint8_t* sd_i = vc->sd;
    for (unsigned i = 0; i < tau; ++i) {
        unsigned Ni = galas_bavc_leaves(i, ps);
        v_idx += (unsigned)galas_convert_to_vole(iv, sd_i, 0, i, ellhat_bytes,
                                                 ui + (size_t)i * ellhat_bytes,
                                                 v[v_idx], ps);
        sd_i += (size_t)Ni * lb;
    }
    /* 0-pad up to lambda v-rows */
    for (; v_idx < ps->p.lambda; ++v_idx) {
        memset(v[v_idx], 0, ellhat_bytes);
    }
    /* u = u_0 ; c[i-1] = u_0 XOR u_i  (the correction strings) */
    memcpy(u, ui, ellhat_bytes);
    for (unsigned i = 1; i < tau; ++i) {
        xor_bytes(c + (size_t)(i - 1) * ellhat_bytes, u, ui + (size_t)i * ellhat_bytes, ellhat_bytes);
    }
    free(ui);
}

bool galas_vole_reconstruct(uint8_t* com, uint8_t** q, const uint8_t* iv,
                            const uint16_t* i_delta, const uint8_t* decom,
                            const uint8_t* c, unsigned ellhat,
                            const galas_paramset_t* ps) {
    unsigned lb = ps->p.lambda / 8;
    unsigned ellhat_bytes = (ellhat + 7) / 8;
    unsigned tau = ps->p.tau;

    galas_bavc_rec_t rec;
    rec.h = com;
    rec.s = NULL;   /* bavc_reconstruct allocates rec->s internally */

    if (!galas_bavc_reconstruct(decom, i_delta, iv, ps, &rec)) {
        free(rec.s);
        return false;
    }

    /* max tree leaves */
    unsigned maxNi = 0;
    for (unsigned i = 0; i < tau; ++i) {
        unsigned Ni = galas_bavc_leaves(i, ps);
        if (Ni > maxNi) maxNi = Ni;
    }
    uint8_t* sd   = malloc((size_t)maxNi * lb);
    uint8_t* qtmp = malloc((size_t)galas_bavc_depth(0, ps) * ellhat_bytes);
    /* ensure qtmp big enough for any tree's depth */
    size_t qtmp_cap = 0;
    for (unsigned i = 0; i < tau; ++i) {
        size_t d = galas_bavc_depth(i, ps);
        if (d > qtmp_cap) qtmp_cap = d;
    }
    free(qtmp);
    qtmp = malloc(qtmp_cap * ellhat_bytes);

    unsigned q_idx = 0;
    uint8_t* sd_i = rec.s;
    for (unsigned i = 0; i < tau; ++i) {
        unsigned Ni = galas_bavc_leaves(i, ps);
        /* reconstruct the full sd[] (Ni seeds) with the challenged one missing;
           FAEST places them at j ^ i_delta[i] for j != i_delta[i]. */
        for (unsigned j = 0; j < Ni; ++j) {
            if (j < i_delta[i])
                memcpy(sd + (size_t)(j ^ i_delta[i]) * lb, sd_i + (size_t)j * lb, lb);
            else if (j == i_delta[i])
                continue;
            else
                memcpy(sd + (size_t)(j ^ i_delta[i]) * lb, sd_i + (size_t)(j - 1) * lb, lb);
        }
        int ki = galas_convert_to_vole(iv, sd, /*sd0_bot=*/1, i, ellhat_bytes,
                                       NULL, qtmp, ps);
        if (i == 0) {
            memcpy(q[q_idx], qtmp, (size_t)ellhat_bytes * ki);
            q_idx += ki;
        } else {
            for (int d = 0; d < ki; ++d, ++q_idx) {
                /* masked XOR with c[i-1] when bit d of i_delta[i] is set */
                int mask = (i_delta[i] >> d) & 1;
                uint8_t* qrow = qtmp + (size_t)d * ellhat_bytes;
                uint8_t* corr = (uint8_t*)c + (size_t)(i - 1) * ellhat_bytes;
                for (size_t b = 0; b < ellhat_bytes; ++b)
                    q[q_idx][b] = qrow[b] ^ (mask ? corr[b] : 0);
            }
        }
        sd_i += (size_t)(Ni - 1) * lb;
    }
    for (; q_idx < ps->p.lambda; ++q_idx) {
        memset(q[q_idx], 0, ellhat_bytes);
    }
    free(qtmp); free(sd); free(rec.s);
    return true;
}
