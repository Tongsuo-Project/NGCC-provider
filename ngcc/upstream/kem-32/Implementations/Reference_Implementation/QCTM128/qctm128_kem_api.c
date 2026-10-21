#include <string.h>

#include "qctm128_kem_api.h"

#define KEM_KAT_ENCAPS_MAX_TRIES 256

int KEM_KeyGen(unsigned char *pk, unsigned char *sk)
{
    return crypto_kem_keypair(pk, sk);
}

int KEM_Encaps(unsigned char *ct, unsigned char *ss, const unsigned char *pk)
{
    return crypto_kem_enc(ct, ss, pk);
}

int KEM_Decaps(unsigned char *ss, const unsigned char *ct, const unsigned char *sk)
{
    return crypto_kem_dec(ss, ct, sk);
}

int KEM_EncapsDecapsChecked(unsigned char *ct,
                            unsigned char *ss,
                            unsigned char *ss_dec,
                            const unsigned char *pk,
                            const unsigned char *sk)
{
    int i;

    if (ct == NULL || ss == NULL || ss_dec == NULL || pk == NULL ||
        sk == NULL) {
        return FAIL;
    }

    for (i = 0; i < KEM_KAT_ENCAPS_MAX_TRIES; i++) {
        if (KEM_Encaps(ct, ss, pk) != SUCCESS) {
            return FAIL;
        }
        if (KEM_Decaps(ss_dec, ct, sk) == SUCCESS &&
            memcmp(ss, ss_dec, KEM_BYTES) == 0) {
            return SUCCESS;
        }
    }

    return FAIL;
}

int KeyGen(unsigned char *pk, unsigned char *sk)
{
    return KEM_KeyGen(pk, sk);
}

int Encaps(unsigned char *ct, unsigned char *ss, const unsigned char *pk)
{
    return KEM_Encaps(ct, ss, pk);
}

int Decaps(unsigned char *ss, const unsigned char *ct, const unsigned char *sk)
{
    return KEM_Decaps(ss, ct, sk);
}
