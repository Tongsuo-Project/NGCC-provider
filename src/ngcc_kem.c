/*
 * Copyright 2026 The Tongsuo Project Authors. All Rights Reserved.
 * Licensed under the Apache License 2.0; see LICENSE.
 */
#include <openssl/evp.h>
#include "ngcc_prov.h"

/* Encapsulate for the public key retained by vctx. NULL output queries sizes. */
static int ngcc_kem_encaps(void *vctx, unsigned char *ct, size_t *ct_len,
                          unsigned char *ss, size_t *ss_len)
{
    NGCC_OP *ctx = vctx;
    NGCC_SCHEME *scheme;
    unsigned long long ct_len_ull, ss_len_ull;
    int ret;

    if (ctx == NULL || ct_len == NULL || ss_len == NULL
        || ctx->key == NULL || ctx->key->pk == NULL)
        return 0;
    scheme = ctx->scheme;
    ct_len_ull = scheme->output_len;
    ss_len_ull = scheme->ss_len;
    if (ct == NULL || ss == NULL) {
        *ct_len = scheme->output_len;
        *ss_len = scheme->ss_len;
        return 1;
    }
    if (*ct_len < ct_len_ull || *ss_len < ss_len_ull)
        return 0;
    pthread_mutex_lock(scheme->lock);
    if (!ngcc_random_seed(scheme, ctx->libctx)) {
        pthread_mutex_unlock(scheme->lock);
        return 0;
    }
    ret = scheme->enc(ctx->key->pk, scheme->pk_len, ss, &ss_len_ull,
                       ct, &ct_len_ull);
    pthread_mutex_unlock(scheme->lock);
    if (ret != 0 || ct_len_ull != scheme->output_len
        || ss_len_ull != scheme->ss_len) {
        OPENSSL_cleanse(ss, scheme->ss_len);
        return 0;
    }
    *ct_len = (size_t)ct_len_ull;
    *ss_len = (size_t)ss_len_ull;
    return 1;
}

/* Reject a wrong wire length before passing ciphertext to author code. */
static int ngcc_kem_decaps(void *vctx, unsigned char *ss, size_t *ss_len,
                          const unsigned char *ct, size_t ct_len)
{
    NGCC_OP *ctx = vctx;
    NGCC_SCHEME *scheme;
    unsigned long long ss_len_ull;
    int ret;

    if (ctx == NULL || ss_len == NULL || ctx->key == NULL
        || ctx->key->sk == NULL)
        return 0;
    scheme = ctx->scheme;
    ss_len_ull = scheme->ss_len;
    if (ss == NULL) {
        *ss_len = scheme->ss_len;
        return 1;
    }
    if (*ss_len < ss_len_ull || ct == NULL || ct_len != scheme->output_len)
        return 0;
    pthread_mutex_lock(scheme->lock);
    ret = scheme->dec(ctx->key->sk, scheme->sk_len, (unsigned char *)ct,
                       ct_len, ss, &ss_len_ull);
    pthread_mutex_unlock(scheme->lock);
    if (ret != 0 || ss_len_ull != scheme->ss_len) {
        OPENSSL_cleanse(ss, scheme->ss_len);
        return 0;
    }
    *ss_len = (size_t)ss_len_ull;
    return 1;
}

/* Keep KEM dispatches in ngcc_kem.c, as in the original provider layout. */
#define NGCC_KEM(tag, getter, names, primary)                                  \
static void *ngcc_##tag##_kem_newctx(void *provctx)                            \
{                                                                              \
    return ngcc_op_new(getter(), provctx);                                     \
}                                                                              \
const OSSL_DISPATCH ngcc_##tag##_functions[] = {                               \
    NGCC_DISPATCH(OSSL_FUNC_KEM_NEWCTX, ngcc_##tag##_kem_newctx),              \
    NGCC_DISPATCH(OSSL_FUNC_KEM_FREECTX, ngcc_op_free),                        \
    NGCC_DISPATCH(OSSL_FUNC_KEM_DUPCTX, ngcc_op_dup),                          \
    NGCC_DISPATCH(OSSL_FUNC_KEM_ENCAPSULATE_INIT, ngcc_op_init),               \
    NGCC_DISPATCH(OSSL_FUNC_KEM_ENCAPSULATE, ngcc_kem_encaps),                 \
    NGCC_DISPATCH(OSSL_FUNC_KEM_DECAPSULATE_INIT, ngcc_op_init),               \
    NGCC_DISPATCH(OSSL_FUNC_KEM_DECAPSULATE, ngcc_kem_decaps),                 \
    { 0, NULL }                                                                \
};
#include "ngcc_kem_algorithms.inc"
#undef NGCC_KEM
