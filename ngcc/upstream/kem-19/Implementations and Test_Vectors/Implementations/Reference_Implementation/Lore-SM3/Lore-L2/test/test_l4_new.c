#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../api.h"
#include "../bch_codec.h"
#include "../poly.h"
#include "../randombytes.h"
#include "../sampler.h"
#include "../encoding.h"

#if LORE_LEVEL != 4 || LORE_N != 1024 || LORE_K != 2
#error "This test requires the new Lore-512 parameters"
#endif

static int equivalent_mod_q(int32_t a, int32_t b) {
    int32_t d = (a - b) % LORE_Q;
    return d == 0;
}

static int test_add_modt(void) {
    poly a = {0}, b = {0}, result;
    a.coeffs[0] = -1;
    b.coeffs[0] = 1;
    poly_add_modt(&result, &a, &b);
    for (int i = 0; i < LORE_N; ++i) {
        if (result.coeffs[i] != 0) {
            fprintf(stderr, "mod-t addition leaked a carry into coefficient %d\n", i);
            return 1;
        }
    }
    for (int i = 0; i < LORE_N; ++i) {
        a.coeffs[i] = (int16_t)((i % LORE_T) - 1);
        b.coeffs[i] = (int16_t)(((3 * i + 1) % LORE_T) - 1);
    }
    poly_add_modt(&result, &a, &b);
    for (int i = 0; i < LORE_N; ++i) {
        if (result.coeffs[i] != sym_mod(a.coeffs[i] + b.coeffs[i], LORE_T)) {
            fprintf(stderr, "mod-t addition mismatch at coefficient %d\n", i);
            return 1;
        }
    }
    poly_add_modt(&a, &a, &b);
    for (int i = 0; i < LORE_N; ++i) {
        if (a.coeffs[i] != result.coeffs[i]) {
            fprintf(stderr, "in-place mod-t addition mismatch at coefficient %d\n", i);
            return 1;
        }
    }
    return 0;
}

static int test_monomial_products(void) {
    /* x^i * x^(N-1) checks every position, including every wrap sign. */
    for (int i = 0; i < LORE_N; ++i) {
        poly a = {0}, b = {0}, product, a_ntt, b_ntt;
        a.coeffs[i] = 1;
        b.coeffs[LORE_N - 1] = 1;
        a_ntt = a;
        b_ntt = b;
        poly_ntt(&a_ntt);
        poly_ntt(&b_ntt);
        poly_pointwise_montgomery(&product, &a_ntt, &b_ntt);
        poly_invntt_tomont(&product);
        int position = i == 0 ? LORE_N - 1 : i - 1;
        int sign = i == 0 ? 1 : -1;
        for (int j = 0; j < LORE_N; ++j)
            if (!equivalent_mod_q(product.coeffs[j], j == position ? sign : 0))
                return 1;
        poly_mul_modt(&product, &a, &b);
        for (int j = 0; j < LORE_N; ++j)
            if (sym_mod(product.coeffs[j], LORE_T) !=
                sym_mod(j == position ? sign : 0, LORE_T)) return 1;
    }
    return 0;
}

