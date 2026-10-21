#include "../../../common/verification_bounds_test.h"
#include "../../../common/response_arithmetic_test.h"
#include "../../../common/symmetric_hash_test.h"
#include "../../../common/rans_decode_test.h"
#include "public_key_encoding_test.h"
/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information
Engineering, CAS School of Cyber Security, University of Chinese Academy of Sciences
*/
#include <stdio.h>
#include <string.h>

#include "../SIG_AlgorithmInstance.h"
#include "../params.h"
#include "../drng.h"
#include "../ntt.h"
#include "../packing.h"
#include "../polyvec.h"
#include "../sample.h"
#include "../symmetric.h"
#include "test_common.h"
#include "joint_l2_bound_test.h"
#include "secret_encoding_test.h"
#include "../../../common/spectral_test.h"

#define MAX_MSG_BYTES 512

#if BIT_SIGNBYTES != 2976
#error "BiT-256 signature size does not match the parameter set"
#endif

#if BIT_USE_SHAKE
#define XOF_TEST_BYTES (3 * SHAKE128_RATE + 37)
static int test_xof128_stream_wrapper(void)
{
    uint8_t seed[BIT_SEEDBYTES + 2];
    uint8_t extseed[sizeof(seed) + 1];
    uint8_t expected[XOF_TEST_BYTES];
    uint8_t actual[XOF_TEST_BYTES];
    const size_t chunks[] = {1, SHAKE128_RATE - 3, 5, SHAKE128_RATE, 19, 2 * SHAKE128_RATE + 15};
    bit_xof128_state st;
    size_t off = 0;

    test_fill_seed(seed, sizeof(seed), 7, 0);
    memcpy(extseed, seed, sizeof(seed));
    extseed[sizeof(seed)] = 23;
    shake128(expected, sizeof(expected), extseed, sizeof(extseed));

    bit_xof128_init(&st, extseed, sizeof(extseed));
    for (size_t i = 0; i < sizeof(chunks) / sizeof(chunks[0]); i++) {
        size_t take = chunks[i];
        if (take > sizeof(actual) - off)
            take = sizeof(actual) - off;
        if (take == 0)
            break;
        bit_xof128_squeeze(&st, actual + off, take);
        off += take;
    }
    if (off < sizeof(actual)) {
        bit_xof128_squeeze(&st, actual + off, sizeof(actual) - off);
    }
    bit_xof128_zeroize(&st);
    return memcmp(actual, expected, sizeof(expected));
}
#endif

static void make_hint_scalar(polyveck *h, const polyveck *w1, const polyveck *w, const polyveck *z2,
                             const poly *c, uint8_t b)
{
    const int32_t add_c_mask = -(int32_t)(1U - b);

    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int32_t w_hat =
                (int32_t)w->vec[i].coeffs[j] - z2->vec[i].coeffs[j];
            int16_t hb;
            int32_t h_val;

            if (i == 0)
                w_hat += (int32_t)c->coeffs[j] & add_c_mask;
            decompose_hint(&hb, (int16_t)w_hat);
            h_val = (int32_t)w1->vec[i].coeffs[j] - hb;
            h_val += (h_val >> 31) & BIT_HINT_MODULUS;
            h_val -= BIT_HINT_MODULUS & ((BIT_HINT_MODULUS / 2 - h_val) >> 31);
            h->vec[i].coeffs[j] = (int16_t)h_val;
        }
    }
}

static void compute_w_prime_scalar(polyveck *out, const polyveck *w_hat, const polyveck *h)
{
    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int16_t hb;
            int32_t val;

            decompose_hint(&hb, w_hat->vec[i].coeffs[j]);
            val = (int32_t)hb + h->vec[i].coeffs[j];
            val += (val >> 31) & BIT_HINT_MODULUS;
            val -= BIT_HINT_MODULUS;
            val += (val >> 31) & BIT_HINT_MODULUS;
            out->vec[i].coeffs[j] = (int16_t)val;
        }
    }
}

