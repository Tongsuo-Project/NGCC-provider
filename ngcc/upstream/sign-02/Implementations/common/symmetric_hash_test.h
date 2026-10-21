/* Copyright (c) 2026 Hang Zhang. */
#ifndef BIT_SYMMETRIC_HASH_TEST_H
#define BIT_SYMMETRIC_HASH_TEST_H

#include <string.h>
#include "symmetric.h"
#if !BIT_USE_SHAKE
#include "auxfunc.h"
#endif

static int test_template_hash(uint8_t *out, const uint8_t *input, size_t length)
{
#if BIT_USE_SHAKE
#if BIT_TRBYTES == 32
    sha3_256(out, input, length);
#else
    sha3_512(out, input, length);
#endif
    return 0;
#else
#if BIT_TRBYTES == 32
    return sm3hash(256, input, (unsigned long long)length * 8ULL, out);
#else
    return pseudohash(512, input, (unsigned long long)length * 8ULL, out);
#endif
#endif
}

static int test_symmetric_hashes(void)
{
    static const size_t lengths[] = {0, 1, 63, 64, 65, 4095, 4096, 4097, 8192};
    uint8_t message[8192], prefix[BIT_TRBYTES];
    uint8_t input[sizeof(prefix) + sizeof(message)];
    uint8_t expected[BIT_TRBYTES], actual[BIT_TRBYTES];
    uint8_t xof_expected[96], xof_actual[96];

    for (size_t i = 0; i < sizeof(message); i++)
        message[i] = (uint8_t)(5 * i + 3);
    for (size_t i = 0; i < sizeof(prefix); i++)
        prefix[i] = (uint8_t)(7 * i + 1);
    memcpy(input, prefix, sizeof(prefix));

    for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
        size_t length = lengths[i];
        if (test_template_hash(expected, message, length) != 0 ||
            bit_hash(actual, message, length) != 0 ||
            memcmp(expected, actual, sizeof(expected)) != 0)
            return 1;

        memcpy(input + sizeof(prefix), message, length);
        if (test_template_hash(expected, input, sizeof(prefix) + length) != 0 ||
            bit_hash_message(actual, prefix, sizeof(prefix), message, length) != 0 ||
            memcmp(expected, actual, sizeof(expected)) != 0)
            return 2;

#if BIT_USE_SHAKE
        shake256(xof_expected, sizeof(xof_expected), message, length);
#else
        if (pseudoXOF(sizeof(xof_expected) * 8ULL, message,
                      (unsigned long long)length * 8ULL, xof_expected) != 0)
            return 3;
#endif
        if (bit_xof256(xof_actual, sizeof(xof_actual), message, length) != 0 ||
            memcmp(xof_expected, xof_actual, sizeof(xof_expected)) != 0)
            return 4;
    }
    return 0;
}
#endif