static int test_ntt_product(void) {
    for (int trial = 0; trial < 64; ++trial) {
        poly a, b, c, a_ntt, b_ntt;
        int32_t reference[LORE_N] = {0};
        uint8_t bytes[4 * LORE_N];
        randombytes(bytes, sizeof bytes);
        for (int i = 0; i < LORE_N; ++i) {
            if (trial < 32) {
                a.coeffs[i] = (int16_t)((int)(bytes[i] % 9) - 4);
                b.coeffs[i] = (int16_t)((int)(bytes[LORE_N + i] % 9) - 4);
            } else {
                uint16_t av = (uint16_t)(bytes[2 * i] | ((uint16_t)bytes[2 * i + 1] << 8));
                uint16_t bv = (uint16_t)(bytes[2 * LORE_N + 2 * i] |
                                         ((uint16_t)bytes[2 * LORE_N + 2 * i + 1] << 8));
                a.coeffs[i] = (int16_t)((av % LORE_Q) - LORE_Q / 2);
                b.coeffs[i] = (int16_t)((bv % LORE_Q) - LORE_Q / 2);
            }
        }
        for (int i = 0; i < LORE_N; ++i) {
            for (int j = 0; j < LORE_N; ++j) {
                int idx = i + j;
                int32_t term = (int32_t)a.coeffs[i] * b.coeffs[j];
                if (idx >= LORE_N) reference[idx - LORE_N] -= term;
                else reference[idx] += term;
            }
        }
        a_ntt = a;
        b_ntt = b;
        poly_ntt(&a_ntt);
        poly_ntt(&b_ntt);
        poly_pointwise_montgomery(&c, &a_ntt, &b_ntt);
        poly_invntt_tomont(&c);
        for (int i = 0; i < LORE_N; ++i) {
            if (!equivalent_mod_q(c.coeffs[i], reference[i])) {
                fprintf(stderr, "NTT product mismatch at trial %d coefficient %d: %d != %d mod %d\n",
                        trial, i, c.coeffs[i], reference[i], LORE_Q);
                return 1;
            }
        }
    }
    return 0;
}

static int test_modt_product(void) {
    for (int trial = 0; trial < 16; ++trial) {
        poly a, b, c;
        int32_t reference[LORE_N] = {0};
        uint8_t bytes[2 * LORE_N];
        randombytes(bytes, sizeof bytes);
        for (int i = 0; i < LORE_N; ++i) {
            a.coeffs[i] = (int16_t)((int)(bytes[i] % LORE_T) - 1);
            b.coeffs[i] = (int16_t)((int)(bytes[LORE_N + i] % LORE_T) - 1);
        }
        for (int i = 0; i < LORE_N; ++i) {
            for (int j = 0; j < LORE_N; ++j) {
                int index = i + j;
                int32_t term = (int32_t)a.coeffs[i] * b.coeffs[j];
                if (index >= LORE_N) reference[index - LORE_N] -= term;
                else reference[index] += term;
            }
        }
        poly_mul_modt(&c, &a, &b);
        for (int i = 0; i < LORE_N; ++i) {
            if (c.coeffs[i] != sym_mod((int16_t)reference[i], LORE_T)) {
                fprintf(stderr, "mod-t product mismatch at trial %d coefficient %d\n", trial, i);
                return 1;
            }
        }
    }
    return 0;
}

static int test_sampler_counts(void) {
    poly_crt_vec sampled;
    poly_sparse sparse[LORE_K];
    uint8_t seed[LORE_SYMBYTES];
    int counts[5] = {0};
    const int expected[5] = {200, 400, 848, 400, 200};
    randombytes(seed, sizeof seed);
    sample_fixed_weight(&sampled, sparse, seed, 0);
    for (int p = 0; p < LORE_K; ++p) {
        for (int i = 0; i < LORE_N; ++i) {
            int value = sampled.vec[p].q_poly.coeffs[i];
            if (value < -2 || value > 2 ||
                sampled.vec[p].t_poly.coeffs[i] != value) {
                fprintf(stderr, "Sampler invalid coefficient\n");
                return 1;
            }
            counts[value + 2]++;
        }
    }
    for (int i = 0; i < 5; ++i) {
        if (counts[i] != expected[i]) {
            fprintf(stderr, "Sampler count mismatch for value %d: %d != %d\n",
                    i - 2, counts[i], expected[i]);
            return 1;
        }
    }
    return 0;
}

