#ifndef QCTM256_KEM_API_H
#define QCTM256_KEM_API_H

#include "scheme_api.h"


#define PKC_ALGNAME CRYPTO_ALGNAME
#define PKC_PUBLICKEYBYTES CRYPTO_PUBLICKEYBYTES
#define PKC_SECRETKEYBYTES CRYPTO_SECRETKEYBYTES
#define PKC_CIPHERTEXTBYTES CRYPTO_CIPHERTEXTBYTES
#define PKC_BYTES CRYPTO_BYTES

#define KEM_PUBLICKEYBYTES CRYPTO_PUBLICKEYBYTES
#define KEM_SECRETKEYBYTES CRYPTO_SECRETKEYBYTES
#define KEM_CIPHERTEXTBYTES CRYPTO_CIPHERTEXTBYTES
#define KEM_BYTES CRYPTO_BYTES

int KEM_KeyGen(unsigned char *pk, unsigned char *sk);
int KEM_Encaps(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int KEM_Decaps(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

int KeyGen(unsigned char *pk, unsigned char *sk);
int Encaps(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int Decaps(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

#endif
