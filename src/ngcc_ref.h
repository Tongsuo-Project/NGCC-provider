#ifndef NGCC_REF_H
#define NGCC_REF_H
#include <stddef.h>
#include <openssl/core.h>
#include <openssl/core_dispatch.h>
#include <openssl/crypto.h>
#include <pthread.h>

/* Author callback sigs, with their original unsigned long long sizes. */
typedef int (*ngcc_keygen)(unsigned char *, unsigned long long *,
                        unsigned char *, unsigned long long *);
typedef int (*ngcc_enc)(unsigned char *, unsigned long long,
                     unsigned char *, unsigned long long *,
                     unsigned char *, unsigned long long *);
typedef int (*ngcc_dec)(unsigned char *, unsigned long long,
                     unsigned char *, unsigned long long,
                     unsigned char *, unsigned long long *);
typedef int (*ngcc_sign)(unsigned char *, unsigned long long,
                      unsigned char *, unsigned long long,
                      unsigned char *, unsigned long long *);
typedef int (*ngcc_verify)(unsigned char *, unsigned long long,
                        unsigned char *, unsigned long long,
                        unsigned char *, unsigned long long);

/* One concrete ref implementation: sizes, callbacks and its global RNG lock. */
typedef struct {
    const char *name, *id, *instance;
    int type, security_bits;
    size_t pk_len, sk_len, output_len, ss_len;
    ngcc_keygen keygen;
    ngcc_enc enc;
    ngcc_dec dec;
    ngcc_sign sign;
    ngcc_verify verify;
    int (*seed)(const unsigned char *, unsigned long long);
    pthread_mutex_t *lock;
} NGCC_SCHEME;
#endif
