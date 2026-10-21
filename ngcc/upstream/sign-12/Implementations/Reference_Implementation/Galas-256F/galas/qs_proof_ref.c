/*
 * qs_proof_ref.c -- portable degree-3 QuickSilver proof for Galas.
 */
#include "qs_proof_ref.h"
#include "gf2n.h"
#include "owf.h"
#include <stdlib.h>
#include <string.h>

#include "galas_params_128.h"
#include "galas_params_160.h"
#include "galas_params_256.h"
#include "galas_params_384.h"
#include "galas_params_512.h"

#define QS_DEGREE 3u
#define QS_NUM_CONSTRAINTS 5u

static unsigned witness_bits(unsigned lambda) { return (5u * lambda) / 2u; }

static unsigned get_bit_le(const uint8_t* in, unsigned bit) {
    return (unsigned)((in[bit / 8u] >> (bit % 8u)) & 1u);
}

static void gf_set_bit(const gf_ctx* ctx, gf_limb_t* out, unsigned bit) {
    gf_zero(ctx, out);
    out[bit / 64u] = ((gf_limb_t)1u << (bit % 64u));
}

static void gf_one(const gf_ctx* ctx, gf_limb_t* out) {
    gf_zero(ctx, out);
    out[0] = 1u;
}

static void gf_mul_alpha(const gf_ctx* ctx, gf_limb_t* out, const gf_limb_t* a) {
    const unsigned L = ctx->nlimbs;
    const unsigned rem = ctx->n % 64u;
    const unsigned top_bit = rem ? (rem - 1u) : 63u;
    const unsigned top_limb = L - 1u;
    const unsigned carry_top = (unsigned)((a[top_limb] >> top_bit) & 1u);
    gf_limb_t carry = 0;
    for (unsigned i = 0; i < L; ++i) {
        gf_limb_t next = a[i] >> 63;
        out[i] = (a[i] << 1) | carry;
        carry = next;
    }
    if (rem) out[L - 1u] &= (((gf_limb_t)1u << rem) - 1u);
    if (carry_top) {
        for (unsigned i = 0; i < L; ++i) {
            gf_limb_t low = ctx->phi[i];
            if (i == L - 1u && rem) low &= (((gf_limb_t)1u << rem) - 1u);
            out[i] ^= low;
        }
    }
}

static const uint8_t* get_bprime(unsigned lambda) {
    switch (lambda) {
        case 128: return (const uint8_t*)GALAS_128_BPRIME;
        case 160: return (const uint8_t*)GALAS_160_BPRIME;
        case 256: return (const uint8_t*)GALAS_256_BPRIME;
        case 384: return (const uint8_t*)GALAS_384_BPRIME;
        case 512: return (const uint8_t*)GALAS_512_BPRIME;
        default: return NULL;
    }
}

static const uint64_t* map_coeffs(const galas_owf_params* P, unsigned id) {
    switch (id) {
        case 0: return P->M0;
        case 1: return P->M1;
        case 2: return P->M2;
        case 3: return P->L0;
        case 4: return P->L1;
        case 5: return P->L2;
        default: return P->L3;
    }
}

typedef struct {
    unsigned lambda;
    unsigned nlimbs;
    int ready;
    gf_limb_t* epow2;        /* [lambda][lambda] */
    gf_limb_t* bprime_frob;  /* [lambda][lambda/2] */
    gf_limb_t* m_frob[3];    /* [lambda][lambda] for M0/M1/M2 */
} qs_const_cache;

static qs_const_cache g_caches[5];

static int cache_slot(unsigned lambda) {
    switch (lambda) {
        case 128: return 0;
        case 160: return 1;
        case 256: return 2;
        case 384: return 3;
        case 512: return 4;
        default: return -1;
    }
}

static gf_limb_t* table2(gf_limb_t* base, unsigned L, unsigned cols,
                         unsigned row, unsigned col) {
    return base + ((size_t)row * cols + col) * L;
}

static const gf_limb_t* ctable2(const gf_limb_t* base, unsigned L, unsigned cols,
                                unsigned row, unsigned col) {
    return base + ((size_t)row * cols + col) * L;
}

