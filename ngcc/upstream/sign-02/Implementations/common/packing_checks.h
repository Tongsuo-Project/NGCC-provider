/* Copyright (c) 2026 Hang Zhang. */
#ifndef BIT_PACKING_CHECKS_H
#define BIT_PACKING_CHECKS_H

#include <stdint.h>
#include <string.h>
#include "params.h"
#include "ct_audit.h"

_Static_assert(((BIT_L + BIT_K) * BIT_N / 4) % 8 == 0,
               "packed small-secret length must contain whole words");

/* Packed s/e codes 00, 01 and 11 encode 0, 1 and -1; 10 is reserved.
 * memcpy allows unaligned input; the bit test is byte-order independent. */
static inline int small2_encoding_invalid(const unsigned char *in)
{
    uint64_t bad = 0;
    for (size_t i = 0; i < (BIT_L + BIT_K) * BIT_N / 4; i += 8) {
        uint64_t packed;
        memcpy(&packed, in + i, sizeof packed);
        bad |= (packed >> 1) & ~packed;
    }
    uint32_t invalid = (bad & UINT64_C(0x5555555555555555)) != 0;
    /* Constant for valid generated keys; decoded coefficients stay secret. */
    BIT_CT_DECLASSIFY(&invalid, sizeof invalid);
    return (int)invalid;
}
#endif
