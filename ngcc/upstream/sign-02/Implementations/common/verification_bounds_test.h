/* Copyright (c) 2026 Hang Zhang. */
#ifndef BIT_VERIFICATION_BOUNDS_TEST_H
#define BIT_VERIFICATION_BOUNDS_TEST_H

#include <stdint.h>
#include <string.h>
#include "sample.h"

static int test_verification_bounds(void)
{
    polyvecm1 z = {0};
    polyveck h = {0};
    const unsigned int positions[] = {0, 15, 16, BIT_N - 1};

    for (unsigned int block = 0; block <= BIT_L; block++) {
        for (unsigned int i = 0; i < sizeof(positions) / sizeof(positions[0]); i++) {
            for (int sign = -1; sign <= 1; sign += 2) {
                for (int delta = -1; delta <= 1; delta++) {
                    memset(&z, 0, sizeof(z));
                    z.vec[block].coeffs[positions[i]] = sign * (BIT_Q_HALF + delta);
                    if (check_signature_bounds(&z, &h) != 0 ||
                        (check_signature_verification_bounds(&z, &h) != 0) != (delta > 0))
                        return 1;
                }
            }
        }
    }
    memset(&z, 0, sizeof(z));
    if (!check_signature_verification_bounds(&z, &h))
        return 1;
    z.vec[0].coeffs[0] = 1;
    h.vec[0].coeffs[0] = BIT_H_INF + 1;
    if (!check_signature_verification_bounds(&z, &h))
        return 1;
    memset(&h, 0, sizeof(h));
    for (unsigned int i = 0; i <= BIT_H_L2_BOUND_SQ; i++)
        h.vec[i / BIT_N].coeffs[i % BIT_N] = 1;
    if (!check_signature_verification_bounds(&z, &h))
        return 1;
    memset(&h, 0, sizeof(h));
    for (unsigned int block = 0; block <= BIT_L; block++)
        for (unsigned int j = 0; j < BIT_N; j++)
            z.vec[block].coeffs[j] = BIT_Q_HALF;
    if (!check_signature_verification_bounds(&z, &h))
        return 1;
    for (unsigned int block = 0; block <= BIT_L; block++)
        for (unsigned int j = 0; j < BIT_N; j++)
#if BIT_N == 1024
            z.vec[block].coeffs[j] = INT32_MIN;
#else
            z.vec[block].coeffs[j] = INT16_MIN;
#endif
    return !check_signature_verification_bounds(&z, &h);
}

#endif
