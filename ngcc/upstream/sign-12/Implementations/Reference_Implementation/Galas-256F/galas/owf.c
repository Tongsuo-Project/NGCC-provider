/*
 * owf.c — Gala one-way function (reference). See owf.h.
 */
#include "owf.h"
#include "gf2n.h"
#include <string.h>

/* ---- per-n parameter wiring (pulls from galas_params_<n>.h) ---- */
#include "galas_params_128.h"
#include "galas_params_160.h"
#include "galas_params_256.h"
#include "galas_params_384.h"
#include "galas_params_512.h"

#define MAKE_PARAMS(N) \
    static const galas_owf_params P_##N = { \
        N, (N + 7) / 8, \
        GALAS_##N##_C0, GALAS_##N##_C1, GALAS_##N##_C2, \
        GALAS_##N##_M0, GALAS_##N##_M1, GALAS_##N##_M2, \
        GALAS_##N##_L0, GALAS_##N##_L1, GALAS_##N##_L2, GALAS_##N##_L3, \
    }

MAKE_PARAMS(128);
MAKE_PARAMS(160);
MAKE_PARAMS(256);
MAKE_PARAMS(384);
MAKE_PARAMS(512);

const galas_owf_params* galas_owf_params_for(unsigned n) {
    switch (n) {
        case 128: return &P_128;
        case 160: return &P_160;
        case 256: return &P_256;
        case 384: return &P_384;
        case 512: return &P_512;
        default:  return 0;
    }
}

void galas_apply_lin_map(const gf_ctx* ctx, gf_limb_t* out,
                         const gf_limb_t* x,
                         const uint64_t* coeffs) {
    unsigned n = ctx->n;
    unsigned L = ctx->nlimbs;
    /* acc <- sum_j coeffs[j] * x^{2^j} ; xpow tracks x^{2^j} */
    gf_limb_t acc[GF_LIMBS(512)];
    gf_limb_t xpow[GF_LIMBS(512)];
    gf_limb_t tmp[GF_LIMBS(512)];
    for (unsigned i = 0; i < L; ++i) { acc[i] = 0; xpow[i] = x[i]; }
    for (unsigned j = 0; j < n; ++j) {
        /* coeff_j is the j-th field element: nlimbs u64 starting at coeffs + j*L */
        const uint64_t* cj = coeffs + (size_t)j * L;
        gf_limb_t c[GF_LIMBS(512)];
        for (unsigned i = 0; i < L; ++i) c[i] = cj[i];
        if (!gf_is_zero(ctx, c)) {
            gf_mul(ctx, tmp, c, xpow);
            gf_xor(ctx, acc, tmp);
        }
        /* xpow <- xpow^2 */
        gf_limb_t sq[GF_LIMBS(512)];
        gf_sqr(ctx, sq, xpow);
        for (unsigned i = 0; i < L; ++i) xpow[i] = sq[i];
    }
    for (unsigned i = 0; i < L; ++i) out[i] = acc[i];
}

int galas_owf_eval(const galas_owf_params* P, const gf_ctx* ctx,
                   uint8_t* out, const uint8_t* x, const uint8_t* k) {
    unsigned L = ctx->nlimbs;
    gf_limb_t Xk[GF_LIMBS(512)], K[GF_LIMBS(512)];
    gf_limb_t c0[GF_LIMBS(512)], c1[GF_LIMBS(512)], c2[GF_LIMBS(512)];
    gf_from_bytes(ctx, Xk, x);
    gf_from_bytes(ctx, K, k);
    gf_from_bytes(ctx, c0, P->c0);
    gf_from_bytes(ctx, c1, P->c1);
    gf_from_bytes(ctx, c2, P->c2);

    /* r0 = x^k^c0, r1 = k^c1, r2 = k^c2 */
    gf_limb_t r0[GF_LIMBS(512)], r1[GF_LIMBS(512)], r2[GF_LIMBS(512)];
    for (unsigned i = 0; i < L; ++i) {
        r0[i] = Xk[i] ^ K[i] ^ c0[i];
        r1[i] = K[i] ^ c1[i];
        r2[i] = K[i] ^ c2[i];
    }

    /* a0 = M0(r0), a1 = M1(r1), a2 = M2(r2) */
    gf_limb_t a0[GF_LIMBS(512)], a1[GF_LIMBS(512)], a2[GF_LIMBS(512)];
    galas_apply_lin_map(ctx, a0, r0, P->M0);
    galas_apply_lin_map(ctx, a1, r1, P->M1);
    galas_apply_lin_map(ctx, a2, r2, P->M2);

    /* reject if any wide-S-box input is zero */
    if (gf_is_zero(ctx, a0) || gf_is_zero(ctx, a1) || gf_is_zero(ctx, a2))
        return -1;

    /* b_i = a_i^{-1} */
    gf_limb_t b0[GF_LIMBS(512)], b1[GF_LIMBS(512)], b2[GF_LIMBS(512)];
    gf_inv(ctx, b0, a0);
    gf_inv(ctx, b1, a1);
    gf_inv(ctx, b2, a2);

    /* l_i = L_i(b_i) ; z = l0 ^ l1 ^ l2 */
    gf_limb_t l0[GF_LIMBS(512)], l1[GF_LIMBS(512)], l2[GF_LIMBS(512)];
    galas_apply_lin_map(ctx, l0, b0, P->L0);
    galas_apply_lin_map(ctx, l1, b1, P->L1);
    galas_apply_lin_map(ctx, l2, b2, P->L2);
    gf_limb_t z[GF_LIMBS(512)];
    for (unsigned i = 0; i < L; ++i) z[i] = l0[i] ^ l1[i] ^ l2[i];

    if (gf_is_zero(ctx, z)) return -1;

    /* t = z^{-1} ; lk = L3(k) ; out = t ^ lk */
    gf_limb_t t[GF_LIMBS(512)], lk[GF_LIMBS(512)];
    gf_inv(ctx, t, z);
    galas_apply_lin_map(ctx, lk, K, P->L3);
    gf_limb_t o[GF_LIMBS(512)];
    for (unsigned i = 0; i < L; ++i) o[i] = t[i] ^ lk[i];
    gf_to_bytes(ctx, out, o);
    return 0;
}
