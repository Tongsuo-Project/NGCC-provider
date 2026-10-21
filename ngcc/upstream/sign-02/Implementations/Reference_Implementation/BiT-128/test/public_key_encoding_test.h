/* Copyright (c) 2026 Hang Zhang. */
#ifndef BIT_TEST_PUBLIC_KEY_ENCODING_H
#define BIT_TEST_PUBLIC_KEY_ENCODING_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "packing.h"
#include "polyvec.h"

static int test_public_key_canonicalization(void)
{
    poly b;
    for (int base = -BIT_Q_HALF; base <= BIT_Q_HALF; base += BIT_N) {
        for (int j = 0; j < BIT_N; j++)
            b.coeffs[j] = (base + j <= BIT_Q_HALF) ? base + j : 0;
        poly_canonicalize(&b);
        for (int j = 0; j < BIT_N; j++) {
            int input = (base + j <= BIT_Q_HALF) ? base + j : 0;
            int expected = input < 0 ? input + BIT_Q : input;
            if (b.coeffs[j] != expected)
                return 1;
        }
    }
    return 0;
}

static int test_public_key_wire_format(void)
{
    unsigned char pk[BIT_PUBLICKEYBYTES + 16];
    unsigned char seed[BIT_SEEDBYTES], decoded_seed[BIT_SEEDBYTES];
    polyveck p = {0}, decoded;

    for (int i = 0; i < BIT_SEEDBYTES; i++)
        seed[i] = (unsigned char)(17 * i + 3);
    for (int base = 0; base < BIT_Q; base += BIT_N) {
        for (int i = 0; i < BIT_K; i++)
            for (int j = 0; j < BIT_N; j++)
                p.vec[i].coeffs[j] = (int16_t)((base + j + i) % BIT_Q);
        memset(pk, 0xa5, sizeof(pk));
        pack_pk(pk, seed, &p);
        if (unpack_pk(decoded_seed, &decoded, pk) ||
            memcmp(&p, &decoded, sizeof(p)) || memcmp(seed, decoded_seed, sizeof(seed)))
            return 1;
        /* Independent bit-by-bit decoding fixes the wire format. */
        for (int i = 0; i < BIT_K; i++) {
            for (int j = 0; j < BIT_N; j++) {
                unsigned int value = 0;
                size_t bit = ((size_t)i * BIT_N + j) * BIT_PUBLIC_COEFF_BITS;
                for (int k = 0; k < BIT_PUBLIC_COEFF_BITS; k++)
                    value |= ((pk[BIT_SEEDBYTES + (bit + k) / 8] >>
                               ((bit + k) % 8)) & 1U) << k;
                if (value != (unsigned int)p.vec[i].coeffs[j])
                    return 1;
            }
        }
        for (size_t k = BIT_PUBLICKEYBYTES; k < sizeof(pk); k++)
            if (pk[k] != 0xa5)
                return 1;
    }
    for (int pos = 0; pos < BIT_K * BIT_N; pos++) {
        size_t bit = (size_t)BIT_PUBLIC_COEFF_BITS * pos;
        memset(pk, 0, sizeof(pk));
        for (int k = 0; k < BIT_PUBLIC_COEFF_BITS; k++)
            pk[BIT_SEEDBYTES + (bit + k) / 8] |=
                ((BIT_Q >> k) & 1U) << ((bit + k) % 8);
        if (unpack_pk(decoded_seed, &decoded, pk) == 0)
            return 1;
    }
    return 0;
}

static int test_response_alphabet(void)
{
    unsigned char sig[BIT_SIGNBYTES], c[BIT_CHALLENGEBYTES] = {0};
    unsigned char decoded_c[BIT_CHALLENGEBYTES];
    polyvecm1 z = {0}, decoded_z;
    polyveck h = {0}, decoded_h;
    const int high_bound = (BIT_RESPONSE_COEFF_BOUND + 128) / 256;

    for (int block = 0; block <= BIT_L; block++) {
        for (int high = -high_bound; high <= high_bound; high++) {
            int value = high * 256;
            if (value > BIT_RESPONSE_COEFF_BOUND)
                value = BIT_RESPONSE_COEFF_BOUND;
            if (value < -BIT_RESPONSE_COEFF_BOUND)
                value = -BIT_RESPONSE_COEFF_BOUND;
            memset(&z, 0, sizeof(z));
            z.vec[block].coeffs[15] = (int16_t)value;
            if (pack_sig(sig, &z, &h, c) ||
                unpack_sig(&decoded_z, &decoded_h, decoded_c, sig) ||
                memcmp(&z, &decoded_z, sizeof(z)) ||
                memcmp(&h, &decoded_h, sizeof(h)) ||
                memcmp(c, decoded_c, sizeof(c)))
                return 1;
        }
    }
    return 0;
}
#endif
