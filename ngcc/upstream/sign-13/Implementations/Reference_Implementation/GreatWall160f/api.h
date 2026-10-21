#ifndef GREATWALL160_API_H
#define GREATWALL160_API_H

#include "faest.h"
#define CRYPTO_SECRETKEYBYTES GREATWALL_SECRET_KEY_BYTES
#define CRYPTO_PUBLICKEYBYTES GREATWALL_PUBLIC_KEY_BYTES
#define CRYPTO_BYTES FAEST_SIGNATURE_BYTES
#define CRYPTO_ALGNAME GREATWALL_INSTANCE_NAME
#define GREATWALL160_ERR_MISSING_CONSTANTS (-2)

int crypto_sign_keypair(unsigned char* pk, unsigned char* sk);
int crypto_sign(unsigned char* sm, unsigned long long* smlen,
                const unsigned char* m, unsigned long long mlen,
                const unsigned char* sk);
int crypto_sign_open(unsigned char* m, unsigned long long* mlen,
                     const unsigned char* sm, unsigned long long smlen,
                     const unsigned char* pk);
#endif
