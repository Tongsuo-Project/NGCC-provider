/*
 * bavc.c - GALAS BAVC using the FAEST reference one-tree layout.
 *
 * The commitment tree has L leaves and 2L-1 nodes.  The tau VOLE vectors are
 * interleaved in the leaf layer exactly as in RefCodes/ref-FAEST:
 *
 *   pos(i,j) = L-1 + tau*j + i                         for short-vector range
 *   pos(i,j) = L-1 + tau*2^(k-1) + tau1*(j mod 2^(k-1)) + i
 *                                                        for long-vector tail
 *
 * GALAS uses FAEST's SHAKE leaf hash: a PRG output is split as
 * sd || com, where sd has lambda bits and com has 2*lambda bits.
 * Symmetric calls are routed through the NGCC-backed xof_* layer.
 */
#include "bavc.h"
#include "xof.h"
#include <stdlib.h>
#include <string.h>

#define NODE(nodes, slot, lb) (&(nodes)[(size_t)(slot) * (lb)])

static void h1_update_domain(xof_ctx* ctx) {
    const uint8_t ds = 1;
    xof_update(ctx, &ds, sizeof(ds));
}

static inline void set_bit(uint8_t* s, uint32_t i) {
    s[i >> 3] |= (uint8_t)(1u << (i & 7));
}

static inline void clear_bit(uint8_t* s, uint32_t i) {
    s[i >> 3] &= (uint8_t)~(1u << (i & 7));
}

static inline int get_bit(const uint8_t* s, uint32_t i) {
    return (s[i >> 3] >> (i & 7)) & 1u;
}

static inline uint32_t pos_in_tree(unsigned i, unsigned j,
                                   const galas_paramset_t* ps) {
    const uint32_t short_len = 1u << (ps->p.k - 1);
    if (j < short_len) {
        return ps->p.L - 1u + (uint32_t)ps->p.tau * j + i;
    }
    const uint32_t mask = short_len - 1u;
    return ps->p.L - 1u + (uint32_t)ps->p.tau * short_len +
           (uint32_t)ps->p.tau1 * (j & mask) + i;
}

static void tree_prg(uint8_t* nodes, uint32_t alpha,
                     const uint8_t* iv, unsigned lambda, unsigned lb) {
    uint8_t* out = NODE(nodes, 2u * alpha + 1u, lb);
    const unsigned total = 2u * lb;
    const unsigned block_bytes = 16u;
    const unsigned blocks = (total + block_bytes - 1u) / block_bytes;
    unsigned first_blocks = 4u - (blocks & 1u);
    if (first_blocks > blocks) first_blocks = blocks;

    unsigned done = 0;
    for (unsigned counter = 0; done < total;) {
        unsigned nblocks = (counter == 0) ? first_blocks : 2u;
        if (nblocks > blocks - counter) nblocks = blocks - counter;

        uint8_t block[64];
        xof_ctx ctx;
        xof_init(&ctx, lambda, XOF_DOMAIN_TREE_PRG);
        xof_update(&ctx, NODE(nodes, alpha, lb), lb);
        xof_update(&ctx, iv, GALAS_IV_SIZE);
        xof_update_u32(&ctx, alpha);
        xof_update_u32(&ctx, counter);
        xof_final(&ctx);
        xof_squeeze(&ctx, block, (size_t)nblocks * block_bytes);
        xof_clear(&ctx);

        unsigned take = total - done;
        unsigned produced = nblocks * block_bytes;
        if (take > produced) take = produced;
        memcpy(out + done, block, take);
        done += take;
        counter += nblocks;
    }
}

static uint8_t* generate_seeds(const uint8_t* rootKey, const uint8_t* iv,
                               const galas_paramset_t* ps) {
    const unsigned lb = galas_lambda_bytes(ps);
    uint8_t* nodes = calloc((size_t)2 * ps->p.L - 1u, lb);
    if (!nodes) return NULL;

    memcpy(NODE(nodes, 0, lb), rootKey, lb);
    for (uint32_t alpha = 0; alpha < ps->p.L - 1u; ++alpha) {
        tree_prg(nodes, alpha, iv, ps->p.lambda, lb);
    }
    return nodes;
}

static void leaf_commit(uint8_t* sd, uint8_t* com, const uint8_t* key,
                        const uint8_t* iv, uint32_t tweak,
                        unsigned lambda, unsigned lb) {
    uint8_t out[3 * GALAS_MAX_LAMBDA_BYTES];

    xof_ctx ctx;
    xof_init(&ctx, lambda, XOF_DOMAIN_LEAF);
    xof_update(&ctx, key, lb);
    xof_update(&ctx, iv, GALAS_IV_SIZE);
    xof_update_u32(&ctx, tweak);
    {
        const uint8_t ctr0 = 0;
        xof_update(&ctx, &ctr0, sizeof(ctr0));
    }
    xof_final(&ctx);
    xof_squeeze(&ctx, out, (size_t)3 * lb);
    xof_clear(&ctx);

    memcpy(sd, out, lb);
    memcpy(com, out + lb, (size_t)2 * lb);
}