static int test_hint_matches_scalar(void)
{
    polyveck w1 = {0}, w = {0}, z2 = {0};
    polyveck actual = {0}, expected = {0};
    polyveck recovered_actual = {0}, recovered_expected = {0};
    poly c = {0};

    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            uint32_t x = (uint32_t)(i * BIT_N + j);
            w1.vec[i].coeffs[j] = (int16_t)((7 * x + 3) % BIT_HINT_MODULUS);
            w.vec[i].coeffs[j] = (int16_t)((37 * x + 11) % BIT_Q - BIT_Q_HALF);
            z2.vec[i].coeffs[j] = (int16_t)((13 * x + 5) % 269 - 134);
        }
    }
    for (int j = 0; j < BIT_N; j++)
        c.coeffs[j] = (int16_t)((j % 11 == 0) ? 1 : 0);

    for (uint8_t b = 0; b <= 1; b++) {
        polyveck_make_hint(&actual, &w1, &w, &z2, &c, b);
        make_hint_scalar(&expected, &w1, &w, &z2, &c, b);
        if (memcmp(&actual, &expected, sizeof(actual)) != 0)
            return 1;
    }

    polyveck_compute_w_prime(&recovered_actual, &w, &actual);
    compute_w_prime_scalar(&recovered_expected, &w, &actual);
    return memcmp(&recovered_actual, &recovered_expected, sizeof(recovered_actual));
}

static int test_secret_key_encoding(void)
{
    static const int16_t small_values[3] = {0, 1, -1};
    uint8_t seed_a[BIT_SEEDBYTES], seed_a_out[BIT_SEEDBYTES];
    uint8_t secret_seed[BIT_SEEDBYTES], secret_seed_out[BIT_SEEDBYTES];
    uint8_t tr[BIT_TRBYTES], tr_out[BIT_TRBYTES];
    uint8_t sk[BIT_SECRETKEYBYTES];
    polyveck public_vector = {0}, public_out = {0}, e = {0}, e_out = {0};
    polyvecl s0 = {0}, s0_out = {0};
    const size_t small_off = BIT_SEEDBYTES + BIT_POLYVECK_PUBLIC_BYTES + BIT_SEEDBYTES + BIT_TRBYTES;

    test_fill_seed(seed_a, sizeof(seed_a), 301, 0);
    test_fill_seed(secret_seed, sizeof(secret_seed), 302, 0);
    test_fill_seed(tr, sizeof(tr), 303, 0);
    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int index = i * BIT_N + j;
            public_vector.vec[i].coeffs[j] = (int16_t)((index * 607 + 19) % BIT_Q);
            e.vec[i].coeffs[j] = small_values[(index + 1) % 3];
        }
    }
    for (int i = 0; i < BIT_L; i++)
        for (int j = 0; j < BIT_N; j++)
            s0.vec[i].coeffs[j] = small_values[(i * BIT_N + j) % 3];

    pack_sk(sk, seed_a, &public_vector, secret_seed, tr, &s0, &e);
    {
        const poly *small_polys[BIT_L + BIT_K];
        size_t off = small_off;
        for (int i = 0; i < BIT_L; i++)
            small_polys[i] = &s0.vec[i];
        for (int i = 0; i < BIT_K; i++)
            small_polys[BIT_L + i] = &e.vec[i];
        for (int i = 0; i < BIT_L + BIT_K; i++) {
            for (int j = 0; j < BIT_N; j += 4) {
                uint8_t expected = ((uint16_t)small_polys[i]->coeffs[j] & 3U) |
                                   (((uint16_t)small_polys[i]->coeffs[j + 1] & 3U) << 2) |
                                   (((uint16_t)small_polys[i]->coeffs[j + 2] & 3U) << 4) |
                                   (((uint16_t)small_polys[i]->coeffs[j + 3] & 3U) << 6);
                if (sk[off++] != expected)
                    return 1;
            }
        }
    }
    if (unpack_sk(seed_a_out, &public_out, secret_seed_out, tr_out, &s0_out, &e_out, sk) != 0)
        return 3;
    if (memcmp(seed_a, seed_a_out, sizeof(seed_a)) ||
        memcmp(secret_seed, secret_seed_out, sizeof(secret_seed)) ||
        memcmp(tr, tr_out, sizeof(tr)) || memcmp(&public_vector, &public_out, sizeof(public_vector)) ||
        memcmp(&s0, &s0_out, sizeof(s0)) || memcmp(&e, &e_out, sizeof(e)))
        return 4;
    sk[small_off] = (uint8_t)((sk[small_off] & 0xFCU) | 2U);
    if (unpack_sk(seed_a_out, &public_out, secret_seed_out, tr_out, &s0_out, &e_out, sk) == 0)
        return 5;
    return 0;
}

