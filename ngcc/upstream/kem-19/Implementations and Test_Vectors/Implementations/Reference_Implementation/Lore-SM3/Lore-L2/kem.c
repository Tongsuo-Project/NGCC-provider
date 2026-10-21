#include <stddef.h>
#include <string.h>
#include "kem.h"
#include "indcpa.h"
#include "symmetric.h"
#include "randombytes.h"
#include "verify.h"
#include "params.h"
#include "lore_hash.h"

static void secure_clear(void *v, size_t n) {
    volatile uint8_t *p = (volatile uint8_t *)v;
    while (n--) *p++ = 0;
}

/* Deterministic generation of public and secret key */
int crypto_kem_keypair_derand(unsigned char *pk, unsigned char *sk, const unsigned char *coins)
{
    indcpa_keypair_derand(pk, sk, coins);
    /* sk = sk' || pk || H(pk) || z */
    unsigned char *sk_rest = sk + LORE_SECRETKEYBYTES;
    memcpy(sk_rest, pk, LORE_PUBLICKEYBYTES);
    sk_rest += LORE_PUBLICKEYBYTES;
    lore_hash_h(sk_rest, LORE_DOMAIN_PUBLIC_KEY, pk, LORE_PUBLICKEYBYTES);
    sk_rest += LORE_SYMBYTES;
    for(size_t i = 0; i < LORE_SYMBYTES; i++) {
        sk_rest[i] = coins[LORE_SYMBYTES + i];
    }

    return 0;
}

/* Generates public and private key for the CCA-secure KEM. */
int crypto_kem_keypair(unsigned char *pk, unsigned char *sk)
{
    unsigned char coins[2 * LORE_SYMBYTES];
    randombytes(coins, 2 * LORE_SYMBYTES);
    crypto_kem_keypair_derand(pk, sk, coins);
    secure_clear(coins, sizeof coins);
    return 0;
}

/* Generates ciphertext and shared secret for a given public key. */
int crypto_kem_enc_derand(unsigned char *ct, unsigned char *ss, const unsigned char *pk, const unsigned char *coins)
{
    unsigned char kr[2 * LORE_SYMBYTES];
    unsigned char buf[2 * LORE_SYMBYTES];

    lore_hash_h(buf, LORE_DOMAIN_MESSAGE, coins, LORE_MSG_BYTES);
    lore_hash_h(buf + LORE_SYMBYTES, LORE_DOMAIN_PUBLIC_KEY, pk, LORE_PUBLICKEYBYTES);
    lore_hash_g(kr, buf, 2 * LORE_SYMBYTES);

    indcpa_enc(ct, coins, pk, kr + LORE_SYMBYTES);

    unsigned char kdf_buf[2 * LORE_SYMBYTES];
    memcpy(kdf_buf, kr, LORE_SYMBYTES);
    lore_hash_h(kdf_buf + LORE_SYMBYTES, LORE_DOMAIN_CIPHERTEXT, ct, LORE_CIPHERTEXTBYTES);
    lore_kdf(ss, kdf_buf, sizeof kdf_buf);
    secure_clear(kr, sizeof kr);
    secure_clear(buf, sizeof buf);
    secure_clear(kdf_buf, sizeof kdf_buf);
    return 0;
}

/* Generates ciphertext and shared secret for a given public key. */
int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk)
{
    unsigned char coins[LORE_MSG_BYTES];
    randombytes(coins, LORE_MSG_BYTES);
    crypto_kem_enc_derand(ct, ss, pk, coins);
    secure_clear(coins, sizeof coins);
    return 0;
}

/* Generates shared secret for a given ciphertext and private key. */
int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk)
{
    int fail;
    unsigned char mu[LORE_MSG_BYTES];
    unsigned char kr[2 * LORE_SYMBYTES];
    unsigned char buf[2 * LORE_SYMBYTES];
    unsigned char ct_cmp[LORE_CIPHERTEXTBYTES];
    memset(ct_cmp, 0, sizeof ct_cmp);
    unsigned char ss_invalid[LORE_SYMBYTES];

    const unsigned char *sk_indcpa = sk;
    const unsigned char *pk = sk + LORE_SECRETKEYBYTES;
    const unsigned char *pkh = pk + LORE_PUBLICKEYBYTES;
    const unsigned char *z = pkh + LORE_SYMBYTES;

    if (indcpa_dec(mu, ct, sk_indcpa)) {
        memset(ss, 0, LORE_SYMBYTES);
        return -1;
    }

    lore_hash_h(buf, LORE_DOMAIN_MESSAGE, mu, LORE_MSG_BYTES);
    memcpy(buf + LORE_SYMBYTES, pkh, LORE_SYMBYTES);
    lore_hash_g(kr, buf, 2 * LORE_SYMBYTES);

    indcpa_enc(ct_cmp, mu, pk, kr + LORE_SYMBYTES);

    fail = verify(ct, ct_cmp, LORE_CIPHERTEXTBYTES);

    unsigned char kdf_buf[2 * LORE_SYMBYTES];
    memcpy(kdf_buf, z, LORE_SYMBYTES);
    lore_hash_h(kdf_buf + LORE_SYMBYTES, LORE_DOMAIN_CIPHERTEXT, ct, LORE_CIPHERTEXTBYTES);
    lore_kdf(ss_invalid, kdf_buf, sizeof kdf_buf);

    unsigned char ss_valid[LORE_SYMBYTES];
    memcpy(kdf_buf, kr, LORE_SYMBYTES);
    lore_kdf(ss_valid, kdf_buf, sizeof kdf_buf);
    // cmov reads ss even when fail is zero.
    memcpy(ss, ss_valid, LORE_SYMBYTES);
    cmov(ss, ss_invalid, LORE_SYMBYTES, (unsigned char)fail);

    secure_clear(mu, LORE_MSG_BYTES);
    secure_clear(kr, 2 * LORE_SYMBYTES);
    secure_clear(buf, sizeof buf);
    secure_clear(kdf_buf, sizeof kdf_buf);
    secure_clear(ss_valid, sizeof ss_valid);
    secure_clear(ss_invalid, sizeof ss_invalid);
    return 0;
}