void galas_bavc_commit(const uint8_t* rootKey, const uint8_t* iv,
                       const galas_paramset_t* ps, galas_bavc_t* vc) {
    const unsigned lb = galas_lambda_bytes(ps);
    const unsigned com_size = 2u * lb;

    uint8_t* nodes = generate_seeds(rootKey, iv, ps);
    if (!nodes) {
        vc->h = vc->k = vc->com = vc->sd = NULL;
        return;
    }

    vc->h = malloc(2u * lb);
    vc->com = malloc((size_t)ps->p.L * com_size);
    vc->sd = malloc((size_t)ps->p.L * lb);
    vc->k = nodes;

    xof_ctx hroot;
    xof_init(&hroot, ps->p.lambda, XOF_DOMAIN_H1);

    for (unsigned i = 0, offset = 0; i < ps->p.tau; ++i) {
        xof_ctx hvec;
        xof_init(&hvec, ps->p.lambda, XOF_DOMAIN_H1);

        const unsigned Ni = galas_bavc_leaves(i, ps);
        for (unsigned j = 0; j < Ni; ++j, ++offset) {
            const uint32_t alpha = pos_in_tree(i, j, ps);
            leaf_commit(vc->sd + (size_t)offset * lb,
                        vc->com + (size_t)offset * com_size,
                        NODE(nodes, alpha, lb), iv, i + ps->p.L - 1u,
                        ps->p.lambda, lb);
            xof_update(&hvec, vc->com + (size_t)offset * com_size, com_size);
        }

        uint8_t hi[2 * GALAS_MAX_LAMBDA_BYTES];
        h1_update_domain(&hvec);
        xof_final(&hvec);
        xof_squeeze(&hvec, hi, 2u * lb);
        xof_clear(&hvec);
        xof_update(&hroot, hi, 2u * lb);
    }

    h1_update_domain(&hroot);
    xof_final(&hroot);
    xof_squeeze(&hroot, vc->h, 2u * lb);
    xof_clear(&hroot);
}

bool galas_bavc_open(const galas_bavc_t* vc, const uint16_t* i_delta,
                     uint8_t* decom, const galas_paramset_t* ps) {
    const unsigned lb = galas_lambda_bytes(ps);
    const unsigned com_size = 2u * lb;
    const uint32_t total_nodes = 2u * ps->p.L - 1u;
    uint8_t* decom_end = decom + (size_t)com_size * ps->p.tau +
                         (size_t)ps->p.T_open * lb;

    uint8_t* hidden = calloc((total_nodes + 7u) / 8u, 1);
    if (!hidden) return false;

    unsigned nh = 0;
    for (unsigned i = 0; i < ps->p.tau; ++i) {
        if (i_delta[i] >= galas_bavc_leaves(i, ps)) {
            free(hidden);
            return false;
        }

        uint32_t alpha = pos_in_tree(i, i_delta[i], ps);
        set_bit(hidden, alpha);
        ++nh;

        while (alpha > 0 && !get_bit(hidden, (alpha - 1u) / 2u)) {
            alpha = (alpha - 1u) / 2u;
            set_bit(hidden, alpha);
            ++nh;
        }
    }

    if (nh - 2u * ps->p.tau + 1u > ps->p.T_open) {
        free(hidden);
        return false;
    }

    uint8_t* dp = decom;
    for (unsigned i = 0, offset = 0; i < ps->p.tau; ++i) {
        const unsigned Ni = galas_bavc_leaves(i, ps);
        memcpy(dp, vc->com + (size_t)(offset + i_delta[i]) * com_size, com_size);
        dp += com_size;
        offset += Ni;
    }

    for (int32_t alpha = (int32_t)ps->p.L - 2; alpha >= 0; --alpha) {
        const uint32_t left = 2u * (uint32_t)alpha + 1u;
        const uint32_t right = left + 1u;
        const int left_hidden = get_bit(hidden, left);
        const int right_hidden = get_bit(hidden, right);

        if (left_hidden || right_hidden) set_bit(hidden, (uint32_t)alpha);
        else clear_bit(hidden, (uint32_t)alpha);

        if (left_hidden ^ right_hidden) {
            const uint32_t sibling = left + (uint32_t)left_hidden;
            if (dp + lb > decom_end) {
                free(hidden);
                return false;
            }
            memcpy(dp, NODE(vc->k, sibling, lb), lb);
            dp += lb;
        }
    }

    if (dp < decom_end) memset(dp, 0, (size_t)(decom_end - dp));
    free(hidden);
    return true;
}

