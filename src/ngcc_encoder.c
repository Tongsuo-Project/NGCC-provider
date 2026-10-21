/*
 * Copyright 2026 The Tongsuo Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <limits.h>
#include <string.h>
#include <openssl/bio.h>
#include <openssl/core.h>
#include <openssl/core_dispatch.h>
#include <openssl/core_names.h>
#include <openssl/objects.h>
#include <openssl/params.h>
#include <openssl/param_build.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

#include "ngcc_prov.h"

typedef struct {
    OSSL_LIB_CTX *libctx;
    int nid;
    int pem;
} NGCC_SIG_ENCODER_CTX;

/* default encoder context new function, without nid */
static OSSL_FUNC_encoder_newctx_fn              ngcc_sig_encoder_newctx;
static OSSL_FUNC_encoder_freectx_fn             ngcc_sig_encoder_freectx;
static OSSL_FUNC_encoder_does_selection_fn      ngcc_sig_encoder_spki_does_selection;
static OSSL_FUNC_encoder_does_selection_fn      ngcc_sig_encoder_pkcs8_does_selection;
/* public key text encoder function */
static OSSL_FUNC_encoder_encode_fn              ngcc_sig_encoder_text_encode;
/* SubjectPublicKeyInfo encoder */
static OSSL_FUNC_encoder_encode_fn              ngcc_sig_encoder_spki_der_encode;
/* PKCS#8 (private key) encoder */
static OSSL_FUNC_encoder_encode_fn              ngcc_sig_encoder_pkcs8_der_encode;

static void *ngcc_sig_encoder_newctx(void *vprovctx)
{
    NGCC_PROV_CTX *prov_ctx = (NGCC_PROV_CTX *)vprovctx;
    NGCC_SIG_ENCODER_CTX *ctx = NULL;

    if (prov_ctx == NULL)
        return NULL;

    if ((ctx = OPENSSL_zalloc(sizeof(*ctx))) == NULL)
        return NULL;

    ctx->libctx = prov_ctx->libctx;

    return ctx;
}

static void ngcc_sig_encoder_freectx(void *vctx)
{
    OPENSSL_free(vctx);
}

static int ngcc_sig_encoder_spki_does_selection(void *vctx, int selection)
{
    (void)vctx;
    if (selection == 0)
        return 1;
    return (selection & OSSL_KEYMGMT_SELECT_PUBLIC_KEY);
}

static int ngcc_sig_encoder_pkcs8_does_selection(void *vctx, int selection)
{
    (void)vctx;
    if (selection == 0)
        return 1;
    return (selection & OSSL_KEYMGMT_SELECT_PRIVATE_KEY);
}

static int ngcc_sig_encoder_text_encode(void *vctx, OSSL_CORE_BIO *cout,
                                       const void *obj_raw,
                                       const OSSL_PARAM obj_abstract[],
                                       int selection,
                                       OSSL_PASSPHRASE_CALLBACK *cb,
                                       void *cbarg)
{
    int ret = 0;
    size_t i;
    BIO *out = NULL;
    const NGCC_KEY *key = (const NGCC_KEY *)obj_raw;
    NGCC_SIG_ENCODER_CTX *ctx = (NGCC_SIG_ENCODER_CTX *)vctx;

    (void)cb;
    (void)cbarg;
    (void)selection;
    if (ctx == NULL || key == NULL || key->pk == NULL || obj_abstract != NULL)
        goto end;

    if ((out = BIO_new_from_core_bio(ctx->libctx, cout)) == NULL)
        goto end;

    if ((selection & OSSL_KEYMGMT_SELECT_PUBLIC_KEY) == 0)
        goto end;

    for (i = 0; i < key->scheme->pk_len; i++) {
        BIO_printf(out, "%02x", key->pk[i]);
        if (i % 16 == 15 || i == key->scheme->pk_len - 1)
            BIO_printf(out, "\n");
        else
            BIO_printf(out, ":");
    }

    ret = 1;
end:
    BIO_free(out);
    return ret;
}

