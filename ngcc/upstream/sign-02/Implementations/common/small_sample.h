/* Copyright (c) 2026 Hang Zhang. */
#ifndef BIT_SMALL_SAMPLE_H
#define BIT_SMALL_SAMPLE_H
#include <stdint.h>

/* Map a uniform 3- or 4-bit symbol to {-1, 0, 1} without indexed memory. */
static inline int32_t bit_small_sample(uint32_t symbol)
{
    return (int32_t)(symbol == 1U) - (int32_t)(symbol == 0U);
}
#endif