static bool reconstruct_keys(uint8_t* hidden, uint8_t* known, uint8_t* nodes,
                             const uint8_t* decom, const uint16_t* i_delta,
                             const uint8_t* iv, const galas_paramset_t* ps) {
    const unsigned lb = galas_lambda_bytes(ps);
    const unsigned com_size = 2u * lb;
    const uint8_t* seeds = decom + (size_t)com_size * ps->p.tau;
    const uint8_t* seeds_end = seeds + (size_t)ps->p.T_open * lb;

    for (unsigned i = 0; i < ps->p.tau; ++i) {
        if (i_delta[i] >= galas_bavc_leaves(i, ps)) return false;
        set_bit(hidden, pos_in_tree(i, i_delta[i], ps));
    }

    for (int32_t alpha = (int32_t)ps->p.L - 2; alpha >= 0; --alpha) {
        const uint32_t left = 2u * (uint32_t)alpha + 1u;
        const uint32_t right = left + 1u;
        const int left_hidden = get_bit(hidden, left);
        const int right_hidden = get_bit(hidden, right);

        if (left_hidden || right_hidden) set_bit(hidden, (uint32_t)alpha);
        else clear_bit(hidden, (uint32_t)alpha);

        if (left_hidden ^ right_hidden) {
            if (seeds + lb > seeds_end) return false;
            const uint32_t sibling = left + (uint32_t)left_hidden;
            memcpy(NODE(nodes, sibling, lb), seeds, lb);
            set_bit(known, sibling);
            seeds += lb;
        }
    }

    while (seeds < seeds_end) {
        if (*seeds++) return false;
    }

    for (uint32_t alpha = 0; alpha < ps->p.L - 1u; ++alpha) {
        if (!get_bit(known, alpha)) continue;
        const uint32_t left = 2u * alpha + 1u;
        const uint32_t right = left + 1u;
        if (!get_bit(known, left) || !get_bit(known, right)) {
            tree_prg(nodes, alpha, iv, ps->p.lambda, lb);
            set_bit(known, left);
            set_bit(known, right);
        }
    }

    return true;
}

bool galas_bavc_reconstruct(const uint8_t* decom, const uint16_t* i_delta,
                            const uint8_t* iv, const galas_paramset_t* ps,
                            galas_bavc_rec_t* rec) {
    const unsigned lb = galas_lambda_bytes(ps);
    const unsigned com_size = 2u * lb;
    const uint32_t total_nodes = 2u * ps->p.L - 1u;

    uint8_t* hidden = calloc((total_nodes + 7u) / 8u, 1);
    uint8_t* known = calloc((total_nodes + 7u) / 8u, 1);
    uint8_t* nodes = calloc(total_nodes, lb);
    if (!hidden || !known || !nodes) goto fail;

    if (!reconstruct_keys(hidden, known, nodes, decom, i_delta, iv, ps)) goto fail;

    if (!rec->h) goto fail;
    if (!rec->s) {
        rec->s = malloc((size_t)(ps->p.L - ps->p.tau) * lb);
        if (!rec->s) goto fail;
    }

    xof_ctx hroot;
    xof_init(&hroot, ps->p.lambda, XOF_DOMAIN_H1);

    for (unsigned i = 0, offset = 0; i < ps->p.tau; ++i) {
        xof_ctx hvec;
        xof_init(&hvec, ps->p.lambda, XOF_DOMAIN_H1);

        const unsigned Ni = galas_bavc_leaves(i, ps);
        for (unsigned j = 0; j < Ni; ++j) {
            const uint32_t alpha = pos_in_tree(i, j, ps);
            if (j == i_delta[i]) {
                xof_update(&hvec, decom + (size_t)i * com_size, com_size);
            } else {
                uint8_t com[2 * GALAS_MAX_LAMBDA_BYTES];
                leaf_commit(rec->s + (size_t)offset * lb, com,
                            NODE(nodes, alpha, lb), iv, i + ps->p.L - 1u,
                            ps->p.lambda, lb);
                ++offset;
                xof_update(&hvec, com, com_size);
            }
        }

        uint8_t hi[2 * GALAS_MAX_LAMBDA_BYTES];
        h1_update_domain(&hvec);
        xof_final(&hvec);
        xof_squeeze(&hvec, hi, 2u * lb);
        xof_clear(&hvec);
        xof_update(&hroot, hi, 2u * lb);
    }

    h1_update_domain(&hroot);
    xof_final(&hroot);
    xof_squeeze(&hroot, rec->h, 2u * lb);
    xof_clear(&hroot);

    free(nodes);
    free(hidden);
    free(known);
    return true;

fail:
    free(nodes);
    free(hidden);
    free(known);
    return false;
}

void galas_bavc_clear(galas_bavc_t* vc) {
    free(vc->sd);
    free(vc->com);
    free(vc->k);
    free(vc->h);
    vc->sd = vc->com = vc->k = vc->h = NULL;
}

void galas_bavc_rec_clear(galas_bavc_rec_t* rec) {
    free(rec->s);
    rec->s = NULL;
    rec->h = NULL;
}
