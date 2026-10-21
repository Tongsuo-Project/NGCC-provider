#include "symmetric.h"
#include "lore_sm3_wrapper.h"
#include <stdlib.h>
#include <string.h>

void xof_absorb(xof_state *s, const uint8_t seed[LORE_SYMBYTES], uint8_t x, uint8_t y)
{
    memcpy(s->seed, seed, LORE_SYMBYTES);
    s->counter = ((uint32_t)x << 8) | y;
    s->outpos = 0;
}

void xof_squeezeblocks(uint8_t *out, size_t outblocks, xof_state *s)
{
    if (outblocks > SIZE_MAX / XOF_BLOCKBYTES) abort();
    const size_t length = outblocks * XOF_BLOCKBYTES;
    if (lore_sm3_xof_read(out, length, s->outpos, LORE_DOMAIN_MATRIX,
                          s->counter, s->seed, LORE_SYMBYTES) != 0) abort();
    s->outpos += length;
}

void prf(uint8_t *out, size_t outlen, const uint8_t key[LORE_SYMBYTES], uint8_t nonce)
{
    if (lore_sm3_xof(out, outlen, LORE_DOMAIN_SAMPLE, nonce,
                     key, LORE_SYMBYTES) != 0) abort();
}
