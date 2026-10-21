/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense, Institute of Information
Engineering, CAS School of Cyber Security, University of Chinese Academy of Sciences
*/
#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>
#include "params.h"

/* For 0 <= a < 2q, return a representative in [0, q). */
static inline int32_t reduce_modq(int32_t a)
{
    a -= BIT_Q;
    a += (a >> 31) & BIT_Q;
    return a;
}

/* Barrett reduction for signed 32-bit inputs, result in [0, q). */
uint32_t reduce_barrett(int64_t a);

/* For |a| <= q + floor(q/2), return a representative in [-floor(q/2), floor(q/2)]. */
static inline int32_t reduce_signed_modq(int64_t a)
{
    a += BIT_Q & ((a + BIT_Q_HALF) >> 63);
    a -= BIT_Q & ((BIT_Q_HALF - a) >> 63);
    return (int32_t)a;
}

/* Format conversion */
static inline uint32_t reduce_to_unsigned(int32_t c)
{
    return (uint32_t)(c + ((c >> 31) & BIT_Q));
}

static inline int32_t reduce_to_signed(uint32_t t)
{
    int64_t t64 = t;
    int64_t mask = ((BIT_Q / 2) - t64) >> 63;
    return (int32_t)(t64 - (BIT_Q & mask));
}

/* Center a signed 32-bit input in [-floor(q/2), floor(q/2)]. */
static inline int32_t reduce_signed_barrett(int64_t a)
{
    return reduce_to_signed(reduce_barrett(a));
}

#endif
