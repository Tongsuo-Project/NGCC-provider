#include "../../../common/verification_bounds_test.h"
#include "../../../common/response_arithmetic_test.h"
#include "../../../common/symmetric_hash_test.h"
#include "../../../common/rans_decode_test.h"
/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information
Engineering, CAS School of Cyber Security, University of Chinese Academy of Sciences
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../SIG_AlgorithmInstance.h"
#include "../params.h"
#include "../drng.h"
#include "../packing.h"
#include "../endian.h"
#include "../sample.h"
#include "../symmetric.h"
#include "test_common.h"
#include "joint_l2_bound_test.h"
#include "secret_encoding_test.h"
#include "../../../common/spectral_test.h"

#define MAX_MSG_BYTES 512

#if BIT_PUBLICKEYBYTES != 1264 || BIT_SECRETKEYBYTES != 1696
#error "BiT-128 key format mismatch"
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

#include "public_key_encoding_test.h"
static int test_public_key_encoding(void)
{
    return test_public_key_canonicalization() || test_public_key_wire_format() ||
           test_response_alphabet();
}

static int test_z0_rans_encoding(void)
{
    const int high_bound = (BIT_RESPONSE_COEFF_BOUND + 128) / 256;
    uint8_t sig[BIT_SIGNBYTES];
    uint8_t challenge[BIT_CHALLENGEBYTES];
    uint8_t challenge_out[BIT_CHALLENGEBYTES];
    polyvecm1 in = {0};
    polyvecm1 out = {0};
    polyveck h = {0};
    polyveck h_out = {0};
    test_fill_seed(challenge, sizeof(challenge), 512, 0);
    for (int i = 0; i < BIT_N; i++)
        in.vec[0].coeffs[i] = (int16_t)((i % 63) - 31);
    for (int i = 0; i < 2 * high_bound + 1; i++) {
        int z = (i - high_bound) * 256;
        if (z < -BIT_RESPONSE_COEFF_BOUND)
            z = -BIT_RESPONSE_COEFF_BOUND;
        if (z > BIT_RESPONSE_COEFF_BOUND)
            z = BIT_RESPONSE_COEFF_BOUND;
        in.vec[0].coeffs[i] = (int16_t)z;
    }

    /* Check the byte-aligned low stream independently of the decoder. */
    if (pack_sig(sig, &in, &h, challenge) != 0)
        return 1;
    for (int i = 0; i < BIT_N; i++)
        if (sig[BIT_CHALLENGEBYTES + i] != (uint8_t)in.vec[0].coeffs[i])
            return 2;

    if (unpack_sig(&out, &h_out, challenge_out, sig) != 0)
        return 1;
    if (memcmp(challenge, challenge_out, sizeof(challenge)) || memcmp(&in, &out, sizeof(in)) ||
        memcmp(&h, &h_out, sizeof(h_out)))
        return 2;
    /* Codec tests cover coefficients beyond the joint L2 radius. */
    if (!check_signature_bounds(&out, &h_out))
        return 3;

    in.vec[0].coeffs[0] = BIT_GAMMA1;
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
    if (unpack_sig(&out, &h_out, challenge_out, sig) == 0)
        return 4;
    if (pack_sig(sig, &in, &h, challenge) != 0)
        return 5;
    sig[BIT_SIGNBYTES - 1] = 1;
    if (unpack_sig(&out, &h_out, challenge_out, sig) == 0)
        return 6;

    in.vec[1].coeffs[0] = BIT_GAMMA1;
    if (pack_sig(sig, &in, &h, challenge) == 0)
        return 7;
    return 0;
}

