/*
 * Copyright 2026 The Tongsuo Project Authors. All Rights Reserved.
 * Licensed under the Apache License 2.0; see LICENSE.
 */
#include <openssl/rand.h>
#include "ngcc_prov.h"

void *ngcc_op_new(NGCC_SCHEME *scheme, void *provctx)
{
    NGCC_PROV_CTX *provider = provctx;
    NGCC_OP *ctx = OPENSSL_zalloc(sizeof(*ctx));

    if (ctx != NULL) {
        ctx->scheme = scheme;
        ctx->libctx = provider != NULL ? provider->libctx : NULL;
    }
    return ctx;
}

void ngcc_op_free(void *vctx)
{
    NGCC_OP *ctx = vctx;

    if (ctx == NULL)
        return;
    ngcc_keymgmt_free(ctx->key);
    OPENSSL_free(ctx);
}

void *ngcc_op_dup(void *vctx)
{
    NGCC_OP *src = vctx;
    NGCC_OP *dst;

    if (src == NULL || (dst = ngcc_op_new(src->scheme, NULL)) == NULL)
        return NULL;
    dst->libctx = src->libctx;
    dst->selection = src->selection;
    if (src->key != NULL) {
        if (!ngcc_key_up_ref(src->key))
            goto err;
        dst->key = src->key;
    }
    return dst;
err:
    ngcc_op_free(dst);
    return NULL;
}

int ngcc_op_init(void *vctx, void *vkey, const OSSL_PARAM params[])
{
    NGCC_OP *ctx = vctx;
    NGCC_KEY *key = vkey;

    (void)params;
    if (ctx == NULL || key == NULL || key->scheme != ctx->scheme)
        return 0;
    if (!ngcc_key_up_ref(key))
        return 0;
    ngcc_keymgmt_free(ctx->key);
    ctx->key = key;
    return 1;
}

/* Seed author DRNG state from the host. The caller holds scheme->lock. */
int ngcc_random_seed(NGCC_SCHEME *scheme, OSSL_LIB_CTX *libctx)
{
    unsigned char seed[64];
    int ok;

    if (RAND_priv_bytes_ex(libctx, seed, sizeof(seed), 0) != 1)
        return 0;
    ok = scheme->seed(seed, sizeof(seed)) == 0;
    OPENSSL_cleanse(seed, sizeof(seed));
    return ok;
}
