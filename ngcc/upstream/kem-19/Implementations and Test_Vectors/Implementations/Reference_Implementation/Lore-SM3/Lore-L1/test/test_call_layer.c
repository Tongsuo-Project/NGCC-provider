#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../api.h"
#include "../indcpa.h"
#include "../lore_hash.h"
#include "../symmetric.h"

#ifdef LORE_BACKEND_SM3
#include "../auxfunc.h"
#include "../lore_sm3_wrapper.h"
#else
#include "../fips202.h"
#endif

static void reference_header(uint8_t out[LORE_FRAME_BYTES], uint8_t domain,
                              uint32_t counter, size_t length)
{
    const uint8_t fixed[8] = {'L', 'O', 'R', 'E', 1,
#ifdef LORE_BACKEND_SM3
                              2,
#else
                              1,
#endif
                              LORE_LEVEL, domain};
    memcpy(out, fixed, sizeof fixed);
    for (unsigned int i = 0; i < 8; ++i)
        out[8 + i] = (uint8_t)((uint64_t)length >> (56 - 8 * i));
    for (unsigned int i = 0; i < 4; ++i)
        out[16 + i] = (uint8_t)(counter >> (24 - 8 * i));
}

static int check_call_layer(void)
{
    uint8_t input[LORE_SYMBYTES], frame[LORE_FRAME_BYTES + LORE_SYMBYTES];
    uint8_t actual[1024], expected[1024], repeated[1024], digest[128];
    for (size_t i = 0; i < sizeof input; ++i) input[i] = (uint8_t)(17 + i);
    reference_header(frame, LORE_DOMAIN_KEYGEN, 0, sizeof input);
    memcpy(frame + LORE_FRAME_BYTES, input, sizeof input);
    lore_expand_seed(actual, input);
#ifdef LORE_BACKEND_SM3
    if (pseudoXOF(2ULL * LORE_SYMBYTES * 8, frame,
                   (unsigned long long)sizeof frame * 8, expected)) return 1;
#else
    shake256(expected, 2 * LORE_SYMBYTES, frame, sizeof frame);
#endif
    if (memcmp(actual, expected, 2 * LORE_SYMBYTES)) return 1;

    reference_header(frame, LORE_DOMAIN_MESSAGE, 0, sizeof input);
    lore_hash_h(actual, LORE_DOMAIN_MESSAGE, input, sizeof input);
#ifdef LORE_BACKEND_SM3
    const int hash_bits = LORE_SYMBYTES == 32 ? 256 : 512;
    if ((hash_bits == 256 ? sm3hash(hash_bits, frame, sizeof frame * 8ULL, digest) :
         pseudohash(hash_bits, frame, sizeof frame * 8ULL, digest))) return 1;
    memcpy(expected, digest, LORE_SYMBYTES);
#else
    shake256(expected, LORE_SYMBYTES, frame, sizeof frame);
    (void)digest;
#endif
    if (memcmp(actual, expected, LORE_SYMBYTES)) return 1;
    lore_hash_h(repeated, LORE_DOMAIN_PUBLIC_KEY, input, sizeof input);
    if (!memcmp(actual, repeated, LORE_SYMBYTES)) return 1;

    reference_header(frame, LORE_DOMAIN_G, 0, sizeof input);
    lore_hash_g(actual, input, sizeof input);
#ifdef LORE_BACKEND_SM3
    if (pseudohash(2 * LORE_SYMBYTES * 8, frame,
                    sizeof frame * 8ULL, expected)) return 1;
#else
    shake256(expected, 2 * LORE_SYMBYTES, frame, sizeof frame);
#endif
    if (memcmp(actual, expected, 2 * LORE_SYMBYTES)) return 1;

    reference_header(frame, LORE_DOMAIN_KDF, 0, sizeof input);
    lore_kdf(actual, input, sizeof input);
#ifdef LORE_BACKEND_SM3
    if (pseudoXOF(LORE_SYMBYTES * 8ULL, frame,
                   sizeof frame * 8ULL, expected)) return 1;
#else
    shake256(expected, LORE_SYMBYTES, frame, sizeof frame);
#endif
    if (memcmp(actual, expected, LORE_SYMBYTES)) return 1;

    /* Expansion lengths must not select a different PRF stream. */
    prf(actual, sizeof actual, input, 7);
    prf(repeated, 71, input, 7);
    if (memcmp(actual, repeated, 71)) return 1;
    prf(repeated, sizeof repeated, input, 8);
    if (!memcmp(actual, repeated, sizeof actual)) return 1;

    xof_state split, whole;
    xof_absorb(&split, input, 1, 0);
    xof_absorb(&whole, input, 1, 0);
#ifdef LORE_BACKEND_SM3
    const size_t blockbytes = XOF_BLOCKBYTES;
#else
    const size_t blockbytes = SHAKE128_RATE;
#endif
    xof_squeezeblocks(actual, 1, &split);
    xof_squeezeblocks(actual + blockbytes, 3, &split);
    xof_squeezeblocks(repeated, 4, &whole);
    if (memcmp(actual, repeated, 4 * blockbytes)) return 1;
#ifdef LORE_BACKEND_SM3
    reference_header(frame, LORE_DOMAIN_MATRIX, 256, sizeof input);
    if (pseudoXOF(4 * blockbytes * 8ULL, frame,
                   sizeof frame * 8ULL, expected) ||
        memcmp(actual, expected, 4 * blockbytes)) return 1;
    /* Read partial blocks and reject lengths before passing them to API_PKC. */
    for (size_t offset = 0; offset < 70; ++offset) {
        if (lore_sm3_xof_read(actual, 47, offset, LORE_DOMAIN_SAMPLE, 7,
                              input, sizeof input)) return 1;
        prf(expected, 128, input, 7);
        if (memcmp(actual, expected + offset, 47)) return 1;
    }
    if (lore_sm3_xof(actual, SIZE_MAX, LORE_DOMAIN_SAMPLE, 0, input, sizeof input) == 0 ||
        lore_sm3_hash(actual, 31, LORE_DOMAIN_MESSAGE, 0, input, sizeof input) == 0 ||
        lore_sm3_xof_read(actual, 16, SIZE_MAX - 7, LORE_DOMAIN_SAMPLE, 0,
                          input, sizeof input) == 0 ||
        lore_sm3_hash(actual, LORE_SYMBYTES, LORE_DOMAIN_MESSAGE, 0, NULL, 1) == 0 ||
        lore_sm3_hash(actual, LORE_SYMBYTES, LORE_DOMAIN_MESSAGE, 0, input, SIZE_MAX) == 0 ||
        lore_sm3_xof(NULL, 1, LORE_DOMAIN_SAMPLE, 0, input, sizeof input) == 0 ||
        lore_sm3_xof(actual, 1, LORE_DOMAIN_SAMPLE, 0, NULL, 1) == 0) return 1;
#endif
    return 0;
}

