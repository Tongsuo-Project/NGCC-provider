#include "qctm256_kem_api.h"

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