static int ngcc_sig_encoder_spki_der_encode(void *vctx, OSSL_CORE_BIO *cout,
                                           const void *obj_raw,
                                           const OSSL_PARAM obj_abstract[],
                                           int selection,
                                           OSSL_PASSPHRASE_CALLBACK *cb,
                                           void *cbarg)
{
    int ret = 0;
    BIO *out = NULL;
    X509_PUBKEY *x509_pk = NULL;
    unsigned char *pk_copy = NULL;
    const NGCC_KEY *key = (const NGCC_KEY *)obj_raw;
    NGCC_SIG_ENCODER_CTX *ctx = (NGCC_SIG_ENCODER_CTX *)vctx;

    (void)cb;
    (void)cbarg;
    (void)selection;
    if (ctx == NULL || key == NULL || key->pk == NULL || obj_abstract != NULL)
        return 0;

    if ((x509_pk = X509_PUBKEY_new()) == NULL)
        goto end;

    /* This step is necessary since X509_PUBKEY_set0_param does not "copy" pk */
    if ((pk_copy = OPENSSL_memdup(key->pk, key->scheme->pk_len)) == NULL)
        goto end;

    if (X509_PUBKEY_set0_param(x509_pk, OBJ_nid2obj(ctx->nid),
                               V_ASN1_UNDEF, NULL, pk_copy, (int)key->scheme->pk_len) != 1) {
        OPENSSL_free(pk_copy);
        goto end;
    }

    if ((out = BIO_new_from_core_bio(ctx->libctx, cout)) == NULL)
        goto end;

    if ((ctx->pem ? PEM_write_bio_X509_PUBKEY(out, x509_pk)
                  : i2d_X509_PUBKEY_bio(out, x509_pk)) != 1)
        goto end;

    ret = 1;
end:
    X509_PUBKEY_free(x509_pk);
    BIO_free(out);
    return ret;
}

static int ngcc_sig_encoder_pkcs8_der_encode(void *vctx, OSSL_CORE_BIO *cout,
                                            const void *obj_raw,
                                            const OSSL_PARAM obj_abstract[],
                                            int selection,
                                            OSSL_PASSPHRASE_CALLBACK *cb,
                                            void *cbarg)
{
    int ret = 0;
    BIO *out = NULL;
    PKCS8_PRIV_KEY_INFO *p8_info = NULL;
    unsigned char *sk_copy = NULL;
    size_t encoded_len;
    const NGCC_KEY *key = (const NGCC_KEY *)obj_raw;
    NGCC_SIG_ENCODER_CTX *ctx = (NGCC_SIG_ENCODER_CTX *)vctx;

    (void)cb;
    (void)cbarg;
    (void)selection;
    if (ctx == NULL || key == NULL || key->sk == NULL || obj_abstract != NULL)
        return 0;

    if ((p8_info = PKCS8_PRIV_KEY_INFO_new()) == NULL)
        goto end;

    /* Keep the original PKCS#8 envelope. Append the public key so reloads
     * can match a certificate without deriving it inside author code. */
    encoded_len = key->scheme->sk_len;
    if (key->pk != NULL)
        encoded_len += key->scheme->pk_len;
    if (encoded_len > INT_MAX || (sk_copy = OPENSSL_malloc(encoded_len)) == NULL)
        goto end;
    memcpy(sk_copy, key->sk, key->scheme->sk_len);
    if (key->pk != NULL)
        memcpy(sk_copy + key->scheme->sk_len, key->pk, key->scheme->pk_len);

    if (PKCS8_pkey_set0(p8_info, OBJ_nid2obj(ctx->nid), 0,
                        V_ASN1_UNDEF, NULL, sk_copy, (int)encoded_len) != 1) {
        OPENSSL_clear_free(sk_copy, encoded_len);
        goto end;
    }

    if ((out = BIO_new_from_core_bio(ctx->libctx, cout)) == NULL)
        goto end;

    if ((ctx->pem ? PEM_write_bio_PKCS8_PRIV_KEY_INFO(out, p8_info)
                  : i2d_PKCS8_PRIV_KEY_INFO_bio(out, p8_info)) != 1)
        goto end;

    ret = 1;
end:
    PKCS8_PRIV_KEY_INFO_free(p8_info);
    BIO_free(out);
    return ret;
}

