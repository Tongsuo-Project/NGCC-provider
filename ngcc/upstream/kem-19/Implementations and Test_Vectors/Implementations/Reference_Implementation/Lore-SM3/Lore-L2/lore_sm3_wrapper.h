#ifndef LORE_SM3_WRAPPER_H
#define LORE_SM3_WRAPPER_H

#include "lore_hash.h"

/*
 * These wrappers use the unmodified API_PKC functions in auxfunc.c.
 * Their lengths are bytes; the official API takes bits.
 * pseudohash and pseudoXOF are correctness placeholders, NOT 384/512-bit
 * security constructions. Domain separation and long output do not change that.
 */
#define lore_sm3_hash LORE_NAMESPACE(sm3_hash)
#define lore_sm3_xof LORE_NAMESPACE(sm3_xof)
#define lore_sm3_xof_read LORE_NAMESPACE(sm3_xof_read)

int lore_sm3_hash(uint8_t *out, size_t outlen, uint8_t domain, uint32_t counter,
                  const uint8_t *in, size_t inlen);
int lore_sm3_xof(uint8_t *out, size_t outlen, uint8_t domain, uint32_t counter,
                 const uint8_t *in, size_t inlen);
int lore_sm3_xof_read(uint8_t *out, size_t outlen, size_t offset,
                      uint8_t domain, uint32_t counter,
                      const uint8_t *in, size_t inlen);
#endif
