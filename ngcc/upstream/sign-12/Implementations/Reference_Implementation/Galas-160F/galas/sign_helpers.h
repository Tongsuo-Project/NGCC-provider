/*
 * sign_helpers.h — Fiat-Shamir challenge derivation helpers for GALAS Sign/Verify.
 *
 * These wrap the H2/H3/H4 oracles into the specific challenge derivations
 * used by the VOLEitH transcript (matching ref-FAEST's hash_mu / hash_r_iv /
 * hash_challenge_1/2/3).
 */
#ifndef GALAS_SIGN_HELPERS_H
#define GALAS_SIGN_HELPERS_H

#include <stdint.h>
#include "instances.h"

/* mu = H2_0(pk || msg) -- 2*lambda bits */
void galas_hash_mu(uint8_t* mu, const uint8_t* pk, unsigned pk_len,
                   const uint8_t* msg, unsigned msg_len, unsigned lambda);

/* derive (rootKey, iv) from H3(sk || mu || rho). rootKey = lambda bytes, iv = IV_SIZE. */
void galas_hash_r_iv(uint8_t* rootkey, uint8_t* iv,
                     const uint8_t* sk, unsigned sk_len,
                     const uint8_t* mu, unsigned mu_len,
                     const uint8_t* rho, unsigned rho_len,
                     unsigned lambda);

#endif
