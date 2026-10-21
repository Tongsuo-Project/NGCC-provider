#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../api.h"
#include "../indcpa.h"
#include "../symmetric.h"
#include "../lore_hash.h"

/* Output buffers have explicit guards. These checks complement ASan. */
#define GUARDED(name, length) struct { uint8_t before[16], bytes[length], after[16]; } name
#define INTACT(name) (guard_ok((name).before) && guard_ok((name).after))

static int guard_ok(const uint8_t guard[16])
{
    for (unsigned int i = 0; i < 16; ++i) if (guard[i] != 0xa5) return 0;
    return 1;
}

int main(void)
{
    GUARDED(pk, CRYPTO_PUBLICKEYBYTES);
    GUARDED(sk, CRYPTO_SECRETKEYBYTES);
    GUARDED(ct, CRYPTO_CIPHERTEXTBYTES);
    GUARDED(ss, CRYPTO_BYTES);
    GUARDED(message, LORE_MSG_BYTES);
    uint8_t kp_coins[CRYPTO_KEYPAIRCOINBYTES];
    uint8_t enc_coins[CRYPTO_ENCCOINBYTES];
    uint8_t enc_ss[CRYPTO_BYTES], expected[CRYPTO_BYTES];
    const unsigned long long bad_lengths[] = {0, LORE_MSG_BYTES - 1,
                                             LORE_MSG_BYTES + 1, ULLONG_MAX};
    for (unsigned int trial = 0; trial < 256; ++trial) {
        memset(&pk, 0xa5, sizeof pk);
        memset(&sk, 0xa5, sizeof sk);
        memset(&ct, 0xa5, sizeof ct);
        memset(&ss, 0xa5, sizeof ss);
        memset(&message, 0xa5, sizeof message);
        for (unsigned int i = 0; i < sizeof kp_coins; ++i)
            kp_coins[i] = (uint8_t)(trial + 31 * i);
        for (unsigned int i = 0; i < sizeof enc_coins; ++i)
            enc_coins[i] = (uint8_t)(trial ^ (17 * i));
        if (crypto_kem_keypair_derand(pk.bytes, sk.bytes, kp_coins) ||
            crypto_kem_enc_derand(ct.bytes, enc_ss, pk.bytes, enc_coins) ||
            crypto_kem_dec(ss.bytes, ct.bytes, sk.bytes) ||
            memcmp(ss.bytes, enc_ss, sizeof enc_ss)) return 1;
        /* Follow the versioned call-layer transcript and KEM selection. */
        uint8_t hashes[2 * LORE_SYMBYTES], kr[2 * LORE_SYMBYTES];
        uint8_t kdf_input[2 * LORE_SYMBYTES], expected_ct[CRYPTO_CIPHERTEXTBYTES];
        lore_hash_h(hashes, LORE_DOMAIN_MESSAGE, enc_coins, LORE_MSG_BYTES);
        lore_hash_h(hashes + LORE_SYMBYTES, LORE_DOMAIN_PUBLIC_KEY, pk.bytes, CRYPTO_PUBLICKEYBYTES);
        if (memcmp(hashes + LORE_SYMBYTES,
                   sk.bytes + LORE_SECRETKEYBYTES + LORE_PUBLICKEYBYTES,
                   LORE_SYMBYTES)) return 1;
        lore_hash_g(kr, hashes, sizeof hashes);
        indcpa_enc(expected_ct, enc_coins, pk.bytes, kr + LORE_SYMBYTES);
        if (memcmp(expected_ct, ct.bytes, sizeof expected_ct)) return 1;
        memcpy(kdf_input, kr, LORE_SYMBYTES);
        lore_hash_h(kdf_input + LORE_SYMBYTES, LORE_DOMAIN_CIPHERTEXT, ct.bytes, CRYPTO_CIPHERTEXTBYTES);
        lore_kdf(expected, kdf_input, sizeof kdf_input);
        if (memcmp(expected, enc_ss, sizeof expected)) return 1;
        unsigned long long length = 0;
        if (crypto_pke_decrypt(message.bytes, &length, ct.bytes, sk.bytes) ||
            length != LORE_MSG_BYTES ||
            memcmp(message.bytes, enc_coins, LORE_MSG_BYTES)) return 1;

        /* The format reserves trailing padding. Its mutation must trigger
         * implicit rejection, even though the recovered message is unchanged. */
        if (ct.bytes[CRYPTO_CIPHERTEXTBYTES - 1] != 0) return 1;
        ct.bytes[CRYPTO_CIPHERTEXTBYTES - 1] = (uint8_t)(1 + trial % 255);
        uint8_t fail_input[2 * LORE_SYMBYTES];
        memcpy(fail_input, sk.bytes + CRYPTO_SECRETKEYBYTES - LORE_SYMBYTES,
               LORE_SYMBYTES);
        lore_hash_h(fail_input + LORE_SYMBYTES, LORE_DOMAIN_CIPHERTEXT, ct.bytes, CRYPTO_CIPHERTEXTBYTES);
        lore_kdf(expected, fail_input, sizeof fail_input);
        if (crypto_kem_dec(ss.bytes, ct.bytes, sk.bytes) ||
            memcmp(ss.bytes, expected, sizeof expected)) return 1;

        for (unsigned int i = 0; i < sizeof bad_lengths / sizeof bad_lengths[0]; ++i) {
            memset(ct.bytes, 0xa5, sizeof ct.bytes);
            if (crypto_pke_encrypt(ct.bytes, enc_coins, bad_lengths[i], pk.bytes) != -1)
                return 1;
            for (unsigned int j = 0; j < sizeof ct.bytes; ++j)
                if (ct.bytes[j] != 0xa5) return 1;
        }
        if (!INTACT(pk) || !INTACT(sk) || !INTACT(ct) || !INTACT(ss) || !INTACT(message))
            return 1;
    }
    puts("API: 256 guarded roundtrips, exact rejection keys, and length checks passed");
    return 0;
}