static int ensure_cache(qs_const_cache** out, const gf_ctx* ctx,
                        const galas_owf_params* P) {
    const unsigned n = ctx->n;
    const unsigned m = n / 2u;
    const unsigned L = ctx->nlimbs;
    int slot = cache_slot(n);
    if (slot < 0) return -1;
    qs_const_cache* c = &g_caches[slot];
    if (c->ready) {
        *out = c;
        return 0;
    }

    c->lambda = n;
    c->nlimbs = L;
    c->epow2 = calloc((size_t)n * n * L, sizeof(gf_limb_t));
    c->bprime_frob = calloc((size_t)n * m * L, sizeof(gf_limb_t));
    for (unsigned map = 0; map < 3; ++map) {
        c->m_frob[map] = calloc((size_t)n * n * L, sizeof(gf_limb_t));
    }
    if (!c->epow2 || !c->bprime_frob || !c->m_frob[0] ||
        !c->m_frob[1] || !c->m_frob[2]) {
        return -1;
    }

    for (unsigned t = 0; t < n; ++t) {
        gf_limb_t* dst = table2(c->epow2, L, n, 0, t);
        gf_set_bit(ctx, dst, t);
        for (unsigned i = 1; i < n; ++i) {
            gf_sqr(ctx, table2(c->epow2, L, n, i, t),
                   table2(c->epow2, L, n, i - 1u, t));
        }
    }

    const uint8_t* bprime = get_bprime(n);
    if (!bprime) return -1;
    for (unsigned j = 0; j < m; ++j) {
        gf_from_bytes(ctx, table2(c->bprime_frob, L, m, 0, j),
                      bprime + (size_t)j * ctx->nbytes);
        for (unsigned i = 1; i < n; ++i) {
            gf_sqr(ctx, table2(c->bprime_frob, L, m, i, j),
                   table2(c->bprime_frob, L, m, i - 1u, j));
        }
    }

    for (unsigned map = 0; map < 3; ++map) {
        const uint64_t* coeffs = map_coeffs(P, map);
        for (unsigned j = 0; j < n; ++j) {
            gf_limb_t* dst = table2(c->m_frob[map], L, n, 0, j);
            for (unsigned k = 0; k < L; ++k) dst[k] = coeffs[(size_t)j * L + k];
            for (unsigned i = 1; i < n; ++i) {
                gf_sqr(ctx, table2(c->m_frob[map], L, n, i, j),
                       table2(c->m_frob[map], L, n, i - 1u, j));
            }
        }
    }

    c->ready = 1;
    *out = c;
    return 0;
}

typedef struct {
    unsigned deg;
    gf_limb_t c[QS_DEGREE + 1u][GF_LIMBS(512)];
} qs_elem_ref;

typedef struct {
    const gf_ctx* ctx;
    int verifier;
    const uint8_t* witness;
    gf_limb_t (*macs)[GF_LIMBS(512)];
    gf_limb_t delta_pows[QS_DEGREE][GF_LIMBS(512)];
    gf_limb_t hash_comb[2][GF_LIMBS(512)];
    gf_limb_t key_secpar[GF_LIMBS(512)];
    gf_limb_t key64[GF_LIMBS(512)];
    gf_limb_t h_secpar[QS_DEGREE][GF_LIMBS(512)];
    gf_limb_t h_64[QS_DEGREE][GF_LIMBS(512)];
} qs_state_ref;

static void elem_clear(const gf_ctx* ctx, qs_elem_ref* e, unsigned deg) {
    e->deg = deg;
    for (unsigned i = 0; i <= QS_DEGREE; ++i) gf_zero(ctx, e->c[i]);
}

static void elem_copy(const gf_ctx* ctx, qs_elem_ref* out, const qs_elem_ref* in) {
    out->deg = in->deg;
    for (unsigned i = 0; i <= QS_DEGREE; ++i) gf_copy(ctx, out->c[i], in->c[i]);
}