static int test_rans_length_header_and_canonicality(void)
{
    uint8_t sig[BIT_SIGNBYTES];
    uint8_t challenge[BIT_CHALLENGEBYTES] = {0};
    uint8_t challenge_out[BIT_CHALLENGEBYTES];
    polyvecm1 in = {0}, out = {0};
    polyveck h = {0}, h_out = {0};
    size_t off = BIT_CHALLENGEBYTES + BIT_Z_LOW_BYTES;
    uint16_t code;

    for (int i = 0; i < BIT_L; i++)
        for (int j = 0; j < BIT_N; j++) {
            int r = (i * BIT_N + j) % 32;
            int high = r < 16 ? 0 : r < 23 ? -1 : r < 30 ? 1 : r == 30 ? -2 : 2;
            in.vec[i + 1].coeffs[j] = (int16_t)(high * 256);
        }
    if (pack_sig(sig, &in, &h, challenge) != 0)
        return 1;
    code = load16_le(sig + off);
    if (!(code & 0x8000U) || (code & 0x7FFFU) <= 127)
        return 2;
    if (unpack_sig(&out, &h_out, challenge_out, sig) != 0 || memcmp(&in, &out, sizeof(in)) ||
        memcmp(&h, &h_out, sizeof(h)))
        return 3;

    in.vec[0].coeffs[0] = 256;
    if (pack_sig(sig, &in, &h, challenge) != 0)
        return 4;
    code = load16_le(sig + off);
    if (code & 0x8000U)
        return 5;
    if (unpack_sig(&out, &h_out, challenge_out, sig) != 0 || memcmp(&in, &out, sizeof(in)))
        return 6;

    store16_le(sig + off, 511);
    sig[off + 2] = 255;
    if (unpack_sig(&out, &h_out, challenge_out, sig) == 0)
        return 7;
    if (pack_sig(sig, &in, &h, challenge) != 0)
        return 8;
    sig[off + 3] = 0;
    sig[off + 4] = 0;
    sig[off + 5] = 0;
    sig[off + 6] = 0;
    if (unpack_sig(&out, &h_out, challenge_out, sig) == 0)
        return 9;
    if (pack_sig(sig, &in, &h, challenge) != 0)
        return 10;
    sig[BIT_SIGNBYTES - 1] = 1;
    if (unpack_sig(&out, &h_out, challenge_out, sig) == 0)
        return 11;

    return 0;
}