static int check_seed_tail(void)
{
    uint8_t coins[CRYPTO_KEYPAIRCOINBYTES] = {0};
    uint8_t pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
    uint8_t changed_pk[CRYPTO_PUBLICKEYBYTES], changed_sk[CRYPTO_SECRETKEYBYTES];
    if (crypto_kem_keypair_derand(pk, sk, coins)) return 1;
    for (size_t i = 32; i < LORE_SYMBYTES; ++i) {
        coins[i] = 1;
        if (crypto_kem_keypair_derand(changed_pk, changed_sk, coins) ||
            !memcmp(pk, changed_pk, sizeof pk) ||
            !memcmp(sk, changed_sk, LORE_SECRETKEYBYTES)) return 1;
        coins[i] = 0;
    }
    /* z remains a separate rejection secret; it is not core-key entropy. */
    coins[LORE_SYMBYTES] = 1;
    if (crypto_kem_keypair_derand(changed_pk, changed_sk, coins) ||
        memcmp(pk, changed_pk, sizeof pk) ||
        memcmp(sk, changed_sk, LORE_SECRETKEYBYTES) ||
        !memcmp(sk + CRYPTO_SECRETKEYBYTES - LORE_SYMBYTES,
                 changed_sk + CRYPTO_SECRETKEYBYTES - LORE_SYMBYTES,
                 LORE_SYMBYTES)) return 1;
    return 0;
}

int main(void)
{
    if (check_call_layer() || check_seed_tail()) return 1;
    printf("L%d: %d-byte core seed, call-layer domains, and stream checks passed\n",
           LORE_LEVEL, LORE_SYMBYTES);
    return 0;
}
