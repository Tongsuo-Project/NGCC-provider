/*
 * Copyright 2026 The Tongsuo Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/core.h>
#include <openssl/core_dispatch.h>
#include <openssl/core_names.h>
#include <openssl/params.h>
#include <openssl/proverr.h>
#include <openssl/x509.h>
#include <stdint.h>
#include <string.h>

#include "ngcc_prov.h"

typedef struct {
    /* Common parameters */
    OSSL_LIB_CTX *libctx;
    NGCC_KEY *key;
    int operation;
    int nid;
    NGCC_SCHEME *scheme;
    unsigned char *message;
    size_t message_len;
    /* Keep the original sign/verify callbacks; sizes come from the descriptor. */
    ngcc_sign ngcc_sig_sign;
    ngcc_verify ngcc_sig_verify;
} NGCC_SIG_CTX;

static OSSL_FUNC_signature_freectx_fn               ngcc_sig_freectx;
static OSSL_FUNC_signature_dupctx_fn                ngcc_sig_dupctx;
static OSSL_FUNC_signature_gettable_ctx_params_fn   ngcc_sig_gettable_ctx_params;
static OSSL_FUNC_signature_get_ctx_params_fn        ngcc_sig_get_ctx_params;
/* Original one-shot entry points; streaming hooks are added below. */
static OSSL_FUNC_signature_sign_init_fn             ngcc_sig_sign_init;
static OSSL_FUNC_signature_sign_fn                  ngcc_sig_sign;
static OSSL_FUNC_signature_digest_sign_init_fn      ngcc_sig_digest_sign_init;
static OSSL_FUNC_signature_digest_sign_fn           ngcc_sig_digest_sign;
static OSSL_FUNC_signature_verify_init_fn           ngcc_sig_verify_init;
static OSSL_FUNC_signature_verify_fn                ngcc_sig_verify;
static OSSL_FUNC_signature_digest_verify_init_fn    ngcc_sig_digest_verify_init;
static OSSL_FUNC_signature_digest_verify_fn         ngcc_sig_digest_verify;

static void ngcc_sig_freectx(void *vctx)
{
    NGCC_SIG_CTX *ctx = (NGCC_SIG_CTX *)vctx;

    if (ctx != NULL) {
        ngcc_keymgmt_free(ctx->key);
        OPENSSL_clear_free(ctx->message, ctx->message_len);
    }
    OPENSSL_free(ctx);
}

static void *ngcc_sig_dupctx(void *vctx)
{
    NGCC_SIG_CTX *src = (NGCC_SIG_CTX *)vctx;
    NGCC_SIG_CTX *dst;

    if (src == NULL)
        return NULL;
    dst = OPENSSL_zalloc(sizeof(*dst));
    if (dst == NULL)
        return NULL;
    *dst = *src;
    dst->key = NULL;
    dst->message = NULL;
    if (src->key != NULL) {
        if (!ngcc_key_up_ref(src->key))
            goto err;
        dst->key = src->key;
    }
    if (src->message_len != 0
        && (dst->message = OPENSSL_memdup(src->message, src->message_len)) == NULL)
        goto err;
    return dst;
err:
    ngcc_sig_freectx(dst);
    return NULL;
}

static const OSSL_PARAM ngcc_sig_known_gettable_ctx_params[] = {
    /* DER encoded ASN.1 AlgorithmIdentifier, including OID and parameters */
    OSSL_PARAM_octet_string(OSSL_SIGNATURE_PARAM_ALGORITHM_ID, NULL, 0),
    OSSL_PARAM_END
};

static const OSSL_PARAM *ngcc_sig_gettable_ctx_params(void *vctx, void *provctx)
{
    (void)vctx;
    (void)provctx;
    return ngcc_sig_known_gettable_ctx_params;
}