/* Original per-algorithm encoder constructors, now using the ref registry. */
#define NGCC_SIG_ENCODER_ALG_IMPL(tag, getter, names, primary)                              \
static void *ngcc_sig_##tag##_encoder_newctx(void *provctx)                                 \
{                                                                                           \
    NGCC_SIG_ENCODER_CTX *ctx = ngcc_sig_encoder_newctx(provctx);                           \
    if (ctx != NULL)                                                                        \
        ctx->nid = ngcc_sig_nid(primary);                                                   \
    return ctx;                                                                             \
}                                                                                           \
static void *ngcc_sig_##tag##_encoder_pem_newctx(void *provctx)                             \
{                                                                                           \
    NGCC_SIG_ENCODER_CTX *ctx = ngcc_sig_##tag##_encoder_newctx(provctx);                   \
    if (ctx != NULL)                                                                        \
        ctx->pem = 1;                                                                       \
    return ctx;                                                                             \
}                                                                                           \
const OSSL_DISPATCH ngcc_sig_##tag##_encoder_text_functions[] = {                           \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_NEWCTX, ngcc_sig_##tag##_encoder_newctx),               \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_FREECTX, ngcc_sig_encoder_freectx),                     \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_DOES_SELECTION, ngcc_sig_encoder_spki_does_selection),  \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_ENCODE, ngcc_sig_encoder_text_encode),                  \
    { 0, NULL }                                                                             \
};                                                                                          \
const OSSL_DISPATCH ngcc_sig_##tag##_encoder_spki_der_functions[] = {                       \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_NEWCTX, ngcc_sig_##tag##_encoder_newctx),               \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_FREECTX, ngcc_sig_encoder_freectx),                     \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_DOES_SELECTION, ngcc_sig_encoder_spki_does_selection),  \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_ENCODE, ngcc_sig_encoder_spki_der_encode),              \
    { 0, NULL }                                                                             \
};                                                                                          \
const OSSL_DISPATCH ngcc_sig_##tag##_encoder_pkcs8_der_functions[] = {                      \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_NEWCTX, ngcc_sig_##tag##_encoder_newctx),               \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_FREECTX, ngcc_sig_encoder_freectx),                     \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_DOES_SELECTION, ngcc_sig_encoder_pkcs8_does_selection), \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_ENCODE, ngcc_sig_encoder_pkcs8_der_encode),             \
    { 0, NULL }                                                                             \
};                                                                                          \
const OSSL_DISPATCH ngcc_sig_##tag##_encoder_spki_pem_functions[] = {                       \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_NEWCTX, ngcc_sig_##tag##_encoder_pem_newctx),           \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_FREECTX, ngcc_sig_encoder_freectx),                     \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_DOES_SELECTION, ngcc_sig_encoder_spki_does_selection),  \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_ENCODE, ngcc_sig_encoder_spki_der_encode),              \
    { 0, NULL }                                                                             \
};                                                                                          \
const OSSL_DISPATCH ngcc_sig_##tag##_encoder_pkcs8_pem_functions[] = {                      \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_NEWCTX, ngcc_sig_##tag##_encoder_pem_newctx),           \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_FREECTX, ngcc_sig_encoder_freectx),                     \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_DOES_SELECTION, ngcc_sig_encoder_pkcs8_does_selection), \
    NGCC_DISPATCH(OSSL_FUNC_ENCODER_ENCODE, ngcc_sig_encoder_pkcs8_der_encode),             \
    { 0, NULL }                                                                             \
};

#define NGCC_SIG NGCC_SIG_ENCODER_ALG_IMPL
#include "ngcc_sig_algorithms.inc"
#undef NGCC_SIG
#undef NGCC_SIG_ENCODER_ALG_IMPL