static void elem_const(qs_elem_ref* out, const qs_state_ref* st,
                       unsigned deg, const gf_limb_t* value) {
    const gf_ctx* ctx = st->ctx;
    elem_clear(ctx, out, deg);
    if (st->verifier) {
        if (deg == 0) gf_copy(ctx, out->c[0], value);
        else gf_mul(ctx, out->c[0], value, st->delta_pows[deg - 1u]);
    } else {
        gf_copy(ctx, out->c[deg], value);
    }
}

static void elem_promote(qs_elem_ref* out, const qs_state_ref* st,
                         const qs_elem_ref* in, unsigned deg) {
    const gf_ctx* ctx = st->ctx;
    elem_clear(ctx, out, deg);
    if (deg == in->deg) {
        elem_copy(ctx, out, in);
        return;
    }
    unsigned shift = deg - in->deg;
    if (st->verifier) {
        gf_mul(ctx, out->c[0], in->c[0], st->delta_pows[shift - 1u]);
    } else {
        for (unsigned i = 0; i <= in->deg; ++i) {
            gf_copy(ctx, out->c[i + shift], in->c[i]);
        }
    }
}

static void elem_add(qs_elem_ref* out, const qs_state_ref* st,
                     const qs_elem_ref* a, const qs_elem_ref* b) {
    const gf_ctx* ctx = st->ctx;
    unsigned deg = a->deg > b->deg ? a->deg : b->deg;
    qs_elem_ref ap, bp;
    elem_promote(&ap, st, a, deg);
    elem_promote(&bp, st, b, deg);
    elem_clear(ctx, out, deg);
    if (st->verifier) {
        gf_add(ctx, out->c[0], ap.c[0], bp.c[0]);
    } else {
        for (unsigned i = 0; i <= deg; ++i) gf_add(ctx, out->c[i], ap.c[i], bp.c[i]);
    }
}

static void elem_add_inplace(qs_elem_ref* acc, const qs_state_ref* st,
                             const qs_elem_ref* b) {
    qs_elem_ref tmp;
    elem_add(&tmp, st, acc, b);
    elem_copy(st->ctx, acc, &tmp);
}

static void elem_scale(qs_elem_ref* out, const qs_state_ref* st,
                       const qs_elem_ref* in, const gf_limb_t* k) {
    const gf_ctx* ctx = st->ctx;
    elem_clear(ctx, out, in->deg);
    if (st->verifier) {
        gf_mul(ctx, out->c[0], in->c[0], k);
    } else {
        for (unsigned i = 0; i <= in->deg; ++i) gf_mul(ctx, out->c[i], in->c[i], k);
    }
}

static void elem_mul(qs_elem_ref* out, const qs_state_ref* st,
                     const qs_elem_ref* a, const qs_elem_ref* b) {
    const gf_ctx* ctx = st->ctx;
    unsigned deg = a->deg + b->deg;
    elem_clear(ctx, out, deg);
    if (st->verifier) {
        gf_mul(ctx, out->c[0], a->c[0], b->c[0]);
        return;
    }
    for (unsigned i = 0; i <= a->deg; ++i) {
        for (unsigned j = 0; j <= b->deg; ++j) {
            gf_limb_t prod[GF_LIMBS(512)];
            gf_mul(ctx, prod, a->c[i], b->c[j]);
            gf_xor(ctx, out->c[i + j], prod);
        }
    }
}

static void elem_bit(qs_elem_ref* out, const qs_state_ref* st, unsigned index) {
    const gf_ctx* ctx = st->ctx;
    elem_clear(ctx, out, 1);
    if (st->verifier) {
        gf_copy(ctx, out->c[0], st->macs[index]);
    } else {
        gf_copy(ctx, out->c[0], st->macs[index]);
        if (get_bit_le(st->witness, index)) out->c[1][0] = 1u;
    }
}

static void hash_update(qs_state_ref* st, unsigned idx, const gf_limb_t* in) {
    const gf_ctx* ctx = st->ctx;
    gf_limb_t tmp[GF_LIMBS(512)];
    gf_mul(ctx, tmp, st->h_secpar[idx], st->key_secpar);
    gf_add(ctx, st->h_secpar[idx], tmp, in);
    gf_mul(ctx, tmp, st->h_64[idx], st->key64);
    gf_add(ctx, st->h_64[idx], tmp, in);
}

