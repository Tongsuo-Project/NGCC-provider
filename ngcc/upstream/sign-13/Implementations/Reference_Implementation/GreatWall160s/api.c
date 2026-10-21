#include <limits.h>
#include <string.h>
#include "api.h"
#include "faest.h"
#include "randomness.h"
#include "greatwall.h"
int crypto_sign_keypair(unsigned char* pk,unsigned char* sk) {
    if(!greatwall160_constants_available())return GREATWALL160_ERR_MISSING_CONSTANTS;
    if(!pk||!sk)return -1;
    do {
        if(rand_bytes(sk,GREATWALL_SECRET_KEY_BYTES)!=0)return -1;
        sk[GREATWALL_FIELD_BYTES-1]&=0x07;
        sk[GREATWALL_SECRET_KEY_BYTES-1]&=0x07;
    } while(!faest_pubkey(pk,sk));
    return 0;
}
int crypto_sign(unsigned char* sm,unsigned long long* smlen,const unsigned char* m,unsigned long long mlen,const unsigned char* sk) {
    if(!smlen)return -1;
    *smlen=0;
    if(!greatwall160_constants_available())return GREATWALL160_ERR_MISSING_CONSTANTS;
    if(!sm||!sk||(!m&&mlen)||mlen>SIZE_MAX-FAEST_SIGNATURE_BYTES||mlen>ULLONG_MAX-FAEST_SIGNATURE_BYTES)return -1;
    uint8_t random_seed[SECURITY_PARAM/8];
    if(rand_bytes(random_seed,sizeof(random_seed))!=0)return -1;
    if(mlen)memmove(sm,m,(size_t)mlen);
    if(!faest_sign(sm+mlen,sm,(size_t)mlen,sk,random_seed,sizeof(random_seed)))return -1;
    *smlen=mlen+FAEST_SIGNATURE_BYTES;
    return 0;
}
int crypto_sign_open(unsigned char* m,unsigned long long* mlen,const unsigned char* sm,unsigned long long smlen,const unsigned char* pk) {
    if(!mlen)return -1;
    *mlen=0;
    if(!greatwall160_constants_available())return GREATWALL160_ERR_MISSING_CONSTANTS;
    if(!sm||!pk||smlen<FAEST_SIGNATURE_BYTES||smlen>SIZE_MAX||smlen>PTRDIFF_MAX)return -1;
    size_t message_len=(size_t)smlen-FAEST_SIGNATURE_BYTES;
    if((!m&&message_len)||!faest_verify(sm+message_len,sm,message_len,pk))return -1;
    if(message_len)memmove(m,sm,message_len);
    *mlen=message_len;
    return 0;
}