static int ngcc_sig_get_ctx_params(void *vctx, OSSL_PARAM params[])
{
    OSSL_PARAM *p;
    NGCC_SIG_CTX *ctx = (NGCC_SIG_CTX *)vctx;

    if (ctx == NULL)
        return 0;

    p = OSSL_PARAM_locate(params, OSSL_SIGNATURE_PARAM_ALGORITHM_ID);
    if (p != NULL) {
        X509_ALGOR *algo = NULL;
        unsigned char *algo_der = NULL;
        int algo_der_len = 0;

        /* Use the same registered OID as the public/private key codecs. */
        if (ctx->nid == NID_undef)
            return 0;
        if ((algo = X509_ALGOR_new()) == NULL)
            return 0;

        if (X509_ALGOR_set0(algo, OBJ_nid2obj(ctx->nid),
                           V_ASN1_UNDEF, NULL) != 1) {
            X509_ALGOR_free(algo);
            return 0;
        }

        algo_der_len = i2d_X509_ALGOR(algo, &algo_der);
        X509_ALGOR_free(algo);

        if (algo_der_len <= 0 || algo_der == NULL)
            return 0;

        if (OSSL_PARAM_set_octet_string(p, algo_der, (size_t)algo_der_len) != 1) {
            OPENSSL_free(algo_der);
            return 0;
        }
        OPENSSL_free(algo_der);
    }

    return 1;
}

static int ngcc_sig_sign_init(void *vctx, void *vkey, const OSSL_PARAM params[])
{
    NGCC_SIG_CTX *ctx = (NGCC_SIG_CTX *)vctx;
    NGCC_KEY *key = (NGCC_KEY *)vkey;

    (void)params;
    if (ctx == NULL || vkey == NULL || key->scheme != ctx->scheme)
        return 0;

    if (key->sk == NULL || key->scheme->sk_len == 0)
        return 0;

    if (!ngcc_key_up_ref(key))
        return 0;
    ngcc_keymgmt_free(ctx->key);
    ctx->key = key;
    OPENSSL_clear_free(ctx->message, ctx->message_len);
    ctx->message = NULL;
    ctx->message_len = 0;
    ctx->operation = EVP_PKEY_OP_SIGN;

    return 1;
}

static int ngcc_sig_sign(void *vctx, unsigned char *sig,
                        size_t *sig_len, size_t sig_size,
                        const unsigned char *m, size_t m_len)
{
    static const unsigned char empty = 0;
    NGCC_SIG_CTX *ctx = (NGCC_SIG_CTX *)vctx;
    size_t sn_len = 0;
    unsigned long long sn_len_ull;
    int ret;

    if (ctx == NULL || ctx->key == NULL || ctx->key->sk == NULL || sig_len == NULL
        || (m == NULL && m_len != 0))
        return 0;
    if (m == NULL)
        m = &empty;
    sn_len = ctx->scheme->output_len;

    /* Return sig length when sig == NULL */
    if (sig == NULL) {
        *sig_len = sn_len;
        return 1;
    }

    if (sig_size < sn_len) {
        ERR_raise(ERR_LIB_PROV, PROV_R_OUTPUT_BUFFER_TOO_SMALL);
        return 0;
    }

    sn_len_ull = sn_len;
    pthread_mutex_lock(ctx->scheme->lock);
    if (!ngcc_random_seed(ctx->scheme, ctx->libctx)) {
        pthread_mutex_unlock(ctx->scheme->lock);
        return 0;
    }
    ret = ctx->ngcc_sig_sign(ctx->key->sk, (unsigned long long)ctx->scheme->sk_len,
                            (unsigned char *)m, (unsigned long long)m_len,
                            sig, &sn_len_ull);
    pthread_mutex_unlock(ctx->scheme->lock);
    if (ret == 0 && sn_len_ull <= sn_len && sn_len_ull <= sig_size) {
        *sig_len = (size_t)sn_len_ull;
        return 1;
    }
    return 0;
}

static int ngcc_sig_digest_sign_init(void *vctx, const char *md_name,
                                    void *vkey, const OSSL_PARAM params[])
{
    if (md_name != NULL && md_name[0] != '\0'
        && OPENSSL_strcasecmp(md_name, "UNDEF") != 0)
        return 0;
    return ngcc_sig_sign_init(vctx, vkey, params);
}

