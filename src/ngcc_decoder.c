/*
 * Copyright 2026 The Tongsuo Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <string.h>
#include <openssl/asn1.h>
#include <openssl/asn1t.h>
#include <openssl/bio.h>
#include <openssl/core.h>
#include <openssl/core_dispatch.h>
#include <openssl/core_names.h>
#include <openssl/core_object.h>
#include <openssl/params.h>
#include <openssl/param_build.h>
#include <openssl/x509.h>

#include "ngcc_prov.h"

/* Decoder ASN.1 template for SubjectPublicKeyInfo */
typedef struct {
    X509_ALGOR *algor;
    ASN1_BIT_STRING *pk;
} NGCC_SIG_SPKI;

ASN1_SEQUENCE(NGCC_SIG_SPKI) = {
    ASN1_SIMPLE(NGCC_SIG_SPKI, algor, X509_ALGOR),
    ASN1_SIMPLE(NGCC_SIG_SPKI, pk, ASN1_BIT_STRING)
} ASN1_SEQUENCE_END(NGCC_SIG_SPKI)

/* Decoder context */
typedef struct {
    OSSL_LIB_CTX *libctx;
    int nid;
    NGCC_SCHEME *scheme;
} NGCC_SIG_DECODER_CTX;

static OSSL_FUNC_decoder_freectx_fn             ngcc_sig_decoder_freectx;
static OSSL_FUNC_decoder_does_selection_fn      ngcc_sig_decoder_spki_der_does_selection;
static OSSL_FUNC_decoder_does_selection_fn      ngcc_sig_decoder_pkcs8_der_does_selection;
static OSSL_FUNC_decoder_decode_fn              ngcc_sig_decoder_spki_der_decode;
static OSSL_FUNC_decoder_decode_fn              ngcc_sig_decoder_pkcs8_der_decode;

static void ngcc_sig_decoder_freectx(void *vctx)
{
    OPENSSL_free(vctx);
}

/* SubjectPublicKeyInfo DER decoder */
static int ngcc_sig_decoder_spki_der_does_selection(void *vctx, int selection)
{
    (void)vctx;
    if (selection == 0)
        return 1;
    return (selection & OSSL_KEYMGMT_SELECT_PUBLIC_KEY);
}

/* PKCS#8 DER decoder */
static int ngcc_sig_decoder_pkcs8_der_does_selection(void *vctx, int selection)
{
    (void)vctx;
    if (selection == 0)
        return 1;
    return (selection & OSSL_KEYMGMT_SELECT_PRIVATE_KEY);
}