static void hash_finalize(const qs_state_ref* st, unsigned idx,
                          const gf_limb_t* mask, uint8_t* out) {
    const gf_ctx* ctx = st->ctx;
    gf_limb_t t0[GF_LIMBS(512)], t1[GF_LIMBS(512)], sum[GF_LIMBS(512)];
    gf_mul(ctx, t0, st->hash_comb[0], st->h_secpar[idx]);
    gf_mul(ctx, t1, st->hash_comb[1], st->h_64[idx]);
    gf_add(ctx, sum, mask, t0);
    gf_xor(ctx, sum, t1);
    gf_to_bytes(ctx, out, sum);
}

static int add_constraint(qs_state_ref* st, const qs_elem_ref* x) {
    qs_elem_ref xmax;
    elem_promote(&xmax, st, x, QS_DEGREE);
    if (st->verifier) {
        hash_update(st, 0, xmax.c[0]);
    } else {
        if (!gf_is_zero(st->ctx, xmax.c[QS_DEGREE])) return -1;
        for (unsigned i = 0; i < QS_DEGREE; ++i) {
            hash_update(st, i, xmax.c[i]);
        }
    }
    return 0;
}

static void state_init(qs_state_ref* st, const gf_ctx* ctx, int verifier,
                       const uint8_t* witness,
                       gf_limb_t (*macs)[GF_LIMBS(512)],
                       const uint8_t* delta,
                       const uint8_t* challenge) {
    memset(st, 0, sizeof(*st));
    st->ctx = ctx;
    st->verifier = verifier;
    st->witness = witness;
    st->macs = macs;
    const unsigned lb = ctx->nbytes;
    gf_from_bytes(ctx, st->hash_comb[0], challenge);
    gf_from_bytes(ctx, st->hash_comb[1], challenge + lb);
    gf_from_bytes(ctx, st->key_secpar, challenge + 2u * lb);
    uint8_t key64_bytes[GALAS_MAX_LAMBDA_BYTES] = {0};
    memcpy(key64_bytes, challenge + 3u * lb, 8);
    gf_from_bytes(ctx, st->key64, key64_bytes);
    if (verifier) {
        gf_from_bytes(ctx, st->delta_pows[0], delta);
        for (unsigned i = 1; i < QS_DEGREE; ++i) {
            gf_mul(ctx, st->delta_pows[i], st->delta_pows[i - 1u],
                   st->delta_pows[0]);
        }
    }
}

static int transpose_macs(gf_limb_t (*macs)[GF_LIMBS(512)],
                          uint8_t* const* cols,
                          const uint8_t* correction,
                          const uint8_t* delta,
                          const galas_paramset_t* ps,
                          unsigned rows) {
    gf_ctx ctx;
    if (gf_init(&ctx, ps->p.lambda) != 0) return -1;
    const unsigned lambda = ps->p.lambda;
    const unsigned delta_cols = lambda - ps->p.w_grind;
    const unsigned wb = witness_bits(lambda);
    for (unsigned r = 0; r < rows; ++r) gf_zero(&ctx, macs[r]);
    for (unsigned col = 0; col < lambda; ++col) {
        unsigned dbit = (delta && col < delta_cols) ? get_bit_le(delta, col) : 0;
        for (unsigned r = 0; r < rows; ++r) {
            unsigned bit = get_bit_le(cols[col], r);
            if (correction && dbit && r < wb) bit ^= get_bit_le(correction, r);
            if (bit) macs[r][col / 64u] ^= ((gf_limb_t)1u << (col % 64u));
        }
    }
    return 0;
}

static void combine_mac_masks(const qs_state_ref* st, gf_limb_t* out,
                              unsigned offset) {
    const gf_ctx* ctx = st->ctx;
    const unsigned n = ctx->n;
    gf_copy(ctx, out, st->macs[offset + n - 1u]);
    for (int r = (int)n - 2; r >= 0; --r) {
        gf_limb_t tmp[GF_LIMBS(512)];
        gf_mul_alpha(ctx, tmp, out);
        gf_add(ctx, out, tmp, st->macs[offset + (unsigned)r]);
    }
}

