#ifndef SAMPLE_H
#define SAMPLE_H

#include "trike_types.h"

// Checks secret-key indices for weak keys.
int trike_weak_key_test(const uint32_t *a, const uint32_t *b, const uint32_t *c,
                        uint32_t cycle_limit, uint32_t overlap_limit);

// Generates the secret key polynomials from a seed.
int generate_secret_key(uint8_t *h0, uint8_t *h1, uint8_t *h2, uint32_t *h0_idx, uint32_t *h1_idx, uint32_t *h2_idx, const uint8_t *seed);

// Generates hash-derived vectors from a seed.
int generate_hash_vectors(uint8_t *t1, uint8_t *t2, uint8_t *r1, const uint8_t *seed);

// Generates an error vector from msg || r2 || salt.
int generate_error_vector(uint8_t *e0, uint8_t *e1, uint8_t *e2, const uint8_t *msg, const uint8_t *r2, const uint8_t *salt);

#endif
