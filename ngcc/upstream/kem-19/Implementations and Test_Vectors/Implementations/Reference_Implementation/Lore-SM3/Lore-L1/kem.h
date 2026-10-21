#ifndef KEM_H
#define KEM_H

#include "params.h"

int crypto_kem_keypair_derand(unsigned char *pk, unsigned char *sk, const unsigned char *coins);
int crypto_kem_keypair(unsigned char *pk, unsigned char *sk);

int crypto_kem_enc_derand(unsigned char *ct, unsigned char *ss, const unsigned char *pk, const unsigned char *coins);
int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk);

/* Invalid sparse-key encodings: -1 and zero ss. Invalid ciphertexts with
 * valid keys: implicit rejection and return 0. Buffers must be full-sized. */
int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

#endif