static void load_const_frob(const gf_ctx* ctx, gf_limb_t (*out)[GF_LIMBS(512)],
                            const uint8_t* bytes) {
    gf_from_bytes(ctx, out[0], bytes);
    for (unsigned i = 1; i < ctx->n; ++i) gf_sqr(ctx, out[i], out[i - 1u]);
}

static void load_xor_const_frob(const gf_ctx* ctx,
                                gf_limb_t (*out)[GF_LIMBS(512)],
                                const uint8_t* a, const uint8_t* b) {
    uint8_t tmp[GALAS_MAX_LAMBDA_BYTES];
    for (unsigned i = 0; i < ctx->nbytes; ++i) tmp[i] = a[i] ^ b[i];
    load_const_frob(ctx, out, tmp);
}

static void build_k_frob(qs_elem_ref* k_frob, const qs_state_ref* st,
                         const qs_const_cache* cache,
                         const qs_elem_ref* k_bits) {
    const gf_ctx* ctx = st->ctx;
    const unsigned n = ctx->n;
    const unsigned L = ctx->nlimbs;
    for (unsigned i = 0; i < n; ++i) {
        elem_clear(ctx, &k_frob[i], 1);
        for (unsigned t = 0; t < n; ++t) {
            qs_elem_ref term;
            elem_scale(&term, st, &k_bits[t],
                       ctable2(cache->epow2, L, n, i, t));
            elem_add_inplace(&k_frob[i], st, &term);
        }
    }
}

static void build_rho_frob(qs_elem_ref* rho, const qs_state_ref* st,
                           const qs_elem_ref* k_frob,
                           gf_limb_t (*c_frob)[GF_LIMBS(512)]) {
    const gf_ctx* ctx = st->ctx;
    for (unsigned i = 0; i < ctx->n; ++i) {
        qs_elem_ref cst;
        elem_const(&cst, st, 1, c_frob[i]);
        elem_add(&rho[i], st, &k_frob[i], &cst);
    }
}

static void build_a_frob(qs_elem_ref* a_frob, const qs_state_ref* st,
                         const qs_const_cache* cache,
                         const qs_elem_ref* rho_frob,
                         unsigned map_id) {
    const gf_ctx* ctx = st->ctx;
    const unsigned n = ctx->n;
    const unsigned L = ctx->nlimbs;
    for (unsigned i = 0; i < n; ++i) {
        elem_clear(ctx, &a_frob[i], 1);
        for (unsigned j = 0; j < n; ++j) {
            unsigned idx = (i + j) % n;
            qs_elem_ref term;
            elem_scale(&term, st, &rho_frob[idx],
                       ctable2(cache->m_frob[map_id], L, n, i, j));
            elem_add_inplace(&a_frob[i], st, &term);
        }
    }
}

static void build_sigma_frob(qs_elem_ref* sigma_frob, const qs_state_ref* st,
                             const qs_const_cache* cache,
                             const qs_elem_ref* sigma_bits) {
    const gf_ctx* ctx = st->ctx;
    const unsigned n = ctx->n;
    const unsigned m = n / 2u;
    const unsigned L = ctx->nlimbs;
    for (unsigned i = 0; i < n; ++i) {
        elem_clear(ctx, &sigma_frob[i], 1);
        for (unsigned j = 0; j < m; ++j) {
            qs_elem_ref term;
            elem_scale(&term, st, &sigma_bits[j],
                       ctable2(cache->bprime_frob, L, m, i, j));
            elem_add_inplace(&sigma_frob[i], st, &term);
        }
    }
}

static void linear_from_a(qs_elem_ref* out, const qs_state_ref* st,
                          const qs_elem_ref* sigma_frob,
                          const qs_elem_ref* a_frob,
                          const uint64_t* coeffs) {
    const gf_ctx* ctx = st->ctx;
    const unsigned n = ctx->n;
    const unsigned m = n / 2u;
    const unsigned L = ctx->nlimbs;
    elem_clear(ctx, out, 2);
    for (unsigned i = 0; i < n; ++i) {
        qs_elem_ref prod, term;
        elem_mul(&prod, st, &sigma_frob[i], &a_frob[(i + m) % n]);
        elem_scale(&term, st, &prod, coeffs + (size_t)i * L);
        elem_add_inplace(out, st, &term);
    }
}