static int test_z0_rans_encoding(void)
{
    static const int16_t edge_values[] = {-BIT_RESPONSE_COEFF_BOUND,
                                          BIT_RESPONSE_COEFF_BOUND,
                                          -127,
                                          -65,
                                          -64,
                                          -63,
                                          -33,
                                          -32,
                                          -31,
                                          -1,
                                          0,
                                          1,
                                          31,
                                          32,
                                          33,
                                          63,
                                          64,
                                          65,
                                          127};
    uint8_t sig[BIT_SIGNBYTES];
    uint8_t challenge[BIT_CHALLENGEBYTES];
    uint8_t challenge_out[BIT_CHALLENGEBYTES];
    polyvecm1 in = {0};
    polyvecm1 out = {0};
    polyveck h = {0};
    polyveck h_out = {0};
    test_fill_seed(challenge, sizeof(challenge), 512, 0);
    for (size_t i = 0; i < sizeof(edge_values) / sizeof(edge_values[0]); i++)
        in.vec[0].coeffs[i] = edge_values[i];

    if (pack_sig(sig, &in, &h, challenge) != 0)
        return 1;
    if (unpack_sig(&out, &h_out, challenge_out, sig) != 0)
        return 1;
    if (memcmp(challenge, challenge_out, sizeof(challenge)) || memcmp(&in, &out, sizeof(in)) ||
        memcmp(&h, &h_out, sizeof(h_out)))
        return 2;

    in.vec[0].coeffs[0] = BIT_RESPONSE_COEFF_BOUND + 1;
    if (pack_sig(sig, &in, &h, challenge) == 0)
        return 4;

    return 0;
}

static int test_compressed_signature_roundtrip(void)
{
    uint8_t sig[BIT_SIGNBYTES];
    uint8_t challenge[BIT_CHALLENGEBYTES];
    uint8_t challenge_out[BIT_CHALLENGEBYTES];
    polyvecm1 in = {0}, out = {0};
    polyveck h = {0}, h_out = {0};
    const size_t lengths_off = BIT_CHALLENGEBYTES + BIT_Z_LOW_BYTES;

    test_fill_seed(challenge, sizeof(challenge), 1344, 0);
    for (int i = 0; i < BIT_N; i++)
        in.vec[0].coeffs[i] = (int16_t)((i % 63) - 31);
    in.vec[0].coeffs[0] = -BIT_RESPONSE_COEFF_BOUND;
    in.vec[0].coeffs[1] = BIT_RESPONSE_COEFF_BOUND;
    for (int i = 0; i < BIT_L; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int x = i * BIT_N + j;
            int v = (x % 255) - 127;
            if (x % 19 == 0)
                v += (x & 1) ? 256 : -256;
            in.vec[i + 1].coeffs[j] = (int16_t)v;
        }
    }
    in.vec[1].coeffs[0] = 128;
    in.vec[1].coeffs[1] = -128;
    for (int i = 0; i < BIT_K; i++)
        for (int j = 0; j < BIT_N; j++)
            if ((i * BIT_N + j) % 23 == 0)
                h.vec[i].coeffs[j] = (int16_t)(((i + j) & 1) ? 1 : -1);

    if (pack_sig(sig, &in, &h, challenge) != 0)
        return 1;
    if (unpack_sig(&out, &h_out, challenge_out, sig) != 0)
        return 2;
    if (memcmp(challenge, challenge_out, sizeof(challenge)) || memcmp(&in, &out, sizeof(in)) ||
        memcmp(&h, &h_out, sizeof(h)))
        return 3;

    sig[lengths_off] = 0;
    sig[lengths_off + 1] = 0;
    if (unpack_sig(&out, &h_out, challenge_out, sig) == 0)
        return 4;
    if (pack_sig(sig, &in, &h, challenge) != 0)
        return 5;
    sig[BIT_SIGNBYTES - 1] = 1;
    if (unpack_sig(&out, &h_out, challenge_out, sig) == 0)
        return 6;

    in.vec[1].coeffs[0] = BIT_RESPONSE_COEFF_BOUND;
    in.vec[1].coeffs[1] = -BIT_RESPONSE_COEFF_BOUND;
    if (pack_sig(sig, &in, &h, challenge) != 0)
        return 7;
    if (unpack_sig(&out, &h_out, challenge_out, sig) != 0 || memcmp(&in, &out, sizeof(in)))
        return 8;
    in.vec[1].coeffs[0] = BIT_RESPONSE_COEFF_BOUND + 1;
    if (pack_sig(sig, &in, &h, challenge) == 0)
        return 9;
    return 0;
}