static int ngcc_sig_digest_sign(void *vctx, unsigned char *sig,
                               size_t *sig_len, size_t sig_size,
                               const unsigned char *m, size_t m_len)
{
    return ngcc_sig_sign(vctx, sig, sig_len, sig_size, m, m_len);
}

static int ngcc_sig_verify_init(void *vctx, void *vkey, const OSSL_PARAM params[])
{
    NGCC_SIG_CTX *ctx = (NGCC_SIG_CTX *)vctx;
    NGCC_KEY *key = (NGCC_KEY *)vkey;

    (void)params;
    if (ctx == NULL || vkey == NULL || key->scheme != ctx->scheme)
        return 0;

    if (key->pk == NULL || key->scheme->pk_len == 0)
        return 0;

    if (!ngcc_key_up_ref(key))
        return 0;
    ngcc_keymgmt_free(ctx->key);
    ctx->key = key;
    OPENSSL_clear_free(ctx->message, ctx->message_len);
    ctx->message = NULL;
    ctx->message_len = 0;
    ctx->operation = EVP_PKEY_OP_VERIFY;

    return 1;
}

static int ngcc_sig_verify(void *vctx,
                          const unsigned char *sig, size_t sig_len,
                          const unsigned char *m, size_t m_len)
{
    static const unsigned char empty = 0;
    int ret = -1;
    NGCC_SIG_CTX *ctx = (NGCC_SIG_CTX *)vctx;

    if (ctx == NULL || ctx->key == NULL || ctx->key->pk == NULL || sig == NULL
        || sig_len > ctx->scheme->output_len
        || (m == NULL && m_len != 0))
        return 0;
    if (m == NULL)
        m = &empty;
    pthread_mutex_lock(ctx->scheme->lock);
    ret = ctx->ngcc_sig_verify(ctx->key->pk, (unsigned long long)ctx->scheme->pk_len,
                              (unsigned char *)sig, (unsigned long long)sig_len,
                              (unsigned char *)m, (unsigned long long)m_len);
    pthread_mutex_unlock(ctx->scheme->lock);
    if (ret == 0)
        return 1;
    else
        return 0;
}

static int ngcc_sig_digest_verify_init(void *vctx, const char *md_name,
                                      void *vkey, const OSSL_PARAM params[])
{
    if (md_name != NULL && md_name[0] != '\0'
        && OPENSSL_strcasecmp(md_name, "UNDEF") != 0)
        return 0;
    return ngcc_sig_verify_init(vctx, vkey, params);
}

static int ngcc_sig_digest_verify(void *vctx,
                                 const unsigned char *sig, size_t sig_len,
                                 const unsigned char *m, size_t m_len)
{
    return ngcc_sig_verify(vctx, sig, sig_len, m, m_len);
}

/* Buffer message fragments; proposal signing and its internal hash are unchanged. */
static int ngcc_sig_digest_update(void *vctx, const unsigned char *data, size_t len)
{
    NGCC_SIG_CTX *ctx = vctx;
    unsigned char *copy;

    if (ctx == NULL || (data == NULL && len != 0)
        || len > SIZE_MAX - ctx->message_len)
        return 0;
    if (len == 0)
        return 1;
    copy = OPENSSL_realloc(ctx->message, ctx->message_len + len);
    if (copy == NULL)
        return 0;
    memcpy(copy + ctx->message_len, data, len);
    ctx->message = copy;
    ctx->message_len += len;
    return 1;
}

static int ngcc_sig_digest_sign_final(void *vctx, unsigned char *sig,
                                     size_t *len, size_t size)
{
    NGCC_SIG_CTX *ctx = vctx;

    return ctx != NULL
        && ngcc_sig_sign(ctx, sig, len, size, ctx->message, ctx->message_len);
}

static int ngcc_sig_digest_verify_final(void *vctx, const unsigned char *sig,
                                       size_t len)
{
    NGCC_SIG_CTX *ctx = vctx;

    return ctx != NULL
        && ngcc_sig_verify(ctx, sig, len, ctx->message, ctx->message_len);
}