static void linear_l3k(qs_elem_ref* out, const qs_state_ref* st,
                       const qs_elem_ref* k_frob,
                       const uint64_t* coeffs) {
    const gf_ctx* ctx = st->ctx;
    const unsigned n = ctx->n;
    const unsigned L = ctx->nlimbs;
    elem_clear(ctx, out, 1);
    for (unsigned i = 0; i < n; ++i) {
        qs_elem_ref term;
        elem_scale(&term, st, &k_frob[i], coeffs + (size_t)i * L);
        elem_add_inplace(out, st, &term);
    }
}

static int add_galas_constraints(qs_state_ref* st,
                                 const qs_const_cache* cache,
                                 const galas_owf_params* P,
                                 const uint8_t* pk_x,
                                 const uint8_t* pk_y) {
    const gf_ctx* ctx = st->ctx;
    const unsigned n = ctx->n;
    const unsigned m = n / 2u;

    int rc = -1;
    qs_elem_ref* k_bits = calloc(n, sizeof(qs_elem_ref));
    qs_elem_ref* k_frob = calloc(n, sizeof(qs_elem_ref));
    qs_elem_ref* rho0 = calloc(n, sizeof(qs_elem_ref));
    qs_elem_ref* rho1 = calloc(n, sizeof(qs_elem_ref));
    qs_elem_ref* rho2 = calloc(n, sizeof(qs_elem_ref));
    qs_elem_ref* a0 = calloc(n, sizeof(qs_elem_ref));
    qs_elem_ref* a1 = calloc(n, sizeof(qs_elem_ref));
    qs_elem_ref* a2 = calloc(n, sizeof(qs_elem_ref));
    qs_elem_ref* s0_bits = calloc(m, sizeof(qs_elem_ref));
    qs_elem_ref* s1_bits = calloc(m, sizeof(qs_elem_ref));
    qs_elem_ref* s2_bits = calloc(m, sizeof(qs_elem_ref));
    qs_elem_ref* s0 = calloc(n, sizeof(qs_elem_ref));
    qs_elem_ref* s1 = calloc(n, sizeof(qs_elem_ref));
    qs_elem_ref* s2 = calloc(n, sizeof(qs_elem_ref));
    gf_limb_t (*c0_frob)[GF_LIMBS(512)] = calloc(n, sizeof(*c0_frob));
    gf_limb_t (*c1_frob)[GF_LIMBS(512)] = calloc(n, sizeof(*c1_frob));
    gf_limb_t (*c2_frob)[GF_LIMBS(512)] = calloc(n, sizeof(*c2_frob));

    if (!k_bits || !k_frob || !rho0 || !rho1 || !rho2 || !a0 || !a1 || !a2 ||
        !s0_bits || !s1_bits || !s2_bits || !s0 || !s1 || !s2 ||
        !c0_frob || !c1_frob || !c2_frob) {
        goto done;
    }

    for (unsigned i = 0; i < n; ++i) elem_bit(&k_bits[i], st, i);
    for (unsigned i = 0; i < m; ++i) {
        elem_bit(&s0_bits[i], st, n + i);
        elem_bit(&s1_bits[i], st, n + m + i);
        elem_bit(&s2_bits[i], st, n + 2u * m + i);
    }

    {
        qs_elem_ref prod, one, expr;
        gf_limb_t one_fe[GF_LIMBS(512)];
        gf_one(ctx, one_fe);
        elem_mul(&prod, st, &k_bits[0], &k_bits[1]);
        elem_const(&one, st, prod.deg, one_fe);
        elem_add(&expr, st, &prod, &one);
        if (add_constraint(st, &expr) != 0) goto done;
    }

    build_k_frob(k_frob, st, cache, k_bits);
    load_xor_const_frob(ctx, c0_frob, pk_x, P->c0);
    load_const_frob(ctx, c1_frob, P->c1);
    load_const_frob(ctx, c2_frob, P->c2);
    build_rho_frob(rho0, st, k_frob, c0_frob);
    build_rho_frob(rho1, st, k_frob, c1_frob);
    build_rho_frob(rho2, st, k_frob, c2_frob);
    build_a_frob(a0, st, cache, rho0, 0);
    build_a_frob(a1, st, cache, rho1, 1);
    build_a_frob(a2, st, cache, rho2, 2);
    build_sigma_frob(s0, st, cache, s0_bits);
    build_sigma_frob(s1, st, cache, s1_bits);
    build_sigma_frob(s2, st, cache, s2_bits);

    {
        qs_elem_ref c0p, c1p, c2p, lhs, one, expr;
        gf_limb_t one_fe[GF_LIMBS(512)];
        gf_one(ctx, one_fe);

        elem_mul(&c0p, st, &a0[0], &a0[m]);
        elem_mul(&lhs, st, &s0[0], &c0p);
        elem_const(&one, st, lhs.deg, one_fe);
        elem_add(&expr, st, &lhs, &one);
        if (add_constraint(st, &expr) != 0) goto done;
        elem_mul(&c1p, st, &a1[0], &a1[m]);
        elem_mul(&lhs, st, &s1[0], &c1p);
        elem_const(&one, st, lhs.deg, one_fe);
        elem_add(&expr, st, &lhs, &one);
        if (add_constraint(st, &expr) != 0) goto done;
        elem_mul(&c2p, st, &a2[0], &a2[m]);
        elem_mul(&lhs, st, &s2[0], &c2p);
        elem_const(&one, st, lhs.deg, one_fe);
        elem_add(&expr, st, &lhs, &one);
        if (add_constraint(st, &expr) != 0) goto done;
    }

    {
        qs_elem_ref l0, l1, l2, z, tmp, L3k, yconst, t, prod, one, expr;
        gf_limb_t y_fe[GF_LIMBS(512)];
        gf_limb_t one_fe[GF_LIMBS(512)];
        linear_from_a(&l0, st, s0, a0, P->L0);
        linear_from_a(&l1, st, s1, a1, P->L1);
        linear_from_a(&l2, st, s2, a2, P->L2);
        elem_add(&tmp, st, &l0, &l1);
        elem_add(&z, st, &tmp, &l2);
        linear_l3k(&L3k, st, k_frob, P->L3);
        gf_from_bytes(ctx, y_fe, pk_y);
        elem_const(&yconst, st, 1, y_fe);
        elem_add(&t, st, &yconst, &L3k);
        elem_mul(&prod, st, &z, &t);
        gf_one(ctx, one_fe);
        elem_const(&one, st, prod.deg, one_fe);
        elem_add(&expr, st, &prod, &one);
        if (add_constraint(st, &expr) != 0) goto done;
    }

    rc = 0;

done:
    free(k_bits); free(k_frob); free(rho0); free(rho1); free(rho2);
    free(a0); free(a1); free(a2);
    free(s0_bits); free(s1_bits); free(s2_bits);
    free(s0); free(s1); free(s2);
    free(c0_frob); free(c1_frob); free(c2_frob);
    return rc;
}

