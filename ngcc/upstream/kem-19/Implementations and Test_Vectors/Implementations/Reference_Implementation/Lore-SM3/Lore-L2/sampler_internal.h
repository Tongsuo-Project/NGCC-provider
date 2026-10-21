#ifndef LORE_SAMPLER_INTERNAL_H
#define LORE_SAMPLER_INTERNAL_H

#include <stdint.h>

/* Multiply-high interval sampling. Every accepted index has exactly
 * floor(65536 / bound) preimages. Requires 1 <= bound <= 65535. */
static inline int lore_sample_index16(uint16_t word, uint16_t bound,
                                      uint16_t *index)
{
    uint32_t product = (uint32_t)word * bound;
    uint16_t low = (uint16_t)product;
    if (low < bound && low < (65536u % bound))
        return 0;
    *index = (uint16_t)(product >> 16);
    return 1;
}

#endif
