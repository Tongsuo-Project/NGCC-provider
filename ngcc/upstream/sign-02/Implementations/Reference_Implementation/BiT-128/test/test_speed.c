/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information
Engineering, CAS School of Cyber Security, University of Chinese Academy of Sciences
*/
#include <stdint.h>
#include <stdio.h>
#include "../SIG_AlgorithmInstance.h"
#include "../params.h"
#include "../poly.h"
#include "../polyvec.h"
#include "../drng.h"
#include "../sample.h"
#include "../parabola.h"
#include "cpucycles.h"
#include "speed_print.h"

DRNG_ctx drng_algorithm;

#ifndef NTESTS
#define NTESTS 1000
#endif

uint64_t t[NTESTS];

int main(void)
{
    unsigned int i;
    unsigned long long pk_len;
    unsigned long long sk_len;
    unsigned long long sig_len;
    uint8_t pk[BIT_PUBLICKEYBYTES];
    uint8_t sk[BIT_SECRETKEYBYTES];
    uint8_t msg[BIT_MESSAGEBYTES] = {0};
    uint8_t sm[BIT_SIGNBYTES];
    uint8_t seed[55] = {0};
    uint8_t bit_seed[BIT_SEEDBYTES] = {0};
    uint8_t mask_seed[BIT_SEEDBYTES] = {0};
    uint8_t challenge[BIT_CHALLENGEBYTES] = {0};
    uint16_t nonce;
    poly_matrix_ntt mat_ntt;
    poly a = {0};
    poly b = {0};
    poly c = {0};
    bit_parabola_fixed y_fixed = {{{0}}};
    poly_ntt a_ntt = {0};
    poly_ntt b_ntt = {0};
    poly_ntt c_ntt = {0};

    if (init_random_number(&drng_algorithm, seed, sizeof(seed)) != 0) {
        return 1;
    }
    for (i = 0; i < BIT_SEEDBYTES; i++) {
        bit_seed[i] = (uint8_t)i;
    }
    for (i = 0; i < BIT_CHALLENGEBYTES; i++) {
        challenge[i] = (uint8_t)(i + 1);
    }
    nonce = 0;
    poly_sample_S1(&a, bit_seed, &nonce);
    poly_sample_S1(&b, bit_seed, &nonce);
    poly_to_ntt(&a_ntt, &a);
    poly_to_ntt(&b_ntt, &b);

    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        poly_matrix_expand_ntt(&mat_ntt, bit_seed);
    }
    print_results("polyvec_matrix_expand:", t, NTESTS);

    nonce = 0;
    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        poly_sample_S1(&a, bit_seed, &nonce);
    }
    print_results("secret_s1_sampler:", t, NTESTS);

    for (i = 0; i < NTESTS; ++i) {
        /* Give every sample its own nonce domain, even for long benchmarks. */
        for (unsigned int j = 0; j < 4; j++)
            mask_seed[j] = (uint8_t)(i >> (8 * j));
        nonce = 0;
        t[i] = cpucycles();
        if (bit_parabola_sample_fixed(&y_fixed, mask_seed, &nonce) != 0) {
            fprintf(stderr, "mask sampling failed at iteration %u\n", i);
            return 1;
        }
    }
    print_results("mask_paraboloid_q13:", t, NTESTS);

    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        poly_to_ntt(&a_ntt, &a);
    }
    print_results("poly_ntt:", t, NTESTS);

    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        poly_from_ntt(&a, &a_ntt);
    }
    print_results("poly_invntt_tomont:", t, NTESTS);

    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        poly_ntt_basemul_raw(&c_ntt, &a_ntt, &b_ntt);
    }
    print_results("poly_pointwise_raw:", t, NTESTS);

    poly_ntt_basemul_raw(&c_ntt, &a_ntt, &b_ntt);
    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        poly_ntt_montgomery_lift(&c_ntt);
    }
    print_results("poly_montgomery_lift:", t, NTESTS);

    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        poly_ntt_basemul_raw(&c_ntt, &a_ntt, &b_ntt);
        poly_ntt_montgomery_lift(&c_ntt);
    }
    print_results("poly_pointwise_montgomery:", t, NTESTS);

    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        poly_challenge(&c, challenge);
    }
    print_results("poly_challenge:", t, NTESTS);

    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        if (sig_keygen(pk, &pk_len, sk, &sk_len) != 0)
            return 1;
    }
    print_results("Keypair:", t, NTESTS);

    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        if (sig_sign(sk, sk_len, msg, sizeof(msg), sm, &sig_len) != 0)
            return 1;
    }
    print_results("Sign:", t, NTESTS);

    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        if (sig_verify(pk, pk_len, sm, sig_len, msg, sizeof(msg)) != 0)
            return 1;
    }
    print_results("Verify:", t, NTESTS);

    return 0;
}
