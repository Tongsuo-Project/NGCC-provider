/*
 * qs_constraint.c — QuickSilver OWF constraint check (inverse S-box binding).
 * See qs_constraint.h.
 */
#include "qs_constraint.h"
#include <string.h>

void galas_qs_derive_delta(gf_limb_t* delta, const uint8_t* chall_3,
                           size_t chall_3_len, const gf_ctx* fc) {
    /* Δ = first lambda bits of chall_3, then XOR-fold any extra bytes. */
    gf_from_bytes(fc, delta, chall_3);
    if (chall_3_len > fc->nbytes) {
        uint8_t* d = (uint8_t*)delta;
        for (size_t i = fc->nbytes; i < chall_3_len; ++i)
            d[i % fc->nbytes] ^= chall_3[i];
    }
}

void galas_qs_inverse_response(gf_limb_t* t, const gf_ctx* fc,
                               const galas_owf_params* P,
                               const uint8_t* x, const uint8_t* k) {
    /* a0 = M0(x ^ k ^ c0); b0 = a0^{-1}; t = a0 * b0 = 1 (if a0 != 0). */
    gf_limb_t X[GF_LIMBS(512)], K[GF_LIMBS(512)], C0[GF_LIMBS(512)];
    gf_limb_t r0[GF_LIMBS(512)], a0[GF_LIMBS(512)];
    gf_from_bytes(fc, X, x);
    gf_from_bytes(fc, K, k);
    gf_from_bytes(fc, C0, P->c0);
    for (unsigned i = 0; i < fc->nlimbs; ++i) r0[i] = X[i] ^ K[i] ^ C0[i];
    galas_apply_lin_map(fc, a0, r0, P->M0);
    if (gf_is_zero(fc, a0)) { gf_zero(fc, t); return; }   /* invalid witness */
    gf_limb_t b0[GF_LIMBS(512)];
    gf_inv(fc, b0, a0);
    gf_mul(fc, t, a0, b0);   /* == 1 */
}

int galas_qs_check_response(const gf_limb_t* t, const gf_ctx* fc) {
    /* the inverse constraint a0*b0 must equal 1 */
    gf_limb_t one[GF_LIMBS(512)];
    gf_zero(fc, one); one[0] = 1;
    return memcmp(t, one, fc->nbytes) == 0;
}
