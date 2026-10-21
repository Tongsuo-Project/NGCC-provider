/**
 * @file symmetric.c
 * @brief Cryptographic primitives: a SHAKE-256–based pseudo-random number generator (PRNG) and extendable-output
 * function (XOF), plus hash functions built on SHA3-256 and SHA3-512.
 */

#include "symmetric.h"
#include <stdint.h>
#include <string.h>
#include "auxfunc.h"

/**
 * @typedef shake256_prng_ctx
 * @brief Incremental SHAKE-256 prng context.
 *
 */
shake256_xof_ctx shake256_prng_ctx;

/**
 * @param[in] entropy_input Pointer to input entropy bytes
 * @param[in] enlen Length of entropy string in bytes
 */
void prng_init(uint8_t *entropy_input, uint32_t enlen) {
    pseudoXOF(CTX_LEN * 8, entropy_input, enlen * 8, (unsigned char *)(&shake256_prng_ctx)->ctx);
}

/**
 *
 * @param[out] output Pointer to output
 * @param[in] outlen length of output in bytes
 */
void prng_get_bytes(uint8_t *output, uint32_t outlen) {
    xof_get_bytes(&shake256_prng_ctx, output, outlen);
}

/**
 *
 * @param[out] xof_ctx   Pointer to the XOF context to be initialized.
 * @param[in]  seed      Pointer to the input seed.
 * @param[in]  seed_size Size of the seed in bytes.
 */
void xof_init(shake256_xof_ctx *xof_ctx, const uint8_t *seed, uint32_t seed_size) {
    pseudoXOF(CTX_LEN * 8, seed, seed_size * 8, (unsigned char *)xof_ctx->ctx);
}

/**
 *
 * @param[out] output      Buffer (32 bytes) to receive the hash output.
 * @param[in]  ek_kem      Encapsulation key of the KEM.
 */
void hash_h(uint8_t *output, const uint8_t ek_kem[PUBLIC_KEY_BYTES]) {
    sm3hash(256, ek_kem, PUBLIC_KEY_BYTES * 8, output);
}

/**
 * @brief Compute the hash function G (SHA3-512) with domain separation.
 *
 * @param[out] output        Buffer (64 bytes) to receive the hash output.
 * @param[in]  hash_ek_kem   Hash of the KEM encapsulation key.
 * @param[in]  m             Message bytes.
 * @param[in]  salt          Salt value.
 */
void hash_g(uint8_t *output, const uint8_t hash_ek_kem[SEED_BYTES], const uint8_t m[PARAM_SECURITY_BYTES],
            const uint8_t salt[SALT_BYTES]) {
    uint8_t msg[SEED_BYTES + PARAM_SECURITY_BYTES + SALT_BYTES];
    memcpy(msg, hash_ek_kem, SEED_BYTES);
    memcpy(msg + SEED_BYTES, m, PARAM_SECURITY_BYTES);
    memcpy(msg + SEED_BYTES + PARAM_SECURITY_BYTES, salt, SALT_BYTES);
    pseudohash((SHARED_SECRET_BYTES + SEED_BYTES) * 8, msg, sizeof(msg) * 8, output);
}

/**
 * @brief Compute the hash function J (SHA3-256) with domain separation.
 *
 * @param[out] output       Buffer (32 bytes) to receive the hash output.
 * @param[in]  hash_ek_kem  Hash of the KEM encapsulation key.
 * @param[in]  sigma        The string sigma.
 * @param[in]  c_kem        Pointer to ciphertext struct (includes c_pke.u, c_pke.v, and salt).
 */
void hash_j(uint8_t *output, const uint8_t hash_ek_kem[SEED_BYTES], const uint8_t sigma[PARAM_SECURITY_BYTES],
            const ciphertext_kem_t *c_kem) {
    uint8_t hash_ek_kem_sigma[SEED_BYTES + PARAM_SECURITY_BYTES];
    memcpy(hash_ek_kem_sigma, hash_ek_kem, SEED_BYTES);
    memcpy(hash_ek_kem_sigma + SEED_BYTES, sigma, PARAM_SECURITY_BYTES);
    uint8_t hash_hash_ek_kem_sigma[SEED_BYTES];
    sm3hash(256, hash_ek_kem_sigma, sizeof(hash_ek_kem_sigma) * 8, hash_hash_ek_kem_sigma);

    uint8_t ciphertext[SALT_BYTES + VEC_N_SIZE_64 * 8 + VEC_N_SIZE_64 * 8];
    memcpy(ciphertext, c_kem->salt, SALT_BYTES);
    memcpy(ciphertext + SALT_BYTES, c_kem->c_pke.u, VEC_N_SIZE_64 * 8);
    memcpy(ciphertext + SALT_BYTES + VEC_N_SIZE_64 * 8, c_kem->c_pke.v, VEC_N_SIZE_64 * 8);
    uint8_t hash_ciphertext[SEED_BYTES];
    sm3hash(256, ciphertext, sizeof(ciphertext) * 8, hash_ciphertext);

    uint8_t msg[SEED_BYTES * 2];
    memcpy(msg, hash_hash_ek_kem_sigma, SEED_BYTES);
    memcpy(msg + SEED_BYTES, hash_ciphertext, SEED_BYTES);
    sm3hash(SHARED_SECRET_BYTES * 8, msg, sizeof(msg) * 8, output);
}


/**
 * @brief Generates bytes from the internal XOF state and updates the state.
 *
 * The current context is copied into a temporary buffer, used to generate outlen bytes, and then refreshed to obtain the next XOF state.
 *
 * @param[in,out] ctx   Internal XOF context.
 * @param[out] h        Output buffer.
 * @param[in] outlen    Number of bytes to generate.
 */
static void xof_proc(uint64_t *ctx, uint8_t *h, size_t outlen) {
    unsigned char aux[CTX_LEN * 8] = {0};
    memcpy(aux, ctx, sizeof(aux));
    pseudoXOF(outlen * 8, aux, sizeof(aux) * 8, (unsigned char *)h);
    pseudoXOF(CTX_LEN * 8, aux, sizeof(aux) * 8, (unsigned char *)ctx);
}


/**
 * @brief Generates bytes from a SHAKE256-like XOF context.
 *
 * @param[in,out] xof_ctx   XOF context.
 * @param[out] output       Output buffer.
 * @param[in] output_size   Number of bytes to generate.
 */
void xof_get_bytes(shake256_xof_ctx *xof_ctx, uint8_t *output, uint32_t output_size) {
    xof_proc(xof_ctx->ctx, output, output_size);
}