static int test_rounding_law(void) {
    const int expected_delta[4] = {0, 1, 0, 1};
    for (int x = 0; x < 4; ++x) {
        int16_t idx = find_closest_r_idx((int16_t)x);
        int16_t rep = lore_rounding_representative(idx);
        int16_t delta = sym_mod(rep - x, LORE_T);
        if (delta != expected_delta[x]) {
            fprintf(stderr, "Rounding law mismatch for residue %d\n", x);
            return 1;
        }
    }
    return 0;
}

static int test_bch_capacity(void) {
    struct bch_control *bch = init_bch(LORE_BCH_M, LORE_BCH_T, 0);
    if (bch == NULL) {
        fprintf(stderr, "BCH initialization failed\n");
        return 1;
    }
    if (LORE_CODEWORD_BYTES * 8 > LORE_N || bch->ecc_bytes > LORE_ECC_BYTES) {
        fprintf(stderr, "BCH codeword exceeds polynomial capacity\n");
        free_bch(bch);
        return 1;
    }
    uint8_t data[LORE_MSG_BYTES];
    uint8_t corrupted[LORE_MSG_BYTES];
    uint8_t ecc[LORE_ECC_BYTES] = {0};
    uint8_t corrupted_ecc[LORE_ECC_BYTES];
    unsigned int errloc[LORE_BCH_T];
    randombytes(data, sizeof data);
    encode_bch(bch, data, sizeof data, ecc);
    const unsigned int data_bits = LORE_MSG_BYTES * 8u;
    const unsigned int active_bits = data_bits + bch->ecc_bits;
    printf("BCH: m=%u, t=%u, parity=%u active=%u stored=%u bits\n",
           LORE_BCH_M, LORE_BCH_T, bch->ecc_bits, active_bits,
           LORE_CODEWORD_BYTES * 8u);
    if (active_bits > LORE_N) {
        fprintf(stderr, "BCH active codeword exceeds polynomial capacity\n");
        free_bch(bch);
        return 1;
    }
    int fail = 0;
    for (unsigned int pattern = 0; pattern < 5; ++pattern) {
        memcpy(corrupted, data, sizeof data);
        memcpy(corrupted_ecc, ecc, sizeof ecc);
        unsigned int start = pattern * active_bits / 5u;
        unsigned int stride = pattern == 4 ? 17u : 1u;
        for (unsigned int i = 0; i < LORE_BCH_T; ++i) {
            unsigned int bit = (start + i * stride) % active_bits;
            if (bit < data_bits) {
                corrupted[bit / 8] ^= (uint8_t)(1u << (bit % 8));
            } else {
                bit -= data_bits;
                corrupted_ecc[bit / 8] ^= (uint8_t)(1u << (7u - bit % 8u));
            }
        }
        int errors = decode_bch(bch, corrupted, sizeof corrupted,
                                corrupted_ecc, NULL, NULL, errloc);
        if (errors > 0) correct_bch(bch, corrupted, sizeof corrupted, errloc, errors);
        if (errors < 0 || memcmp(corrupted, data, sizeof data) != 0) {
            fprintf(stderr, "BCH failed to correct %d errors in pattern %u\n",
                    LORE_BCH_T, pattern);
            fail = 1;
            break;
        }
    }
    /* Encoding occupies only ecc_bits parity positions; BCH must ignore the
       unused positions even when the polynomial channel flips them. */
    if (!fail) {
        memcpy(corrupted, data, sizeof data);
        memcpy(corrupted_ecc, ecc, sizeof ecc);
        for (unsigned int bit = bch->ecc_bits; bit < LORE_ECC_BYTES * 8u; ++bit)
            corrupted_ecc[bit / 8u] ^= (uint8_t)(1u << (7u - bit % 8u));
        int errors = decode_bch(bch, corrupted, sizeof corrupted,
                                corrupted_ecc, NULL, NULL, errloc);
        if (errors != 0 || memcmp(corrupted, data, sizeof data) != 0) {
            fprintf(stderr, "BCH did not ignore unused parity positions\n");
            fail = 1;
        }
    }
    free_bch(bch);
    return fail;
}