static int test_rans_extreme_and_random_roundtrips(void)
{
    const int high_bound = (BIT_RESPONSE_COEFF_BOUND + 128) / 256;
    uint8_t sig[BIT_SIGNBYTES];
    uint8_t challenge[BIT_CHALLENGEBYTES] = {0};
    uint8_t challenge_out[BIT_CHALLENGEBYTES];
    polyvecm1 in = {0}, out = {0};
    polyveck h = {0}, h_out = {0};
    uint32_t state = 0x9e3779b9U;

    in.vec[0].coeffs[0] = -BIT_RESPONSE_COEFF_BOUND;
    in.vec[0].coeffs[1] = BIT_RESPONSE_COEFF_BOUND;
    in.vec[1].coeffs[0] = -BIT_RESPONSE_COEFF_BOUND;
    in.vec[1].coeffs[1] = BIT_RESPONSE_COEFF_BOUND;
    h.vec[0].coeffs[0] = -3;
    h.vec[0].coeffs[1] = 3;
    if (pack_sig(sig, &in, &h, challenge) != 0 ||
        unpack_sig(&out, &h_out, challenge_out, sig) != 0 || memcmp(&in, &out, sizeof(in)) ||
        memcmp(&h, &h_out, sizeof(h)))
        return 1;

    sig[BIT_CHALLENGEBYTES + 1] = 0; /* The top high symbol with zero low byte is gamma1. */
    if (unpack_sig(&out, &h_out, challenge_out, sig) == 0)
        return 20;
    if (pack_sig(sig, &in, &h, challenge) != 0)
        return 21;
    sig[BIT_CHALLENGEBYTES] = 0; /* The bottom high symbol with zero low byte is -gamma1. */
    if (unpack_sig(&out, &h_out, challenge_out, sig) == 0)
        return 22;

    /* Exercise every response high symbol in every polynomial. */
    for (int block = 0; block < BIT_L + 1; block++) {
        memset(&in, 0, sizeof(in));
        for (int j = 0; j < 2 * high_bound + 1; j++) {
            int z = (j - high_bound) * 256;
            if (z < -BIT_RESPONSE_COEFF_BOUND)
                z = -BIT_RESPONSE_COEFF_BOUND;
            if (z > BIT_RESPONSE_COEFF_BOUND)
                z = BIT_RESPONSE_COEFF_BOUND;
            in.vec[block].coeffs[j] = (int16_t)z;
        }
        if (pack_sig(sig, &in, &h, challenge) != 0 ||
            unpack_sig(&out, &h_out, challenge_out, sig) != 0 || memcmp(&in, &out, sizeof(in)))
            return 23;
    }

    for (int round = 0; round < 32; round++) {
        memset(&in, 0, sizeof(in));
        memset(&h, 0, sizeof(h));
        for (int i = 0; i < BIT_L + 1; i++) {
            for (int j = 0; j < BIT_N; j++) {
                state ^= state << 13;
                state ^= state >> 17;
                state ^= state << 5;
                if (i == 0)
                    in.vec[i].coeffs[j] = j == round ? (int16_t)((round & 1) ? 64 : -64)
                                                     : (int16_t)((int)(state % 63) - 31);
                else {
                    int high = (int)(state % 10);
                    high = high < 2 ? -1 : high < 4 ? 1 : 0;
                    in.vec[i].coeffs[j] = (int16_t)(high * 256 + (int8_t)(state >> 8));
                }
            }
        }
        for (int i = 0; i < BIT_K; i++)
            for (int j = 0; j < BIT_N; j++) {
                state ^= state << 13;
                state ^= state >> 17;
                state ^= state << 5;
                h.vec[i].coeffs[j] = (int16_t)((state % 10) < 2 ? -1 : (state % 10) < 4 ? 1 : 0);
            }
        if (pack_sig(sig, &in, &h, challenge) != 0) {
            fprintf(stderr, "random rANS pack failed at round %d\n", round);
            return 2;
        }
        if (unpack_sig(&out, &h_out, challenge_out, sig) != 0) {
            fprintf(stderr, "random rANS unpack failed at round %d\n", round);
            return 2;
        }
        if (memcmp(&in, &out, sizeof(in)) || memcmp(&h, &h_out, sizeof(h))) {
            fprintf(stderr, "random rANS roundtrip differed at round %d\n", round);
            return 2;
        }
    }

    for (int i = 0; i < BIT_K; i++)
        for (int j = 0; j < BIT_N; j++)
            h.vec[i].coeffs[j] = (int16_t)((j & 1) ? 3 : -3);
    if (pack_sig(sig, &in, &h, challenge) == 0)
        return 3;
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

    if (test_public_key_encoding() != 0) {
        printf("FAIL: full public-key encoding\n");
        failures++;
    } else {
        printf("PASS: full public-key encoding\n");
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

    {
        int rc = test_rans_length_header_and_canonicality();
        if (rc != 0) {
            printf("FAIL: rANS length header/canonicality (rc=%d)\n", rc);
            failures++;
        } else {
            printf("PASS: rANS length header/canonicality\n");
        }
    }
    {
        int rc = test_rans_extreme_and_random_roundtrips();
        if (rc != 0) {
            printf("FAIL: rANS extreme/random roundtrips (rc=%d)\n", rc);
            failures++;
        } else {
            printf("PASS: rANS extreme/random roundtrips\n");
        }
    }
    if (getenv("BIT_PACK_ONLY") != NULL)
        return failures != 0;
    if (getenv("BIT_SIGN_SMOKE_ONLY") != NULL) {
        int rc = test_keygen_sign_verify(0, 0, 64);
        if (rc != 0)
            printf("FAIL: ref-128 sign/verify smoke (rc=%d)\n", rc);
        else
            printf("PASS: ref-128 sign/verify smoke\n");
        return failures != 0 || rc != 0;
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
    printf("pk             = %d B  (seed + K*13-bit-poly) = %d + %d*%d\n", BIT_PUBLICKEYBYTES,
           BIT_SEEDBYTES, BIT_K, BIT_POLY_PUBLIC_BYTES);
    printf("sig            = %d B  (challenge + low(z0,z1) + lengths + rANS)\n", BIT_SIGNBYTES);
    printf("                  = %d + %d + %d + %d\n", BIT_CHALLENGEBYTES, BIT_Z_LOW_BYTES,
           BIT_SIG_RANS_LENGTH_BYTES, BIT_SIG_RANS_PAYLOAD_BYTES);
    printf("sk             = %d B\n", BIT_SECRETKEYBYTES);

    return failures != 0 ? 1 : 0;
}
