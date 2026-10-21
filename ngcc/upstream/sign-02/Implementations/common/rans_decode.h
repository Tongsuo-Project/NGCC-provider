/* Copyright (c) 2026 Hang Zhang. */
#ifndef BIT_RANS_DECODE_H
#define BIT_RANS_DECODE_H

#include <stdint.h>

/* Positive frequencies sum to 65536; slot is below 65536.
 * lookup selects the first symbol intersecting its 256-slot bucket. */
static inline unsigned int rans_find_symbol(uint32_t slot, const uint16_t *freq,
                                            const uint16_t *cumulative, const uint8_t *lookup,
                                            uint32_t *start)
{
    unsigned int symbol = lookup[slot >> 8];
    uint32_t base = cumulative[symbol];

    while (slot >= base + freq[symbol])
        base += freq[symbol++];
    *start = base;
    return symbol;
}

/* Each bucket stores its first symbol, cumulative start and frequency. */
static inline unsigned int rans_find_symbol_fast(uint32_t slot, const uint16_t *freq,
                                                 const uint64_t *lookup, uint32_t *start,
                                                 uint32_t *frequency)
{
    uint64_t entry = lookup[slot >> 8];
    unsigned int symbol = (uint8_t)entry;
    uint32_t base = (uint32_t)(entry >> 8) & UINT32_C(0xffff);
    uint32_t f = (uint32_t)(entry >> 24);

    while (slot >= base + f) {
        base += f;
        f = freq[++symbol];
    }
    *start = base;
    *frequency = f;
    return symbol;
}

#endif