static void prove_finalize(qs_state_ref* st, uint8_t* proof, uint8_t* check,
                           const uint8_t* witness) {
    const gf_ctx* ctx = st->ctx;
    const unsigned n = ctx->n;
    const unsigned lb = ctx->nbytes;
    const unsigned wb = witness_bits(n);
    gf_limb_t masks[QS_DEGREE][GF_LIMBS(512)];
    for (unsigned i = 0; i < QS_DEGREE; ++i) gf_zero(ctx, masks[i]);
    unsigned offset = wb;
    for (unsigned i = 0; i < QS_DEGREE - 1u; ++i, offset += n) {
        gf_limb_t mac_mask[GF_LIMBS(512)], val_mask[GF_LIMBS(512)];
        combine_mac_masks(st, mac_mask, offset);
        gf_from_bytes(ctx, val_mask, witness + offset / 8u);
        gf_xor(ctx, masks[i], mac_mask);
        gf_xor(ctx, masks[i + 1u], val_mask);
    }
    hash_finalize(st, 0, masks[0], check);
    hash_finalize(st, 1, masks[1], proof);
    hash_finalize(st, 2, masks[2], proof + lb);
}

static void verify_finalize(qs_state_ref* st, const uint8_t* proof,
                            uint8_t* check) {
    const gf_ctx* ctx = st->ctx;
    const unsigned n = ctx->n;
    const unsigned lb = ctx->nbytes;
    const unsigned wb = witness_bits(n);
    gf_limb_t mask[GF_LIMBS(512)], extra[GF_LIMBS(512)];
    combine_mac_masks(st, mask, wb);
    combine_mac_masks(st, extra, wb + n);
    gf_limb_t p0[GF_LIMBS(512)], p1[GF_LIMBS(512)], coeff[GF_LIMBS(512)];
    gf_from_bytes(ctx, p0, proof);
    gf_from_bytes(ctx, p1, proof + lb);
    gf_add(ctx, coeff, p0, extra);
    gf_limb_t term[GF_LIMBS(512)];
    gf_mul(ctx, term, coeff, st->delta_pows[0]);
    gf_xor(ctx, mask, term);
    gf_mul(ctx, term, p1, st->delta_pows[1]);
    gf_xor(ctx, mask, term);
    hash_finalize(st, 0, mask, check);
}

