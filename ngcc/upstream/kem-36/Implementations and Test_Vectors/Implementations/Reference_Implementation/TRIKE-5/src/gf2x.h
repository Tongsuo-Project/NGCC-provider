#ifndef GF2X_H
#define GF2X_H


#include <stdlib.h>
#include <string.h>
#include "trike_types.h"

void gf2x_add(uint8_t *c, const uint8_t *a, const uint8_t *b);
int gf2x_mul(uint8_t *c, const uint8_t *a, const uint8_t *b);
int gf2x_inv(uint8_t *inv_a, const uint8_t *a);
int gf2x_shift(uint8_t *out, const uint8_t *in, uint32_t shift);

// Allocates working storage.
static inline void *trike_alloc(size_t size)
{
    return malloc(size);
}

// Clears the array.
static inline void trike_setz(uint8_t *dst, size_t dst_size)
{
    memset(dst, 0, dst_size);
}

// Copies the array.
static inline void fast_cpy(uint64_t *dst, const uint64_t *src, size_t size64)
{
    memcpy(dst, src, size64 * 8);
}

// Copies the array and clears padding.
static inline void trike_assign(const uint8_t *src, uint8_t *dst, size_t src_size, size_t dst_size)
{
    memcpy(dst, src, src_size);
    memset(dst + src_size, 0, dst_size - src_size);
}
#endif
