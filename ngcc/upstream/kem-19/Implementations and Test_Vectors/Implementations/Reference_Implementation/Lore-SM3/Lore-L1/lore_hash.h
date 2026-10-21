#ifndef LORE_HASH_H
#define LORE_HASH_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"

/* Versioned call-layer domains; byte lengths are converted only at the API. */
enum lore_hash_domain {
    LORE_DOMAIN_KEYGEN = 1,
    LORE_DOMAIN_MESSAGE = 2,
    LORE_DOMAIN_PUBLIC_KEY = 3,
    LORE_DOMAIN_CIPHERTEXT = 4,
    LORE_DOMAIN_G = 5,
    LORE_DOMAIN_KDF = 6,
    LORE_DOMAIN_SAMPLE = 7,
    LORE_DOMAIN_MATRIX = 8
};

#define LORE_FRAME_BYTES 20
#define lore_frame_header LORE_NAMESPACE(frame_header)
#define lore_hash_h LORE_NAMESPACE(hash_h)
#define lore_hash_g LORE_NAMESPACE(hash_g)
#define lore_kdf LORE_NAMESPACE(kdf)
#define lore_expand_seed LORE_NAMESPACE(expand_seed)

void lore_frame_header(uint8_t out[LORE_FRAME_BYTES], uint8_t domain,
                       uint32_t counter, size_t inlen);
void lore_hash_h(uint8_t out[LORE_SYMBYTES], uint8_t domain,
                 const uint8_t *in, size_t inlen);
void lore_hash_g(uint8_t out[2 * LORE_SYMBYTES],
                 const uint8_t *in, size_t inlen);
void lore_kdf(uint8_t out[LORE_SYMBYTES], const uint8_t *in, size_t inlen);
void lore_expand_seed(uint8_t out[2 * LORE_SYMBYTES],
                      const uint8_t seed[LORE_SYMBYTES]);

#endif