int galas_qs_prove(uint8_t* qs_proof, uint8_t* qs_check,
                   uint8_t* u_with_witness, uint8_t* const* v,
                   const uint8_t* challenge,
                   const uint8_t* pk_x, const uint8_t* pk_y,
                   const galas_paramset_t* ps) {
    gf_ctx ctx;
    if (gf_init(&ctx, ps->p.lambda) != 0) return -1;
    const galas_owf_params* P = galas_owf_params_for(ps->p.lambda);
    if (!P) return -1;
    qs_const_cache* cache = NULL;
    if (ensure_cache(&cache, &ctx, P) != 0) return -1;

    const unsigned rows = witness_bits(ps->p.lambda) + 2u * ps->p.lambda;
    gf_limb_t (*macs)[GF_LIMBS(512)] = calloc(rows, sizeof(*macs));
    if (!macs) return -1;
    if (transpose_macs(macs, v, NULL, NULL, ps, rows) != 0) {
        free(macs);
        return -1;
    }

    qs_state_ref st;
    state_init(&st, &ctx, 0, u_with_witness, macs, NULL, challenge);
    int rc = add_galas_constraints(&st, cache, P, pk_x, pk_y);
    if (rc == 0) prove_finalize(&st, qs_proof, qs_check, u_with_witness);
    free(macs);
    return rc;
}

int galas_qs_verify(uint8_t* qs_check, const uint8_t* qs_proof,
                    uint8_t* const* q,
                    const uint8_t* correction,
                    const uint8_t* delta,
                    const uint8_t* challenge,
                    const uint8_t* pk_x, const uint8_t* pk_y,
                    const galas_paramset_t* ps) {
    gf_ctx ctx;
    if (gf_init(&ctx, ps->p.lambda) != 0) return -1;
    const galas_owf_params* P = galas_owf_params_for(ps->p.lambda);
    if (!P) return -1;
    qs_const_cache* cache = NULL;
    if (ensure_cache(&cache, &ctx, P) != 0) return -1;

    const unsigned rows = witness_bits(ps->p.lambda) + 2u * ps->p.lambda;
    gf_limb_t (*macs)[GF_LIMBS(512)] = calloc(rows, sizeof(*macs));
    if (!macs) return -1;
    if (transpose_macs(macs, q, correction, delta, ps, rows) != 0) {
        free(macs);
        return -1;
    }
    qs_state_ref st;
    state_init(&st, &ctx, 1, NULL, macs, delta, challenge);
    int rc = add_galas_constraints(&st, cache, P, pk_x, pk_y);
    if (rc == 0) verify_finalize(&st, qs_proof, qs_check);
    free(macs);
    return rc;
}
