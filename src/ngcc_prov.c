/*
 * Copyright 2026 The Tongsuo Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <openssl/core.h>
#include <openssl/core_dispatch.h>
#include <openssl/core_names.h>
#include <openssl/objects.h>
#include <string.h>

#include "ngcc_prov.h"
#include "ngcc_capabilities.h"
#include "ngcc_oid.h"

/* Signature OIDs are needed for X.509 and key codecs. KEM uses TLS group IDs. */
static struct {
    const char *algorithm;
    const char *short_name;
    const char *oid;
    int nid;
} ngcc_sig_oids[] = {
#define NGCC_SIG_OID(algorithm, short_name, oid)                                           \
    { algorithm, short_name, oid, NID_undef },
#include "ngcc_sig_oids.inc"
#undef NGCC_SIG_OID
    { NULL, NULL, NULL, NID_undef }
};
static CRYPTO_ONCE ngcc_oid_once = CRYPTO_ONCE_STATIC_INIT;
static int ngcc_oids_ready;

static void ngcc_register_sig_oids(void)
{
    size_t i;

    for (i = 0; ngcc_sig_oids[i].algorithm != NULL; ++i) {
        int nid = OBJ_txt2nid(ngcc_sig_oids[i].oid);

        if (nid == NID_undef)
            nid = OBJ_create(ngcc_sig_oids[i].oid,
                             ngcc_sig_oids[i].short_name,
                             ngcc_sig_oids[i].short_name);
        if (nid == NID_undef || !OBJ_add_sigid(nid, NID_undef, nid))
            return;
        ngcc_sig_oids[i].nid = nid;
    }
    ngcc_oids_ready = 1;
}

int ngcc_sig_nid(const char *algorithm)
{
    size_t i;

    for (i = 0; ngcc_sig_oids[i].algorithm != NULL; ++i)
        if (strcmp(algorithm, ngcc_sig_oids[i].algorithm) == 0)
            return ngcc_sig_oids[i].nid;
    return NID_undef;
}

/* Provider parameters */
static const OSSL_PARAM ngcc_param_types[] = {
    OSSL_PARAM_DEFN(OSSL_PROV_PARAM_NAME, OSSL_PARAM_UTF8_PTR, NULL, 0),
    OSSL_PARAM_DEFN(OSSL_PROV_PARAM_VERSION, OSSL_PARAM_UTF8_PTR, NULL, 0),
    OSSL_PARAM_DEFN(OSSL_PROV_PARAM_STATUS, OSSL_PARAM_INTEGER, NULL, 0),
    OSSL_PARAM_END
};

