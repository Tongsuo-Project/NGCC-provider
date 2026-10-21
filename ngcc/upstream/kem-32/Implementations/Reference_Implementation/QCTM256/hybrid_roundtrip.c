#include <stdio.h>
#include <string.h>

#include "scheme_api.h"
#include "rng.h"

int main(void)
{
    unsigned char entropy_input[48];
    unsigned char pk[CRYPTO_PUBLICKEYBYTES];
    unsigned char sk[CRYPTO_SECRETKEYBYTES];
    unsigned char ct[CRYPTO_CIPHERTEXTBYTES];
    unsigned char ss[CRYPTO_BYTES];
    unsigned char ss1[CRYPTO_BYTES];
    int i;

    for (i = 0; i < 48; i++) {
        entropy_input[i] = (unsigned char)i;
    }
    randombytes_init(entropy_input, NULL, 256);

    if (crypto_kem_keypair(pk, sk) != SUCCESS) {
        puts("keypair=fail");
        return 1;
    }
    puts("keypair=ok");
    if (crypto_kem_enc(ct, ss, pk) != SUCCESS) {
        puts("enc=fail");
        return 1;
    }
    puts("enc=ok");
    if (crypto_kem_dec(ss1, ct, sk) != SUCCESS) {
        puts("dec=fail");
        return 1;
    }
    puts("dec=ok");
    if (memcmp(ss, ss1, CRYPTO_BYTES) != 0) {
        puts("ss=bad");
        return 1;
    }
    puts("ss=ok");
    return 0;
}
