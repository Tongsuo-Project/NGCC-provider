/* Test-only failing RNG: must never be linked into production targets. */
#include "api.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
int rand_bytes(uint8_t* out, size_t bytes) { (void)out; (void)bytes; return -1; }
int main(void)
{
    unsigned char pk[CRYPTO_PUBLICKEYBYTES] = {0};
    unsigned char sk[CRYPTO_SECRETKEYBYTES] = {0};
    unsigned char msg[1] = {0};
    unsigned char* sm = calloc(CRYPTO_BYTES + 1, 1);
    assert(sm);
    unsigned long long len = 123;
    assert(crypto_sign_keypair(pk, sk) != 0);
    assert(crypto_sign(sm, &len, msg, 1, sk) != 0 && len == 0);
    len = 123;
    assert(crypto_sign_open(msg, &len, sm, CRYPTO_BYTES - 1, pk) != 0 && len == 0);
    free(sm);
    puts("PASS: RNG failure propagation and short signed-message rejection");
    return 0;
}
