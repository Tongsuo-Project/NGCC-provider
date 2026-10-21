/* Copyright (c) 2026 Hang Zhang. */
#ifndef BIT_TEST_SECRET_ENCODING_H
#define BIT_TEST_SECRET_ENCODING_H
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "packing.h"

static int test_secret_encoding(void)
{
    unsigned char sk[BIT_SECRETKEYBYTES] = {0};
    unsigned char seed[BIT_SEEDBYTES], secret[BIT_SEEDBYTES], tr[BIT_TRBYTES];
    polyvecl s;
    polyveck public_vector, e;
    const size_t offset = 2 * BIT_SEEDBYTES + BIT_POLYVECK_PUBLIC_BYTES + BIT_TRBYTES;
    const size_t length = (BIT_L + BIT_K) * BIT_N / 4;

    /* Exhaust every packed byte; validity is decided coefficient by coefficient. */
    for (unsigned int p = 0; p < BIT_L + BIT_K; p++) {
        const size_t positions[] = {0, BIT_N / 8, BIT_N / 4 - 1};
        for (unsigned int k = 0; k < 3; k++) {
            size_t pos = p * (BIT_N / 4) + positions[k];
            for (unsigned int byte = 0; byte < 256; byte++) {
                int invalid = 0;
                for (unsigned int lane = 0; lane < 4; lane++)
                    invalid |= ((byte >> (2 * lane)) & 3U) == 2;
                sk[offset + pos] = (unsigned char)byte;
                int rc = unpack_sk(seed, &public_vector, secret, tr, &s, &e, sk);
                if ((rc != 0) != invalid) {
                    fprintf(stderr, "secret encoding accepted/rejected incorrectly: pos=%zu byte=%u\n",
                            pos, byte);
                    return 1;
                }
                if (!invalid) {
                    const poly *out = p < BIT_L ? &s.vec[p] : &e.vec[p - BIT_L];
                    for (unsigned int lane = 0; lane < 4; lane++) {
                        unsigned int code = (byte >> (2 * lane)) & 3U;
                        int expected = code == 3 ? -1 : (int)code;
                        if (out->coeffs[4 * positions[k] + lane] != expected)
                            return 1;
                    }
                }
            }
            sk[offset + pos] = 0;
        }
    }
    /* Put the forbidden code at every coefficient, including SIMD boundaries. */
    for (size_t i = 0; i < length; i++) {
        for (unsigned int lane = 0; lane < 4; lane++) {
            sk[offset + i] = (unsigned char)(2U << (2 * lane));
            if (unpack_sk(seed, &public_vector, secret, tr, &s, &e, sk) == 0)
                return 1;
        }
        sk[offset + i] = 0;
    }
    return 0;
}
#endif