static int test_kem(void) {
    for (int trial = 0; trial < 1000; ++trial) {
        uint8_t pk[CRYPTO_PUBLICKEYBYTES];
        uint8_t sk[CRYPTO_SECRETKEYBYTES];
        uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
        uint8_t ss_enc[CRYPTO_BYTES], ss_dec[CRYPTO_BYTES], ss_dec_again[CRYPTO_BYTES];
        uint8_t ss_bad[CRYPTO_BYTES], ss_bad_again[CRYPTO_BYTES];
        memset(ss_dec, 0xa5, sizeof ss_dec);
        memset(ss_dec_again, 0x5a, sizeof ss_dec_again);
        if (crypto_kem_keypair(pk, sk) || crypto_kem_enc(ct, ss_enc, pk) ||
            crypto_kem_dec(ss_dec, ct, sk) ||
            crypto_kem_dec(ss_dec_again, ct, sk) ||
            memcmp(ss_enc, ss_dec, CRYPTO_BYTES) ||
            memcmp(ss_dec, ss_dec_again, CRYPTO_BYTES)) {
            fprintf(stderr, "KEM roundtrip failed at trial %d\n", trial);
            return 1;
        }
        ct[0] ^= 1;
        memset(ss_bad, 0xa5, sizeof ss_bad);
        memset(ss_bad_again, 0x5a, sizeof ss_bad_again);
        if (crypto_kem_dec(ss_bad, ct, sk) ||
            crypto_kem_dec(ss_bad_again, ct, sk) ||
            memcmp(ss_bad, ss_bad_again, CRYPTO_BYTES) ||
            memcmp(ss_enc, ss_bad, CRYPTO_BYTES) == 0) {
            fprintf(stderr, "KEM invalid-ciphertext test failed at trial %d\n", trial);
            return 1;
        }
    }
    return 0;
}

static int test_malformed_ciphertexts(void) {
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss[CRYPTO_BYTES];
    if (crypto_kem_keypair(pk, sk)) return 1;
    for (int trial = 0; trial < 132; ++trial) {
        if (trial < 4) {
            const uint8_t patterns[] = {0x00, 0xff, 0x55, 0xaa};
            memset(ct, patterns[trial], sizeof ct);
        } else {
            randombytes(ct, sizeof ct);
        }
        if (crypto_kem_dec(ss, ct, sk)) {
            fprintf(stderr, "Malformed-ciphertext decapsulation failed at trial %d\n", trial);
            return 1;
        }
    }
    return 0;
}

static int test_malformed_keys(void) {
    uint8_t pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
    uint8_t damaged[CRYPTO_SECRETKEYBYTES], ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss[CRYPTO_BYTES], msg[LORE_MSG_BYTES];
    if (crypto_kem_keypair(pk, sk) || crypto_kem_enc(ct, ss, pk)) return 1;
    uint16_t count = lore_load16_le(sk);
    if (count < 2) return 1;
    for (int trial = 0; trial < 7; ++trial) {
        memcpy(damaged, sk, sizeof damaged);
        uint16_t word;
        switch (trial) {
        case 0: word = 0xffff; lore_store16_le(damaged, word); break;
        case 1: word = LORE_N + 1; lore_store16_le(damaged, word); break;
        case 2: memset(damaged, 0, 2 * LORE_K); break;
        case 3: word = LORE_N; lore_store16_le(damaged + 2 * LORE_K, word); break;
        case 4: memcpy(damaged + 2 * LORE_K + 2, damaged + 2 * LORE_K, 2); break;
        case 5: damaged[2 * LORE_K + 2 * count] = 0; break;
        default: damaged[2 * LORE_K + 2 * count] = 127; break;
        }
        memset(ss, 0xa5, sizeof ss);
        memset(msg, 0xa5, sizeof msg);
        unsigned long long length = sizeof msg;
        if (crypto_kem_dec(ss, ct, damaged) != -1 ||
            crypto_pke_decrypt(msg, &length, ct, damaged) != -1 || length != 0)
            return 1;
        for (size_t j = 0; j < sizeof ss; ++j) if (ss[j]) return 1;
        for (size_t j = 0; j < sizeof msg; ++j) if (msg[j]) return 1;
    }
    /* Arbitrary public keys remain safe to parse; no claim that they are
     * generated keys or will encapsulate to a usable shared secret. */
    for (int trial = 0; trial < 132; ++trial) {
        if (trial < 4) {
            const uint8_t patterns[] = {0, 255, 85, 170};
            memset(pk, patterns[trial], sizeof pk);
        } else randombytes(pk, sizeof pk);
        if (crypto_kem_enc(ct, ss, pk)) return 1;
    }
    return 0;
}