/* Original NGCC_SIG_ALG_IMPL layout, wired to the frozen author descriptors. */
#define NGCC_SIG_ALG_IMPL(tag, getter, names, primary)                                    \
static void *ngcc_sig_##tag##_newctx(void *provctx, const char *propq)                    \
{                                                                                         \
    NGCC_PROV_CTX *provider = provctx;                                                    \
    NGCC_SIG_CTX *ctx;                                                                    \
    (void)propq;                                                                          \
    if (provider == NULL || (ctx = OPENSSL_zalloc(sizeof(*ctx))) == NULL)                 \
        return NULL;                                                                      \
    ctx->libctx = provider->libctx;                                                       \
    ctx->scheme = getter();                                                               \
    ctx->nid = ngcc_sig_nid(primary);                                                     \
    ctx->ngcc_sig_sign = ctx->scheme->sign;                                               \
    ctx->ngcc_sig_verify = ctx->scheme->verify;                                           \
    return ctx;                                                                           \
}                                                                                         \
const OSSL_DISPATCH ngcc_##tag##_functions[] = {                                          \
    NGCC_DISPATCH(OSSL_FUNC_SIGNATURE_NEWCTX, ngcc_sig_##tag##_newctx),                   \
    NGCC_DISPATCH(OSSL_FUNC_SIGNATURE_FREECTX, ngcc_sig_freectx),                         \
    NGCC_DISPATCH(OSSL_FUNC_SIGNATURE_DUPCTX, ngcc_sig_dupctx),                           \
    NGCC_DISPATCH(OSSL_FUNC_SIGNATURE_SIGN_INIT, ngcc_sig_sign_init),                     \
    NGCC_DISPATCH(OSSL_FUNC_SIGNATURE_SIGN, ngcc_sig_sign),                               \
    NGCC_DISPATCH(OSSL_FUNC_SIGNATURE_DIGEST_SIGN_INIT, ngcc_sig_digest_sign_init),       \
    NGCC_DISPATCH(OSSL_FUNC_SIGNATURE_DIGEST_SIGN, ngcc_sig_digest_sign),                 \
    NGCC_DISPATCH(OSSL_FUNC_SIGNATURE_VERIFY_INIT, ngcc_sig_verify_init),                 \
    NGCC_DISPATCH(OSSL_FUNC_SIGNATURE_VERIFY, ngcc_sig_verify),                           \
    NGCC_DISPATCH(OSSL_FUNC_SIGNATURE_DIGEST_VERIFY_INIT, ngcc_sig_digest_verify_init),   \
    NGCC_DISPATCH(OSSL_FUNC_SIGNATURE_DIGEST_VERIFY, ngcc_sig_digest_verify),             \
    NGCC_DISPATCH(OSSL_FUNC_SIGNATURE_GETTABLE_CTX_PARAMS, ngcc_sig_gettable_ctx_params), \
    NGCC_DISPATCH(OSSL_FUNC_SIGNATURE_GET_CTX_PARAMS, ngcc_sig_get_ctx_params),           \
    NGCC_DISPATCH(OSSL_FUNC_SIGNATURE_DIGEST_SIGN_UPDATE, ngcc_sig_digest_update),        \
    NGCC_DISPATCH(OSSL_FUNC_SIGNATURE_DIGEST_SIGN_FINAL, ngcc_sig_digest_sign_final),     \
    NGCC_DISPATCH(OSSL_FUNC_SIGNATURE_DIGEST_VERIFY_UPDATE, ngcc_sig_digest_update),      \
    NGCC_DISPATCH(OSSL_FUNC_SIGNATURE_DIGEST_VERIFY_FINAL, ngcc_sig_digest_verify_final), \
    { 0, NULL }                                                                           \
};

#define NGCC_SIG NGCC_SIG_ALG_IMPL
#include "ngcc_sig_algorithms.inc"
#undef NGCC_SIG
#undef NGCC_SIG_ALG_IMPL