static int test_keygen_sign_verify(unsigned int seed_domain, unsigned int msg_domain,
                                   size_t msg_len)
{
    uint8_t pk[BIT_PUBLICKEYBYTES];
    uint8_t sk[BIT_SECRETKEYBYTES];
    uint8_t sig[BIT_SIGNBYTES];
    uint8_t msg[MAX_MSG_BYTES];
    uint8_t seed[55];
    unsigned long long pk_len = 0, sk_len = 0, sig_len = 0;

    test_fill_seed(seed, sizeof(seed), seed_domain, 0);
    if (init_random_number(&drng_algorithm, seed, (unsigned long long)sizeof(seed)) != 0)
        return -1;

    if (sig_keygen(pk, &pk_len, sk, &sk_len) != 0)
        return -2;
    if (pk_len != BIT_PUBLICKEYBYTES || sk_len != BIT_SECRETKEYBYTES)
        return -3;

    test_fill_message(msg, msg_len, msg_domain, 0);
    if (sig_sign(sk, sk_len, msg, msg_len, sig, &sig_len) != 0)
        return -4;
    if (sig_len != BIT_SIGNBYTES)
        return -5;

    if (sig_verify(pk, pk_len, sig, sig_len, msg, msg_len) != 0)
        return -6;

    /* tampered message */
    if (msg_len > 0) {
        msg[msg_len / 2] ^= 0x80U;
        if (sig_verify(pk, pk_len, sig, sig_len, msg, msg_len) == 0)
            return -7;
        msg[msg_len / 2] ^= 0x80U;
    }

    /* tampered signature — use positions safely away from potential padding bits */
    sig[sig_len / 3] ^= 0x40U;
    if (sig_verify(pk, pk_len, sig, sig_len, msg, msg_len) == 0)
        return -8;
    sig[sig_len / 3] ^= 0x40U;

    sig[sig_len * 2 / 3] ^= 0x02U;
    if (sig_verify(pk, pk_len, sig, sig_len, msg, msg_len) == 0)
        return -9;
    sig[sig_len * 2 / 3] ^= 0x02U;

    /* tampered public key */
    pk[pk_len / 2] ^= 0x55U;
    if (sig_verify(pk, pk_len, sig, sig_len, msg, msg_len) == 0)
        return -10;
    pk[pk_len / 2] ^= 0x55U;

    return 0;
}

static int test_wrong_key_pair(void)
{
    uint8_t pk1[BIT_PUBLICKEYBYTES], pk2[BIT_PUBLICKEYBYTES];
    uint8_t sk1[BIT_SECRETKEYBYTES], sk2[BIT_SECRETKEYBYTES];
    uint8_t sig[BIT_SIGNBYTES];
    uint8_t msg[64];
    uint8_t seed[55];
    unsigned long long pk_len = 0, sk_len = 0, sig_len = 0;

    /* keypair 1 */
    test_fill_seed(seed, sizeof(seed), 100, 0);
    if (init_random_number(&drng_algorithm, seed, (unsigned long long)sizeof(seed)) != 0)
        return -1;
    if (sig_keygen(pk1, &pk_len, sk1, &sk_len) != 0)
        return -2;

    /* keypair 2 */
    test_fill_seed(seed, sizeof(seed), 200, 0);
    if (init_random_number(&drng_algorithm, seed, (unsigned long long)sizeof(seed)) != 0)
        return -3;
    if (sig_keygen(pk2, &pk_len, sk2, &sk_len) != 0)
        return -4;

    test_fill_message(msg, sizeof(msg), 1, 0);
    if (sig_sign(sk1, sk_len, msg, sizeof(msg), sig, &sig_len) != 0)
        return -5;

    /* verify with wrong public key */
    if (sig_verify(pk2, pk_len, sig, sig_len, msg, sizeof(msg)) == 0)
        return -6;

    return 0;
}

static int test_empty_message(void)
{
    uint8_t pk[BIT_PUBLICKEYBYTES];
    uint8_t sk[BIT_SECRETKEYBYTES];
    uint8_t sig[BIT_SIGNBYTES];
    uint8_t seed[55];
    unsigned long long pk_len = 0, sk_len = 0, sig_len = 0;

    test_fill_seed(seed, sizeof(seed), 42, 0);
    if (init_random_number(&drng_algorithm, seed, (unsigned long long)sizeof(seed)) != 0)
        return -1;
    if (sig_keygen(pk, &pk_len, sk, &sk_len) != 0)
        return -2;
    if (sig_sign(sk, sk_len, NULL, 0, sig, &sig_len) != 0)
        return -3;
    if (sig_len != BIT_SIGNBYTES)
        return -4;
    if (sig_verify(pk, pk_len, sig, sig_len, NULL, 0) != 0)
        return -5;
    return 0;
}