static int ngcc_sig_decoder_spki_der_decode(void *vctx, OSSL_CORE_BIO *cin,
                                           int selection,
                                           OSSL_CALLBACK *data_cb,
                                           void *data_cbarg,
                                           OSSL_PASSPHRASE_CALLBACK *pw_cb,
                                           void *pw_cbarg)
{
    int ret = 1;
    BIO *in = NULL;
    NGCC_SIG_SPKI *spki = NULL;
    const ASN1_OBJECT *alg_obj = NULL;
    NGCC_KEY *key = NULL;
    OSSL_PARAM_BLD *bld = NULL;
    OSSL_PARAM *params = NULL;

    NGCC_SIG_DECODER_CTX *ctx = (NGCC_SIG_DECODER_CTX *)vctx;

    (void)selection;
    (void)pw_cb;
    (void)pw_cbarg;
    if (ctx == NULL || data_cb == NULL)
        return 0;

    if ((in = BIO_new_from_core_bio(ctx->libctx, cin)) == NULL)
        return 0;

    if ((spki = (NGCC_SIG_SPKI *)ASN1_item_d2i_bio_ex(ASN1_ITEM_rptr(NGCC_SIG_SPKI),
                                                     in, NULL, ctx->libctx, NULL)) == NULL)
        goto end;

    X509_ALGOR_get0(&alg_obj, NULL, NULL, spki->algor);
    if (alg_obj == NULL || OBJ_obj2nid(alg_obj) != ctx->nid)
        goto end;

    ret = 0;
    if ((key = ngcc_key_new(ctx->scheme)) == NULL)
        goto end;

    if (spki->pk == NULL
        || (size_t)spki->pk->length != ctx->scheme->pk_len
        || (spki->pk->flags & 7) != 0)
        goto end;

    if ((key->pk = OPENSSL_memdup(spki->pk->data, (size_t)spki->pk->length)) == NULL)
        goto end;

    if ((bld = OSSL_PARAM_BLD_new()) == NULL)
        goto end;

    if (OSSL_PARAM_BLD_push_int(bld, OSSL_OBJECT_PARAM_TYPE, OSSL_OBJECT_PKEY) != 1)
        goto end;

    if (OSSL_PARAM_BLD_push_utf8_string(bld, OSSL_OBJECT_PARAM_DATA_TYPE, OBJ_nid2sn(ctx->nid), 0) != 1)
        goto end;

    if (OSSL_PARAM_BLD_push_octet_string(bld, OSSL_OBJECT_PARAM_REFERENCE, &key, sizeof(key)) != 1)
        goto end;

    if ((params = OSSL_PARAM_BLD_to_param(bld)) == NULL)
        goto end;

    ret = data_cb(params, data_cbarg);
    {
        const OSSL_PARAM *reference = OSSL_PARAM_locate_const(params, OSSL_OBJECT_PARAM_REFERENCE);
        memcpy(&key, reference->data, sizeof(key));
    }
end:
    if (spki != NULL)
        ASN1_item_free((ASN1_VALUE *)spki, ASN1_ITEM_rptr(NGCC_SIG_SPKI));
    ngcc_keymgmt_free(key);
    BIO_free(in);
    OSSL_PARAM_BLD_free(bld);
    OSSL_PARAM_free(params);
    return ret;
}

static int ngcc_sig_decoder_pkcs8_der_decode(void *vctx, OSSL_CORE_BIO *cin,
                                            int selection,
                                            OSSL_CALLBACK *data_cb,
                                            void *data_cbarg,
                                            OSSL_PASSPHRASE_CALLBACK *pw_cb,
                                            void *pw_cbarg)
{
    int ret = 1;
    BIO *in = NULL;
    PKCS8_PRIV_KEY_INFO *p8_info = NULL;
    const unsigned char *sk = NULL;
    int sk_len = 0;
    NGCC_KEY *key = NULL;
    const ASN1_OBJECT *alg_obj = NULL;
    OSSL_PARAM_BLD *bld = NULL;
    OSSL_PARAM *params = NULL;

    NGCC_SIG_DECODER_CTX *ctx = (NGCC_SIG_DECODER_CTX *)vctx;

    (void)selection;
    (void)pw_cb;
    (void)pw_cbarg;
    if (ctx == NULL || data_cb == NULL)
        return 0;

    if ((in = BIO_new_from_core_bio(ctx->libctx, cin)) == NULL)
        return 0;

    if ((p8_info = d2i_PKCS8_PRIV_KEY_INFO_bio(in, NULL)) == NULL)
        goto end;

    if (PKCS8_pkey_get0(&alg_obj, &sk, &sk_len, NULL, p8_info) != 1)
        goto end;

    if (OBJ_obj2nid(alg_obj) != ctx->nid)
        goto end;

    ret = 0;
    if ((key = ngcc_key_new(ctx->scheme)) == NULL)
        goto end;

    if (sk_len < 0 || ((size_t)sk_len != ctx->scheme->sk_len
                      && (size_t)sk_len != ctx->scheme->sk_len + ctx->scheme->pk_len))
        goto end;
    key->sk = OPENSSL_memdup(sk, ctx->scheme->sk_len);
    if (key->sk == NULL)
        goto end;
    if ((size_t)sk_len > ctx->scheme->sk_len) {
        key->pk = OPENSSL_memdup(sk + ctx->scheme->sk_len, ctx->scheme->pk_len);
        if (key->pk == NULL)
            goto end;
    }

    if ((bld = OSSL_PARAM_BLD_new()) == NULL)
        goto end;

    if (OSSL_PARAM_BLD_push_int(bld, OSSL_OBJECT_PARAM_TYPE, OSSL_OBJECT_PKEY) != 1)
        goto end;

    if (OSSL_PARAM_BLD_push_utf8_string(bld, OSSL_OBJECT_PARAM_DATA_TYPE, OBJ_nid2sn(ctx->nid), 0) != 1)
        goto end;

    if (OSSL_PARAM_BLD_push_octet_string(bld, OSSL_OBJECT_PARAM_REFERENCE, &key, sizeof(key)) != 1)
        goto end;

    if ((params = OSSL_PARAM_BLD_to_param(bld)) == NULL)
        goto end;

    ret = data_cb(params, data_cbarg);
    {
        const OSSL_PARAM *reference = OSSL_PARAM_locate_const(params, OSSL_OBJECT_PARAM_REFERENCE);
        memcpy(&key, reference->data, sizeof(key));
    }

end:
    ngcc_keymgmt_free(key);
    BIO_free(in);
    OSSL_PARAM_BLD_free(bld);
    OSSL_PARAM_free(params);
    PKCS8_PRIV_KEY_INFO_free(p8_info);
    return ret;
}

