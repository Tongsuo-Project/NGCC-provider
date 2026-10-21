/* Copyright (c) 2026 Hang Zhang. */
#ifndef BIT_SPECTRAL_TEST_H
#define BIT_SPECTRAL_TEST_H

#include "spectral.h"
#include "packing.h"

static int test_keygen_spectrum(void)
{
    polyvecl s = {0};
    polyveck e = {0}, public_vector;
    unsigned char pk[BIT_PUBLICKEYBYTES], sk[BIT_SECRETKEYBYTES];
    unsigned char seed[BIT_SEEDBYTES], secret[BIT_SEEDBYTES], tr[BIT_TRBYTES], entropy[55];
    unsigned long long pklen = 0, sklen = 0;
    uint64_t bound = bit_keygen_spectral_bound_sq(&s, &e);
    if (bound < BIT_TAU || bound > 2U * BIT_TAU)
        return 1;
    s.vec[0].coeffs[BIT_N / 3] = -1;
    bound = bit_keygen_spectral_bound_sq(&s, &e);
    if (bound < 2U * BIT_TAU || bound > 3U * BIT_TAU)
        return 2;
    for (unsigned i = 0; i < BIT_L; i++)
        for (unsigned j = 0; j < BIT_N; j++)
            s.vec[i].coeffs[j] = 1;
    for (unsigned i = 0; i < BIT_K; i++)
        for (unsigned j = 0; j < BIT_N; j++)
            e.vec[i].coeffs[j] = 1;
    if (bit_keygen_spectral_bound_sq(&s, &e) <= BIT_PARABOLA_BETA2)
        return 3;
    test_fill_message(entropy, sizeof(entropy), 0x632, 0);
    if (init_random_number(&drng_algorithm, entropy, sizeof(entropy)) != 0)
        return 4;
    for (unsigned i = 0; i < 16; i++) {
        if (sig_keygen(pk, &pklen, sk, &sklen) != 0 ||
            unpack_sk(seed, &public_vector, secret, tr, &s, &e, sk) != 0)
            return 5;
        if (bit_keygen_spectral_bound_sq(&s, &e) > BIT_PARABOLA_BETA2)
            return 6;
    }
    bit_secure_zero(sk, sizeof(sk));
    bit_secure_zero(secret, sizeof(secret));
    bit_secure_zero(&s, sizeof(s));
    bit_secure_zero(&e, sizeof(e));
    return 0;
}

#endif
