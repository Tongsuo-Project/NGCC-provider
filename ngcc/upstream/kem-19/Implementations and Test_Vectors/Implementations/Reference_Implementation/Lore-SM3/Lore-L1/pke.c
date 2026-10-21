#include "pke.h"
#include "indcpa.h"
#include "symmetric.h"
#include "randombytes.h"
#include "string.h"
#include "api.h"

/* Generates public and private key for the Lore PKE. */
int crypto_pke_keypair(unsigned char *pk, unsigned char *sk) {
    indcpa_keypair(pk, sk);
    return 0;
}

/* Encrypts a message using the Lore PKE. */
int crypto_pke_encrypt(unsigned char *ct,
                       const unsigned char *m,
                       unsigned long long mlen,
                       const unsigned char *pk) {
    if (mlen != LORE_MSG_BYTES) {
        return -1;
    }

    unsigned char coins[LORE_SYMBYTES];
    randombytes(coins, LORE_SYMBYTES); // Coins for randomness

    indcpa_enc(ct, m, pk, coins);
    volatile unsigned char *wipe = coins;
    for (size_t i = 0; i < sizeof coins; ++i) wipe[i] = 0;
    return 0;
}

/* Decrypts a ciphertext using the Lore PKE. */
int crypto_pke_decrypt(unsigned char *m,
                       unsigned long long *mlen,
                       const unsigned char *ct,
                       const unsigned char *sk) {
    if (indcpa_dec(m, ct, sk)) {
        *mlen = 0;
        return -1;
    }
    *mlen = LORE_MSG_BYTES;
    return 0;
}

