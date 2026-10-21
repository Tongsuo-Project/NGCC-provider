/*
 * sign_helpers.c — Fiat-Shamir challenge derivations. See sign_helpers.h.
 */
#include "sign_helpers.h"
#include "random_oracle.h"
#include <string.h>

void galas_hash_mu(uint8_t* mu, const uint8_t* pk, unsigned pk_len,
                   const uint8_t* msg, unsigned msg_len, unsigned lambda) {
    galas_H2_ctx ctx;
    galas_H2_init(&ctx, lambda);
    galas_H2_update(&ctx, pk, pk_len);
    galas_H2_update(&ctx, msg, msg_len);
    galas_H2_0_final(&ctx, mu, 2 * (lambda / 8));
}

void galas_hash_r_iv(uint8_t* rootkey, uint8_t* iv,
                     const uint8_t* sk, unsigned sk_len,
                     const uint8_t* mu, unsigned mu_len,
                     const uint8_t* rho, unsigned rho_len,
                     unsigned lambda) {
    galas_H3_ctx ctx;
    galas_H3_init(&ctx, lambda);
    galas_H3_update(&ctx, sk, sk_len);
    galas_H3_update(&ctx, mu, mu_len);
    if (rho && rho_len) galas_H3_update(&ctx, rho, rho_len);
    galas_H3_final(&ctx, rootkey, lambda / 8, iv);
}
