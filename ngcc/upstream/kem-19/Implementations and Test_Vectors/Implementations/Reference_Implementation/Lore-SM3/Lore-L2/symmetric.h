#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include "params.h"
#include "lore_hash.h"
#include <stdint.h>

typedef struct {
    uint8_t seed[LORE_SYMBYTES];
    uint32_t counter;
    size_t outpos;
} xof_state;
#define XOF_BLOCKBYTES 168

#define xof_absorb LORE_NAMESPACE(xof_absorb)
void xof_absorb(xof_state *s, const uint8_t seed[LORE_SYMBYTES], uint8_t x, uint8_t y);

#define xof_squeezeblocks LORE_NAMESPACE(xof_squeezeblocks)
void xof_squeezeblocks(uint8_t *out, size_t outblocks, xof_state *s);

#define prf LORE_NAMESPACE(prf)
void prf(uint8_t *out, size_t outlen, const uint8_t key[LORE_SYMBYTES], uint8_t nonce);


#endif /* SYMMETRIC_H */
