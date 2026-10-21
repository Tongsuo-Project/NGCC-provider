/* Copyright (c) 2026 Hang Zhang. */
#ifndef BIT_TEST_JOINT_L2_BOUND_H
#define BIT_TEST_JOINT_L2_BOUND_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sample.h"

/* Construct exact integer norms without floating-point rounding. */
static int joint_test_response(polyvecm1 *z, uint64_t norm, unsigned int offset)
{
    unsigned int used = 0;
    memset(z, 0, sizeof(*z));
    while (norm != 0) {
        uint32_t lo = 1, hi = BIT_RESPONSE_COEFF_BOUND;
        while (lo < hi) {
            uint32_t mid = lo + (hi - lo + 1) / 2;
            if ((uint64_t)mid * mid <= norm)
                lo = mid;
            else
                hi = mid - 1;
        }
        if (used == (BIT_L + 1) * BIT_N)
            return 1;
        unsigned int pos = (17 * used + offset) % ((BIT_L + 1) * BIT_N);
        z->vec[pos / BIT_N].coeffs[pos % BIT_N] =
            (used & 1) ? -(int32_t)lo : (int32_t)lo;
        norm -= (uint64_t)lo * lo;
        used++;
    }
    return 0;
}

static int test_joint_l2_bound(void)
{
    polyvecm1 z;
    polyveck h;
    const uint64_t weight = (uint64_t)BIT_GAMMA2 * BIT_GAMMA2;
#if BIT_SECURITY_BITS == 128
    const uint64_t bound = UINT64_C(9469) * 9469;
#elif BIT_SECURITY_BITS == 256
    const uint64_t bound = UINT64_C(17735) * 17735;
#else
    const uint64_t bound = UINT64_C(81158) * 81158;
#endif

    /* Check both sides and equality, with zero and maximal legal hint norm.
     * Moving the nonzero coordinates exercises every response block and lane. */
    for (unsigned int max_hint = 0; max_hint < 2; max_hint++) {
        unsigned int hint_sq = max_hint ? BIT_H_L2_BOUND_SQ : 0;
        memset(&h, 0, sizeof(h));
        for (unsigned int i = 0; i < hint_sq; i++)
            h.vec[i / BIT_N].coeffs[i % BIT_N] = (i & 1) ? -1 : 1;
        for (unsigned int block = 0; block < BIT_L + 1; block++) {
            for (int delta = -1; delta <= 1; delta++) {
                uint64_t target = bound - weight * hint_sq - 1 + (unsigned int)(delta + 1);
                if (joint_test_response(&z, target, block * BIT_N + 15))
                    return 1;
                if ((check_signature_bounds(&z, &h) != 0) != (delta > 0)) {
                    fprintf(stderr, "joint L2 boundary mismatch: hint=%u block=%u delta=%d\n",
                            hint_sq, block, delta);
                    return 1;
                }
            }
        }
    }
    memset(&h, 0, sizeof(h));
    /* Legal coefficient bounds alone must not permit an oversized response. */
    for (unsigned int i = 0; i < BIT_L + 1; i++)
        for (unsigned int j = 0; j < BIT_N; j++)
            z.vec[i].coeffs[j] = BIT_RESPONSE_COEFF_BOUND;
    if (!check_signature_bounds(&z, &h))
        return 1;
    /* Malformed input must neither overflow the accumulator nor be accepted. */
    for (unsigned int i = 0; i < BIT_L + 1; i++)
        for (unsigned int j = 0; j < BIT_N; j++)
#if BIT_N == 1024
            z.vec[i].coeffs[j] = INT32_MIN;
#else
            z.vec[i].coeffs[j] = INT16_MIN;
#endif
    if (!check_signature_bounds(&z, &h))
        return 1;
    for (int k = 1; k <= BIT_RESPONSE_COEFF_BOUND / BIT_Q; k++) {
        for (int sign = -1; sign <= 1; sign += 2) {
            memset(&z, 0, sizeof(z));
            z.vec[0].coeffs[0] = sign * k * BIT_Q;
            if (!check_signature_bounds(&z, &h))
                return 1;
        }
    }
    return 0;
}
#endif