static int test_padding_rejection(void)
{
    uint8_t pk[BIT_PUBLICKEYBYTES];
    uint8_t sk[BIT_SECRETKEYBYTES];
    uint8_t sig[BIT_SIGNBYTES];
    uint8_t msg[64];
    uint8_t seed[55];
    unsigned long long pk_len = 0, sk_len = 0, sig_len = 0;

    test_fill_seed(seed, sizeof(seed), 99, 0);
    if (init_random_number(&drng_algorithm, seed, (unsigned long long)sizeof(seed)) != 0)
        return -1;
    if (sig_keygen(pk, &pk_len, sk, &sk_len) != 0)
        return -2;

    test_fill_message(msg, sizeof(msg), 99, 0);
    if (sig_sign(sk, sk_len, msg, sizeof(msg), sig, &sig_len) != 0)
        return -3;

    /* flip padding bits in last byte of overflow buffer — must be rejected */
    sig[sig_len - 1] ^= 0xE0U;
    if (sig_verify(pk, pk_len, sig, sig_len, msg, sizeof(msg)) == 0)
        return -4;
    sig[sig_len - 1] ^= 0xE0U;

    return 0;
}

static int test_multi_sign_verify(unsigned int seed_domain, unsigned int rounds)
{
    uint8_t pk[BIT_PUBLICKEYBYTES];
    uint8_t sk[BIT_SECRETKEYBYTES];
    uint8_t sig[BIT_SIGNBYTES];
    uint8_t msg[MAX_MSG_BYTES];
    uint8_t seed[55];
    unsigned long long pk_len = 0, sk_len = 0, sig_len = 0;

    test_fill_seed(seed, sizeof(seed), seed_domain, 0);
    if (init_random_number(&drng_algorithm, seed, (unsigned long long)sizeof(seed)) != 0)
        return -1;
    if (sig_keygen(pk, &pk_len, sk, &sk_len) != 0)
        return -2;

    for (unsigned int r = 0; r < rounds; r++) {
        test_fill_message(msg, sizeof(msg), seed_domain, r);
        if (sig_sign(sk, sk_len, msg, sizeof(msg), sig, &sig_len) != 0)
            return -3;
        if (sig_len != BIT_SIGNBYTES)
            return -4;
        if (sig_verify(pk, pk_len, sig, sig_len, msg, sizeof(msg)) != 0)
            return -5;
    }
    return 0;
}

#include "api_input_checks.h"

