/* Copyright (c) 2026 Hang Zhang. */
#ifndef BIT_CHALLENGE_BINDING_TEST_H
#define BIT_CHALLENGE_BINDING_TEST_H
#include <string.h>
#include "symmetric.h"

static int test_challenge_binding(void)
{
    static const size_t lengths[] = {0, 1, 50, 51, 52, 71, 72, 73, 122, 123, 124, 135, 136, 137, 143, 144, 145, 4096};
    uint8_t tr[BIT_TRBYTES], message[4096], commitment[BIT_POLYVECK_W1_BYTES];
    uint8_t input[BIT_TRBYTES + 8 + sizeof(message) + BIT_POLYVECK_W1_BYTES];
    uint8_t expected[BIT_CHALLENGEBYTES], first[BIT_CHALLENGEBYTES], again[BIT_CHALLENGEBYTES];
    for (size_t i = 0; i < sizeof(tr); i++)
        tr[i] = (uint8_t)(3 * i + 1);
    for (size_t i = 0; i < sizeof(message); i++)
        message[i] = (uint8_t)(7 * i + 2);
    for (size_t i = 0; i < sizeof(commitment); i++)
        commitment[i] = (uint8_t)(11 * i + 3);
    for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
        size_t length = lengths[i], off = 0;
        bit_challenge_ctx ctx = {0};
        memcpy(input + off, tr, sizeof(tr));
        off += sizeof(tr);
        for (unsigned j = 0; j < 8; j++)
            input[off++] = (uint8_t)((uint64_t)length >> (8 * j));
        memcpy(input + off, message, length);
        off += length;
        memcpy(input + off, commitment, sizeof(commitment));
        off += sizeof(commitment);
        if (bit_hash(expected, input, off) != 0 ||
            bit_challenge_init(&ctx, tr, length ? message : NULL, length) != 0)
            return 1;
        if (bit_challenge_hash(first, &ctx, commitment) != 0 ||
            memcmp(expected, first, sizeof(first)) != 0) {
            bit_challenge_release(&ctx);
            return 2;
        }
        commitment[19] ^= 1;
        if (bit_challenge_hash(again, &ctx, commitment) != 0 ||
            memcmp(first, again, sizeof(first)) == 0) {
            bit_challenge_release(&ctx);
            return 3;
        }
        commitment[19] ^= 1;
        if (bit_challenge_hash(again, &ctx, commitment) != 0 ||
            memcmp(first, again, sizeof(first)) != 0) {
            bit_challenge_release(&ctx);
            return 4;
        }
        bit_challenge_release(&ctx);
    }
    return 0;
}
#endif
