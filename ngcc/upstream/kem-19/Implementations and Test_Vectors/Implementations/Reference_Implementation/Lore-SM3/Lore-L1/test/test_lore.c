#include <stdio.h>
#include <string.h>
#include "../api.h"
#include "../randombytes.h"
#include "../params.h"

#define NTESTS 1000

int main(void) {

    unsigned char pk[CRYPTO_PKE_PUBLICKEYBYTES];
    unsigned char sk[CRYPTO_PKE_SECRETKEYBYTES];
    unsigned char ct[CRYPTO_PKE_CIPHERTEXTBYTES];
    unsigned char msg[LORE_MSG_BYTES];  
    unsigned char pmsg[LORE_MSG_BYTES]; 
    unsigned long long pmlen;
    int i;
    int fails = 0;

    printf("Starting tests for %s\n", CRYPTO_ALGNAME);

    for (i = 0; i < NTESTS; i++) {
        randombytes(msg, LORE_MSG_BYTES);

        if (crypto_pke_keypair(pk, sk) != 0) {
            printf("ERROR in keypair on iteration %d\n", i);
            fails++;
            continue;
        }

        if (crypto_pke_encrypt(ct, msg, LORE_MSG_BYTES, pk) != 0) {
            printf("ERROR in encrypt on iteration %d\n", i);
            fails++;
            continue;
        }

        if (crypto_pke_decrypt(pmsg, &pmlen, ct, sk) != 0) {
            printf("ERROR in decrypt on iteration %d\n", i);
            fails++;
            continue;
        }

        if (pmlen != LORE_MSG_BYTES) {
            printf("ERROR: decrypted message length incorrect on iteration %d\n", i);
            fails++;
            continue;
        }
        if (memcmp(msg, pmsg, LORE_MSG_BYTES)) {
            printf("Original MSG[0] = %02X\n", msg[0]);
            printf("Decrypted MSG[0] = %02X\n", pmsg[0]);
            printf("ERROR: decrypted message does not match original on iteration %d\n", i);
            fails++;
            continue;
        }
    }

    if (fails) {
        printf("\nERROR: FAILED %d/%d tests!\n", fails, NTESTS);
    } else {
        printf("\nSUCCESS: PASSED all %d tests!\n", NTESTS);
    }

    return fails;
}