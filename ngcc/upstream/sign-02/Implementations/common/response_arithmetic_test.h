/* Copyright (c) 2026 Hang Zhang. */
#ifndef BIT_RESPONSE_ARITHMETIC_TEST_H
#define BIT_RESPONSE_ARITHMETIC_TEST_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "poly.h"

static int test_response_arithmetic(void)
{
    poly a, high, centered;
    poly_ntt expected, actual;
    uint8_t packed[BIT_POLY_W1_BYTES + 1];

    for (uint32_t n = 0; n <= BIT_Q - 1 + BIT_GAMMA2 / 2; n++)
        if (((uint64_t)n * BIT_GAMMA2_BARRETT_MULT >> BIT_GAMMA2_BARRETT_SHIFT)
            != n / BIT_GAMMA2)
            return 1;

    for (int base = 0; base < BIT_Q; base += BIT_N) {
        for (int j = 0; j < BIT_N; j++)
            a.coeffs[j] = (base + j) % BIT_Q;
        poly_highbits(&high, &a);
        for (int j = 0; j < BIT_N; j++) {
            int value = (a.coeffs[j] + BIT_GAMMA2 / 2) / BIT_GAMMA2;
            if (high.coeffs[j] != value)
                return 1;
            decompose_hint(&centered.coeffs[j], a.coeffs[j]);
            if (centered.coeffs[j] != value)
                return 1;
        }
        memset(packed, 0xA5, sizeof(packed));
        poly_pack_w1(packed, &high);
        if (packed[BIT_POLY_W1_BYTES] != 0xA5)
            return 1;
        for (int j = 0; j < BIT_N; j++) {
            unsigned int value = 0;
            for (unsigned int k = 0; k < BIT_W1_BITS; k++) {
                unsigned int bit = j * BIT_W1_BITS + k;
                value |= ((packed[bit / 8] >> (bit % 8)) & 1U) << k;
            }
            if (value != (unsigned int)high.coeffs[j])
                return 1;
        }
    }

    for (int base = -BIT_RESPONSE_COEFF_BOUND;
         base <= BIT_RESPONSE_COEFF_BOUND; base += BIT_N) {
        for (int j = 0; j < BIT_N; j++) {
            int value = base + j;
            a.coeffs[j] = value <= BIT_RESPONSE_COEFF_BOUND ? value : 0;
        }
        centered = a;
        poly_to_ntt(&expected, &a);
        poly_to_ntt_centered(&actual, &centered);
        if (memcmp(&actual, &expected, sizeof(actual)) != 0)
            return 1;
        for (int j = 0; j < BIT_N; j++) {
            int value = a.coeffs[j] % BIT_Q;
            if (value < 0)
                value += BIT_Q;
            if (value > BIT_Q_HALF)
                value -= BIT_Q;
            if (centered.coeffs[j] != value)
                return 1;
        }
    }
    return 0;
}
#endif
