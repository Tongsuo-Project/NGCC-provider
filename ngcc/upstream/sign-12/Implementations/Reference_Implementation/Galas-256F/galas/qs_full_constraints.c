/*
 * qs_full_constraints.c — full degree-3 Gala OWF constraint system.
 * See qs_full_constraints.h. Port of owf_proof.inc::galas_owf_constraints
 * evaluated at Δ=0 (plaintext) for verifiable constraint logic.
 */
#include "qs_full_constraints.h"
#include "bf.h"
#include <string.h>

/* P permutation and B' basis are in galas_params_<n>.h, accessed via the
   generated headers. We need them for sigma computation and embedding. */
#include "galas_params_128.h"
#include "galas_params_160.h"
#include "galas_params_256.h"
#include "galas_params_384.h"
#include "galas_params_512.h"

/* get the P permutation and B' for a given lambda */
static const uint16_t* get_perm(unsigned lambda, unsigned* m_out) {
    *m_out = lambda / 2;
    switch (lambda) {
        case 128: return GALAS_128_P;
        case 160: return GALAS_160_P;
        case 256: return GALAS_256_P;
        case 384: return GALAS_384_P;
        case 512: return GALAS_512_P;
        default: return NULL;
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

/* bf_ctx_128_internal and fc_for_impl reserved for future s128 support */
static gf_ctx ctx128_local __attribute__((unused));
static const gf_ctx* bf_ctx_128_internal(void) __attribute__((unused));
static const gf_ctx* bf_ctx_128_internal(void) {
    static int ready = 0;
    if (!ready) { gf_init(&ctx128_local, 128); ready = 1; }
    return &ctx128_local;
}

static const gf_ctx* fc_for_impl(unsigned lambda) __attribute__((unused));
static const gf_ctx* fc_for_impl(unsigned lambda) {
    switch (lambda) {
        case 128: return bf_ctx_128_internal();
        case 160: return bf_ctx_160();
        case 256: return bf_ctx_256();
        case 384: return bf_ctx_384();
        case 512: return bf_ctx_512();
        default: return NULL;
    }
}

void galas_compute_sigma(uint8_t* sigma_out, const uint8_t* a_in,
                         const gf_ctx* fc, const galas_owf_params* P) {
    (void)P;
    unsigned lambda = fc->n;
    unsigned m = lambda / 2;
    unsigned m_bytes = m / 8;

    /* 1. load a */
    gf_limb_t x[GF_LIMBS(512)];
    gf_from_bytes(fc, x, a_in);
    if (gf_is_zero(fc, x)) { memset(sigma_out, 0, m_bytes); return; }

    /* 2. t = x^{2^m} (m squarings) */
    gf_limb_t t[GF_LIMBS(512)];
    gf_copy(fc, t, x);
    for (unsigned i = 0; i < m; ++i) {
        gf_limb_t sq[GF_LIMBS(512)];
        gf_sqr(fc, sq, t);
        gf_copy(fc, t, sq);
    }

    /* 3. u = x * t = x^{2^m + 1} */
    gf_limb_t u[GF_LIMBS(512)];
    gf_mul(fc, u, x, t);

    /* 4. v = u^{-1} */
    gf_limb_t v[GF_LIMBS(512)];
    gf_inv(fc, v, u);

    /* 5. permute: select m bits from v using P */
    uint8_t v_bytes[64];
    gf_to_bytes(fc, v_bytes, v);
    memset(sigma_out, 0, m_bytes);
    unsigned dummy;
    const uint16_t* P_perm = get_perm(lambda, &dummy);
    for (unsigned j = 0; j < m; ++j) {
        uint16_t src = P_perm[j];
        uint8_t bit = (v_bytes[src / 8] >> (src % 8)) & 1;
        sigma_out[j / 8] |= (bit << (j % 8));
    }
}

int galas_constraints_eval(const gf_ctx* fc, const galas_owf_params* P,
                           const uint8_t* k,
                           const uint8_t* sigma0, const uint8_t* sigma1,
                           const uint8_t* sigma2,
                           const uint8_t* x, const uint8_t* y) {
    unsigned lambda = fc->n;
    unsigned lb = lambda / 8;
    unsigned m = lambda / 2;

    /* load k, x, y, c0/c1/c2 */
    gf_limb_t K[GF_LIMBS(512)], X[GF_LIMBS(512)], Y[GF_LIMBS(512)];
    gf_limb_t C0[GF_LIMBS(512)], C1[GF_LIMBS(512)], C2[GF_LIMBS(512)];
    gf_from_bytes(fc, K, k);
    gf_from_bytes(fc, X, x);
    gf_from_bytes(fc, Y, y);
    gf_from_bytes(fc, C0, P->c0);
    gf_from_bytes(fc, C1, P->c1);
    gf_from_bytes(fc, C2, P->c2);

    /* constraint 1: k[0]·k[1] = 1 (first two bits) */
    uint8_t k0 = k[0] & 1, k1 = (k[0] >> 1) & 1;
    if (!(k0 & k1)) return 0;

    /* rho_i and a_i = M_i(rho_i) */
    gf_limb_t rho0[GF_LIMBS(512)], rho1[GF_LIMBS(512)], rho2[GF_LIMBS(512)];
    for (unsigned i = 0; i < fc->nlimbs; ++i) {
        rho0[i] = X[i] ^ K[i] ^ C0[i];
        rho1[i] = K[i] ^ C1[i];
        rho2[i] = K[i] ^ C2[i];
    }
    gf_limb_t a0[GF_LIMBS(512)], a1[GF_LIMBS(512)], a2[GF_LIMBS(512)];
    galas_apply_lin_map(fc, a0, rho0, P->M0);
    galas_apply_lin_map(fc, a1, rho1, P->M1);
    galas_apply_lin_map(fc, a2, rho2, P->M2);

    /* embed sigma_i to GF(2^lambda) via B' basis */
    const uint8_t* bprime = get_bprime(lambda);
    if (!bprime) return 0;
    gf_limb_t s0[GF_LIMBS(512)], s1[GF_LIMBS(512)], s2[GF_LIMBS(512)];
    gf_zero(fc, s0); gf_zero(fc, s1); gf_zero(fc, s2);
    for (unsigned j = 0; j < m; ++j) {
        gf_limb_t bj[GF_LIMBS(512)];
        gf_from_bytes(fc, bj, bprime + (size_t)j * lb);
        if ((sigma0[j/8] >> (j%8)) & 1) gf_xor(fc, s0, bj);
        if ((sigma1[j/8] >> (j%8)) & 1) gf_xor(fc, s1, bj);
        if ((sigma2[j/8] >> (j%8)) & 1) gf_xor(fc, s2, bj);
    }

    /* constraint 2: sigma_i * a_i^{2^m} * a_i = 1 (degree 3) */
    gf_limb_t one[GF_LIMBS(512)]; gf_zero(fc, one); one[0] = 1;
    for (int idx = 0; idx < 3; ++idx) {
        gf_limb_t* ai = (idx == 0) ? a0 : (idx == 1) ? a1 : a2;
        gf_limb_t* si = (idx == 0) ? s0 : (idx == 1) ? s1 : s2;
        /* a_i^{2^m} */
        gf_limb_t ai_frobm[GF_LIMBS(512)]; gf_copy(fc, ai_frobm, ai);
        for (unsigned j = 0; j < m; ++j) { gf_limb_t t[GF_LIMBS(512)]; gf_sqr(fc, t, ai_frobm); gf_copy(fc, ai_frobm, t); }
        /* lhs = sigma_i * a_i^{2^m} * a_i */
        gf_limb_t tmp[GF_LIMBS(512)];
        gf_mul(fc, tmp, si, ai_frobm);
        gf_limb_t lhs[GF_LIMBS(512)];
        gf_mul(fc, lhs, tmp, ai);
        /* check lhs == 1 */
        if (memcmp(lhs, one, lb) != 0) return 0;
    }

    /* constraint 3: z * t = 1
       b_i = sigma_i * a_i^{2^m}  (the "S" function)
       z = L0(b0) ^ L1(b1) ^ L2(b2)
       t = y ^ L3(k) */
    gf_limb_t b0[GF_LIMBS(512)], b1[GF_LIMBS(512)], b2[GF_LIMBS(512)];
    /* recompute a_i^{2^m} for each */
    gf_limb_t a0_fm[GF_LIMBS(512)], a1_fm[GF_LIMBS(512)], a2_fm[GF_LIMBS(512)];
    gf_copy(fc, a0_fm, a0); gf_copy(fc, a1_fm, a1); gf_copy(fc, a2_fm, a2);
    for (unsigned j = 0; j < m; ++j) {
        gf_limb_t t[GF_LIMBS(512)];
        gf_sqr(fc, t, a0_fm); gf_copy(fc, a0_fm, t);
        gf_sqr(fc, t, a1_fm); gf_copy(fc, a1_fm, t);
        gf_sqr(fc, t, a2_fm); gf_copy(fc, a2_fm, t);
    }
    gf_mul(fc, b0, s0, a0_fm);
    gf_mul(fc, b1, s1, a1_fm);
    gf_mul(fc, b2, s2, a2_fm);

    gf_limb_t l0o[GF_LIMBS(512)], l1o[GF_LIMBS(512)], l2o[GF_LIMBS(512)];
    galas_apply_lin_map(fc, l0o, b0, P->L0);
    galas_apply_lin_map(fc, l1o, b1, P->L1);
    galas_apply_lin_map(fc, l2o, b2, P->L2);
    gf_limb_t z[GF_LIMBS(512)];
    for (unsigned i = 0; i < fc->nlimbs; ++i) z[i] = l0o[i] ^ l1o[i] ^ l2o[i];

    gf_limb_t l3k[GF_LIMBS(512)];
    galas_apply_lin_map(fc, l3k, K, P->L3);
    gf_limb_t t_val[GF_LIMBS(512)];
    for (unsigned i = 0; i < fc->nlimbs; ++i) t_val[i] = Y[i] ^ l3k[i];

    /* z * t == 1 */
    gf_limb_t zt[GF_LIMBS(512)];
    gf_mul(fc, zt, z, t_val);
    if (memcmp(zt, one, lb) != 0) return 0;

    return 1;  /* all constraints satisfied */
}