static int test_deterministic_api(void) {
    uint8_t keypair_coins[CRYPTO_KEYPAIRCOINBYTES];
    uint8_t encaps_coins[CRYPTO_ENCCOINBYTES];
    uint8_t pk1[CRYPTO_PUBLICKEYBYTES], pk2[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk1[CRYPTO_SECRETKEYBYTES], sk2[CRYPTO_SECRETKEYBYTES];
    uint8_t ct1[CRYPTO_CIPHERTEXTBYTES], ct2[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss1[CRYPTO_BYTES], ss2[CRYPTO_BYTES], ss_dec[CRYPTO_BYTES];
    randombytes(keypair_coins, sizeof keypair_coins);
    randombytes(encaps_coins, sizeof encaps_coins);
    if (crypto_kem_keypair_derand(pk1, sk1, keypair_coins) ||
        crypto_kem_keypair_derand(pk2, sk2, keypair_coins) ||
        memcmp(pk1, pk2, sizeof pk1) || memcmp(sk1, sk2, sizeof sk1)) {
        fprintf(stderr, "Deterministic keypair mismatch\n");
        return 1;
    }
    if (crypto_kem_enc_derand(ct1, ss1, pk1, encaps_coins) ||
        crypto_kem_enc_derand(ct2, ss2, pk1, encaps_coins) ||
        memcmp(ct1, ct2, sizeof ct1) || memcmp(ss1, ss2, sizeof ss1) ||
        crypto_kem_dec(ss_dec, ct1, sk1) || memcmp(ss1, ss_dec, sizeof ss1)) {
        fprintf(stderr, "Deterministic encapsulation mismatch\n");
        return 1;
    }
    return 0;
}

int main(void) {
    for (unsigned int value = 0; value < 65536; ++value) {
        unsigned char bytes[2];
        int16_t sampled = -1;
        lore_store16_le(bytes, (uint16_t)value);
        if (bytes[0] != value % 256 || bytes[1] != value / 256 ||
            lore_load16_le(bytes) != value) return 1;
        unsigned int count = rej_uniform_q(&sampled, 1, bytes, sizeof bytes);
        if (value == 65535) {
            if (count != 0 || sampled != -1) return 1;
        } else if (count != 1 || sampled != (int16_t)(value % LORE_Q)) return 1;
    }
    if (test_add_modt()) return 1;
    if (test_monomial_products()) {
        fputs("Monomial wraparound check failed\n", stderr);
        return 1;
    }
    if (test_ntt_product()) return 1;
    if (test_modt_product()) return 1;
    if (test_sampler_counts()) return 1;
    if (test_rounding_law()) return 1;
    if (test_bch_capacity()) return 1;
    if (test_kem()) return 1;
    if (test_malformed_ciphertexts()) return 1;
    if (test_malformed_keys()) {
        fputs("Malformed-key checks failed\n", stderr);
        return 1;
    }
    if (test_deterministic_api()) return 1;
    puts("LORE-512 n=1024 k=2: NTT, sampler, BCH, and KEM tests passed");
    return 0;
}
