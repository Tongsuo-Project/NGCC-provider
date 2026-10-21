/**
 * @file api.h
 */

#ifndef HEP_QC_API_H
#define HEP_QC_API_H

#define CRYPTO_ALGNAME "HEP-QC-5"

#define CRYPTO_SECRETKEYBYTES  1852581
#define CRYPTO_PUBLICKEYBYTES  1852485
#define CRYPTO_BYTES           32
#define CRYPTO_CIPHERTEXTBYTES 14421

// As a technicality, the public key is appended to the secret key in order to respect the NIST API.
// Without this constraint, CRYPTO_SECRETKEYBYTES would be defined as 32

int crypto_kem_keypair(unsigned char *pk, unsigned char *sk);
int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

#endif  // HEP_QC_API_H