/* KEYMGMT_LOAD consumes the key reference on a successful callback. */
#define NGCC_SIG_DECODER_ALG_IMPL(tag, getter, names, primary)                                  \
static void *ngcc_sig_##tag##_decoder_newctx(void *vprovctx)                                    \
{                                                                                               \
    NGCC_PROV_CTX *provctx = vprovctx;                                                          \
    NGCC_SIG_DECODER_CTX *ctx;                                                                  \
    if (provctx == NULL || (ctx = OPENSSL_zalloc(sizeof(*ctx))) == NULL)                        \
        return NULL;                                                                            \
    ctx->libctx = provctx->libctx;                                                              \
    ctx->nid = ngcc_sig_nid(primary);                                                           \
    ctx->scheme = getter();                                                                     \
    return ctx;                                                                                 \
}                                                                                               \
const OSSL_DISPATCH ngcc_sig_##tag##_decoder_spki_der_functions[] = {                           \
    NGCC_DISPATCH(OSSL_FUNC_DECODER_NEWCTX, ngcc_sig_##tag##_decoder_newctx),                   \
    NGCC_DISPATCH(OSSL_FUNC_DECODER_FREECTX, ngcc_sig_decoder_freectx),                         \
    NGCC_DISPATCH(OSSL_FUNC_DECODER_DOES_SELECTION, ngcc_sig_decoder_spki_der_does_selection),  \
    NGCC_DISPATCH(OSSL_FUNC_DECODER_DECODE, ngcc_sig_decoder_spki_der_decode),                  \
    { 0, NULL }                                                                                 \
};                                                                                              \
const OSSL_DISPATCH ngcc_sig_##tag##_decoder_pkcs8_der_functions[] = {                          \
    NGCC_DISPATCH(OSSL_FUNC_DECODER_NEWCTX, ngcc_sig_##tag##_decoder_newctx),                   \
    NGCC_DISPATCH(OSSL_FUNC_DECODER_FREECTX, ngcc_sig_decoder_freectx),                         \
    NGCC_DISPATCH(OSSL_FUNC_DECODER_DOES_SELECTION, ngcc_sig_decoder_pkcs8_der_does_selection), \
    NGCC_DISPATCH(OSSL_FUNC_DECODER_DECODE, ngcc_sig_decoder_pkcs8_der_decode),                 \
    { 0, NULL }                                                                                 \
};

#define NGCC_SIG NGCC_SIG_DECODER_ALG_IMPL
#include "ngcc_sig_algorithms.inc"
#undef NGCC_SIG
#undef NGCC_SIG_DECODER_ALG_IMPL