int main(void)
{
    int hash_check = test_symmetric_hashes();
    if (hash_check != 0) {
        printf("FAIL: symmetric backend hashes (%d)\n", hash_check);
        return 1;
    }
    puts("PASS: symmetric backend hashes");
    if (test_public_key_canonicalization())
        return 1;
    int api_check = test_api_input_validation();
    if (api_check != 0) {
        printf("FAIL: API input validation case %d\n", api_check);
        return 1;
    }
    int failures = test_joint_l2_bound() + test_secret_encoding() + test_verification_bounds();
    int arithmetic_check = test_response_arithmetic();
    if (arithmetic_check != 0)
        fprintf(stderr, "FAIL: HighBits/packing/centered NTT arithmetic\n");
    failures += arithmetic_check;
    int spectrum_check = test_keygen_spectrum();
    if (spectrum_check != 0) {
        printf("FAIL: KeyGen spectral bound (%d)\n", spectrum_check);
        failures++;
    } else {
        puts("PASS: KeyGen spectral bound");
    }
    if (test_rans_lookup() != 0) {
        printf("FAIL: rANS lookup intervals\n");
        failures++;
    } else {
        printf("PASS: rANS lookup intervals\n");
    }

#if BIT_USE_SHAKE
    if (test_xof128_stream_wrapper() != 0) {
        printf("FAIL: xof128 stream wrapper\n");
        failures++;
    } else {
        printf("PASS: xof128 stream wrapper\n");
    }
#endif

    if (test_public_key_wire_format() != 0) {
        printf("FAIL: canonical public-key encoding\n");
        failures++;
    } else {
        printf("PASS: canonical public-key encoding\n");
    }

    if (test_secret_key_encoding() != 0) {
        printf("FAIL: secret-key encoding\n");
        failures++;
    } else {
        printf("PASS: secret-key encoding\n");
    }

    if (test_z0_rans_encoding() != 0) {
        printf("FAIL: z0 low-byte/rANS encoding\n");
        failures++;
    } else {
        printf("PASS: z0 low-byte/rANS encoding\n");
    }

    if (test_compressed_signature_roundtrip() != 0) {
        printf("FAIL: compressed signature roundtrip\n");
        failures++;
    } else {
        printf("PASS: compressed signature roundtrip\n");
    }

    if (test_hint_matches_scalar() != 0) {
        printf("FAIL: hint scalar cross-check\n");
        failures++;
    } else {
        printf("PASS: hint scalar cross-check\n");
    }

    /* multiple seed domains */
    {
        const unsigned int domains[] = {0, 1, 2, 3, 5, 7, 13, 19, 31, 63, 127, 255};
        for (size_t i = 0; i < sizeof(domains) / sizeof(domains[0]); i++) {
            int rc = test_keygen_sign_verify(domains[i], domains[i], 64);
            if (rc != 0) {
                printf("FAIL: keygen/sign/verify domain=%u (rc=%d)\n", domains[i], rc);
                failures++;
            } else {
                printf("PASS: keygen/sign/verify domain=%u\n", domains[i]);
            }
        }
    }

    /* multiple message lengths */
    {
        const size_t msg_lens[] = {0,  1,  2,   3,   5,   8,   13,  21, 32,
                                   47, 64, 100, 128, 200, 256, 400, 512};
        for (size_t i = 0; i < sizeof(msg_lens) / sizeof(msg_lens[0]); i++) {
            int rc = test_keygen_sign_verify(77, 77, msg_lens[i]);
            if (rc != 0) {
                printf("FAIL: message length %zu (rc=%d)\n", msg_lens[i], rc);
                failures++;
            } else {
                printf("PASS: message length %zu\n", msg_lens[i]);
            }
        }
    }

    /* wrong key pair */
    if (test_wrong_key_pair() != 0) {
        printf("FAIL: wrong key pair\n");
        failures++;
    } else {
        printf("PASS: wrong key pair rejection\n");
    }

    /* padding rejection */
    if (test_padding_rejection() != 0) {
        printf("FAIL: padding bit rejection\n");
        failures++;
    } else {
        printf("PASS: padding bit rejection\n");
    }

    /* empty message */
    if (test_empty_message() != 0) {
        printf("FAIL: empty message\n");
        failures++;
    } else {
        printf("PASS: empty message sign/verify\n");
    }

    /* multi sign/verify */
    {
        const unsigned int rounds[] = {1, 5, 10};
        for (size_t i = 0; i < sizeof(rounds) / sizeof(rounds[0]); i++) {
            int rc = test_multi_sign_verify(50 + (unsigned int)i, rounds[i]);
            if (rc != 0) {
                printf("FAIL: multi sign/verify rounds=%u (rc=%d)\n", rounds[i], rc);
                failures++;
            } else {
                printf("PASS: multi sign/verify rounds=%u\n", rounds[i]);
            }
        }
    }

    if (failures == 0) {
        printf("\nAll correctness tests passed.\n");
    } else {
        printf("\n%d test(s) failed.\n", failures);
    }

    printf("\n--- Sizes ---\n");
    printf("SEEDBYTES       = %d\n", BIT_SEEDBYTES);
    printf("CHALLENGEBYTES  = %d\n", BIT_CHALLENGEBYTES);
    printf("TRBYTES         = %d\n", BIT_TRBYTES);
    printf("MESSAGEBYTES    = %d\n", BIT_MESSAGEBYTES);
    printf("pk             = %d B  (seed + K*public-poly) = %d + %d*%d\n", BIT_PUBLICKEYBYTES,
           BIT_SEEDBYTES, BIT_K, BIT_POLY_PUBLIC_BYTES);
    printf("sig            = %d B  (challenge + low(z0,z1) + lengths + rANS)\n", BIT_SIGNBYTES);
    printf("                  = %d + %d + %d + %d\n", BIT_CHALLENGEBYTES, BIT_Z_LOW_BYTES,
           BIT_SIG_RANS_LENGTH_BYTES, BIT_SIG_RANS_PAYLOAD_BYTES);
    printf("sk             = %d B\n", BIT_SECRETKEYBYTES);

    return failures != 0 ? 1 : 0;
}