/* Algorithm implementations remain in their original modules. */
static const OSSL_ALGORITHM ngcc_kem_algs[] = {
#define NGCC_KEM(tag, getter, names, primary)                                              \
    { names, "provider=ngcc", ngcc_##tag##_functions, "NGCC author reference KEM" },
#include "ngcc_kem_algorithms.inc"
#undef NGCC_KEM
    { NULL, NULL, NULL, NULL }
};

static const OSSL_ALGORITHM ngcc_sig_algs[] = {
#define NGCC_SIG(tag, getter, names, primary)                                              \
    { names, "provider=ngcc", ngcc_##tag##_functions, "NGCC author reference sig" },
#include "ngcc_sig_algorithms.inc"
#undef NGCC_SIG
    { NULL, NULL, NULL, NULL }
};

static const OSSL_ALGORITHM ngcc_kmgmt_algs[] = {
#define NGCC_KEM(tag, getter, names, primary)                                              \
    { names, "provider=ngcc", ngcc_##tag##_kmgmt_functions, "NGCC author reference key" },
#include "ngcc_kem_algorithms.inc"
#define NGCC_SIG NGCC_KEM
#include "ngcc_sig_algorithms.inc"
#undef NGCC_SIG
#undef NGCC_KEM
    { NULL, NULL, NULL, NULL }
};

/* Keep the original SubjectPublicKeyInfo and PrivateKeyInfo codec paths. */
static const OSSL_ALGORITHM ngcc_encoder_algs[] = {
#define NGCC_SIG(tag, getter, names, primary)                                              \
    { names, "provider=ngcc,output=text", ngcc_sig_##tag##_encoder_text_functions, NULL }, \
    { names, "provider=ngcc,output=der,structure=SubjectPublicKeyInfo",                    \
      ngcc_sig_##tag##_encoder_spki_der_functions, NULL },                                 \
    { names, "provider=ngcc,output=der,structure=PrivateKeyInfo",                          \
      ngcc_sig_##tag##_encoder_pkcs8_der_functions, NULL },                                \
    { names, "provider=ngcc,output=pem,structure=SubjectPublicKeyInfo",                    \
      ngcc_sig_##tag##_encoder_spki_pem_functions, NULL },                                 \
    { names, "provider=ngcc,output=pem,structure=PrivateKeyInfo",                          \
      ngcc_sig_##tag##_encoder_pkcs8_pem_functions, NULL },
#include "ngcc_sig_algorithms.inc"
#undef NGCC_SIG
    { NULL, NULL, NULL, NULL }
};

static const OSSL_ALGORITHM ngcc_decoder_algs[] = {
#define NGCC_SIG(tag, getter, names, primary)                                              \
    { names, "provider=ngcc,input=der,structure=SubjectPublicKeyInfo",                     \
      ngcc_sig_##tag##_decoder_spki_der_functions, NULL },                                 \
    { names, "provider=ngcc,input=der,structure=PrivateKeyInfo",                           \
      ngcc_sig_##tag##_decoder_pkcs8_der_functions, NULL },
#include "ngcc_sig_algorithms.inc"
#undef NGCC_SIG
    { NULL, NULL, NULL, NULL }
};

static void ngcc_provider_teardown(void *provctx)
{
    NGCC_PROV_CTX *ctx = (NGCC_PROV_CTX *)provctx;

    if (ctx != NULL) {
        OSSL_LIB_CTX_free(ctx->libctx);
        OPENSSL_free(ctx);
    }
}

static const OSSL_PARAM *ngcc_provider_gettable_params(void *provctx)
{
    (void)provctx;
    return ngcc_param_types;
}

static int ngcc_provider_get_params(void *provctx, OSSL_PARAM params[])
{
    OSSL_PARAM *p;

    (void)provctx;
    p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_NAME);
    if (p != NULL && OSSL_PARAM_set_utf8_ptr(p, "NGCC Provider") == 0)
        return 0;
    p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_VERSION);
    if (p != NULL && OSSL_PARAM_set_utf8_ptr(p, "2.1.0") == 0)
        return 0;
    p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_STATUS);
    if (p != NULL && OSSL_PARAM_set_int(p, 1) == 0)
        return 0;
    return 1;
}

static const OSSL_ALGORITHM *ngcc_provider_query(void *provctx, int op_id,
                                                int *no_cache)
{
    (void)provctx;
    *no_cache = 0;
    switch (op_id) {
    case OSSL_OP_KEM:
        return ngcc_kem_algs;
    case OSSL_OP_SIGNATURE:
        return ngcc_sig_algs;
    case OSSL_OP_KEYMGMT:
        return ngcc_kmgmt_algs;
    case OSSL_OP_ENCODER:
        return ngcc_encoder_algs;
    case OSSL_OP_DECODER:
        return ngcc_decoder_algs;
    }
    return NULL;
}

/* Dispatch table */
static const OSSL_DISPATCH ngcc_dispatch_table[] = {
    { OSSL_FUNC_PROVIDER_GETTABLE_PARAMS,   (void (*)(void))ngcc_provider_gettable_params },
    { OSSL_FUNC_PROVIDER_GET_PARAMS,        (void (*)(void))ngcc_provider_get_params },
    { OSSL_FUNC_PROVIDER_QUERY_OPERATION,   (void (*)(void))ngcc_provider_query },
    { OSSL_FUNC_PROVIDER_TEARDOWN,          (void (*)(void))ngcc_provider_teardown },
    { OSSL_FUNC_PROVIDER_GET_CAPABILITIES,  (void (*)(void))ngcc_provider_get_capabilities },
    { 0, NULL }
};

OSSL_provider_init_fn OSSL_provider_init;
__attribute__((visibility("default")))
int OSSL_provider_init(const OSSL_CORE_HANDLE *handle,
                       const OSSL_DISPATCH *in,
                       const OSSL_DISPATCH **out,
                       void **provctx)
{
    NGCC_PROV_CTX *ctx = NULL;

    if (!CRYPTO_THREAD_run_once(&ngcc_oid_once, ngcc_register_sig_oids)
        || !ngcc_oids_ready)
        return 0;

    ctx = OPENSSL_zalloc(sizeof(*ctx));
    if (ctx == NULL)
        return 0;
    ctx->handle = handle;
    ctx->libctx = OSSL_LIB_CTX_new_child(handle, in);

    if (ctx->libctx == NULL) {
        OPENSSL_free(ctx);
        return 0;
    }

    *provctx = ctx;
    *out = ngcc_dispatch_table;

    return 1;
}
