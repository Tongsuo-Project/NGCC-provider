/*
 * Copyright 2026 The Tongsuo Project Authors. All Rights Reserved.
 * Licensed under the Apache License 2.0; see LICENSE.
 */
#include <string.h>
#include <openssl/evp.h>
#include "ngcc_prov.h"

void ngcc_keymgmt_free(void *keydata)
{
    NGCC_KEY *key = keydata;
    int references;

    if (key == NULL)
        return;
    if (!CRYPTO_atomic_add(&key->references, -1, &references, key->lock)
        || references > 0)
        return;
    OPENSSL_clear_free(key->pk, key->scheme->pk_len);
    OPENSSL_clear_free(key->sk, key->scheme->sk_len);
    CRYPTO_THREAD_lock_free(key->lock);
    OPENSSL_free(key);
}

NGCC_KEY *ngcc_key_new(NGCC_SCHEME *scheme)
{
    NGCC_KEY *key = OPENSSL_zalloc(sizeof(*key));

    if (key == NULL)
        return NULL;
    key->lock = CRYPTO_THREAD_lock_new();
    if (key->lock == NULL) {
        OPENSSL_free(key);
        return NULL;
    }
    key->references = 1;
    key->scheme = scheme;
    return key;
}

int ngcc_key_up_ref(NGCC_KEY *key)
{
    int references;

    if (key == NULL
        || !CRYPTO_atomic_add(&key->references, 1, &references, key->lock))
        return 0;
    return references > 1;
}

void *ngcc_keymgmt_dup(const void *keydata, int selection)
{
    const NGCC_KEY *src = keydata;
    NGCC_KEY *dst;

    if (src == NULL || (dst = ngcc_key_new(src->scheme)) == NULL)
        return NULL;
    if ((selection & OSSL_KEYMGMT_SELECT_PUBLIC_KEY) != 0 && src->pk != NULL) {
        dst->pk = OPENSSL_memdup(src->pk, src->scheme->pk_len);
        if (dst->pk == NULL)
            goto err;
    }
    if ((selection & OSSL_KEYMGMT_SELECT_PRIVATE_KEY) != 0 && src->sk != NULL) {
        dst->sk = OPENSSL_memdup(src->sk, src->scheme->sk_len);
        if (dst->sk == NULL)
            goto err;
    }
    return dst;
err:
    ngcc_keymgmt_free(dst);
    return NULL;
}

/* Recover the decoder reference and transfer ownership to libcrypto. */
static void *ngcc_keymgmt_load(const void *reference, size_t reference_size)
{
    NGCC_KEY *key;

    if (reference == NULL || reference_size != sizeof(key))
        return NULL;
    memcpy(&key, reference, sizeof(key));
    *(NGCC_KEY **)reference = NULL;
    return key;
}

static int ngcc_keymgmt_has(const void *keydata, int selection)
{
    const NGCC_KEY *key = keydata;

    if (key == NULL)
        return 0;
    if ((selection & OSSL_KEYMGMT_SELECT_PUBLIC_KEY) != 0 && key->pk == NULL)
        return 0;
    if ((selection & OSSL_KEYMGMT_SELECT_PRIVATE_KEY) != 0 && key->sk == NULL)
        return 0;
    return 1;
}

static int ngcc_keymgmt_match(const void *keydata1, const void *keydata2,
                             int selection)
{
    const NGCC_KEY *key1 = keydata1;
    const NGCC_KEY *key2 = keydata2;

    if (key1 == NULL || key2 == NULL || key1->scheme != key2->scheme
        || !ngcc_keymgmt_has(key1, selection)
        || !ngcc_keymgmt_has(key2, selection))
        return 0;
    if ((selection & OSSL_KEYMGMT_SELECT_PUBLIC_KEY) != 0
        && CRYPTO_memcmp(key1->pk, key2->pk, key1->scheme->pk_len) != 0)
        return 0;
    if ((selection & OSSL_KEYMGMT_SELECT_PRIVATE_KEY) != 0
        && CRYPTO_memcmp(key1->sk, key2->sk, key1->scheme->sk_len) != 0)
        return 0;
    return 1;
}

static int ngcc_keymgmt_validate(const void *keydata, int selection, int checktype)
{
    (void)checktype;
    return ngcc_keymgmt_has(keydata, selection);
}

static const OSSL_PARAM ngcc_key_types[] = {
    OSSL_PARAM_octet_string(OSSL_PKEY_PARAM_PUB_KEY, NULL, 0),
    OSSL_PARAM_octet_string(OSSL_PKEY_PARAM_PRIV_KEY, NULL, 0),
    OSSL_PARAM_utf8_string(OSSL_PKEY_PARAM_GROUP_NAME, NULL, 0),
    OSSL_PARAM_END
};

static const OSSL_PARAM *ngcc_keymgmt_io_types(int selection)
{
    if ((selection & (OSSL_KEYMGMT_SELECT_KEYPAIR
                      | OSSL_KEYMGMT_SELECT_DOMAIN_PARAMETERS)) != 0)
        return ngcc_key_types;
    return NULL;
}

static int ngcc_keymgmt_import(void *keydata, int selection,
                              const OSSL_PARAM params[])
{
    NGCC_KEY *key = keydata;
    const OSSL_PARAM *p;
    unsigned char *pk = NULL, *sk = NULL;

    if (key == NULL || !ngcc_group_params_match(key->scheme, params))
        return 0;
    if ((selection & OSSL_KEYMGMT_SELECT_KEYPAIR) == 0)
        return (selection & OSSL_KEYMGMT_SELECT_DOMAIN_PARAMETERS) != 0;
    if (params == NULL)
        return 0;
    p = OSSL_PARAM_locate_const(params, OSSL_PKEY_PARAM_PUB_KEY);
    if (p != NULL) {
        if (p->data_type != OSSL_PARAM_OCTET_STRING || p->data == NULL
            || p->data_size != key->scheme->pk_len)
            goto err;
        pk = OPENSSL_memdup(p->data, p->data_size);
        if (pk == NULL)
            goto err;
    }
    p = OSSL_PARAM_locate_const(params, OSSL_PKEY_PARAM_PRIV_KEY);
    if ((selection & OSSL_KEYMGMT_SELECT_PRIVATE_KEY) != 0 && p != NULL) {
        if (p->data_type != OSSL_PARAM_OCTET_STRING || p->data == NULL
            || p->data_size != key->scheme->sk_len)
            goto err;
        sk = OPENSSL_memdup(p->data, p->data_size);
        if (sk == NULL)
            goto err;
    }
    if (pk == NULL && sk == NULL)
        goto err;
    if (pk != NULL) {
        OPENSSL_clear_free(key->pk, key->scheme->pk_len);
        key->pk = pk;
    }
    if (sk != NULL) {
        OPENSSL_clear_free(key->sk, key->scheme->sk_len);
        key->sk = sk;
    }
    return 1;
err:
    OPENSSL_clear_free(pk, key->scheme->pk_len);
    OPENSSL_clear_free(sk, key->scheme->sk_len);
    return 0;
}

static int ngcc_keymgmt_export(void *keydata, int selection,
                              OSSL_CALLBACK *cb, void *arg)
{
    NGCC_KEY *key = keydata;
    OSSL_PARAM params[4];
    size_t n = 0;

    if (key == NULL || !ngcc_keymgmt_has(key, selection))
        return 0;
    if ((selection & OSSL_KEYMGMT_SELECT_PUBLIC_KEY) != 0 && key->pk != NULL)
        params[n++] = OSSL_PARAM_construct_octet_string(
            OSSL_PKEY_PARAM_PUB_KEY, key->pk, key->scheme->pk_len);
    if ((selection & OSSL_KEYMGMT_SELECT_PRIVATE_KEY) != 0 && key->sk != NULL)
        params[n++] = OSSL_PARAM_construct_octet_string(
            OSSL_PKEY_PARAM_PRIV_KEY, key->sk, key->scheme->sk_len);
    if ((selection & OSSL_KEYMGMT_SELECT_DOMAIN_PARAMETERS) != 0)
        params[n++] = OSSL_PARAM_construct_utf8_string(
            OSSL_PKEY_PARAM_GROUP_NAME, (char *)ngcc_group_name(key->scheme), 0);
    params[n] = OSSL_PARAM_construct_end();
    return n != 0 && cb(params, arg);
}

static const OSSL_PARAM ngcc_gettable_params[] = {
    OSSL_PARAM_int(OSSL_PKEY_PARAM_BITS, NULL),
    OSSL_PARAM_int(OSSL_PKEY_PARAM_SECURITY_BITS, NULL),
    OSSL_PARAM_int(OSSL_PKEY_PARAM_MAX_SIZE, NULL),
    OSSL_PARAM_octet_string(OSSL_PKEY_PARAM_PUB_KEY, NULL, 0),
    OSSL_PARAM_octet_string(OSSL_PKEY_PARAM_PRIV_KEY, NULL, 0),
    OSSL_PARAM_octet_string(OSSL_PKEY_PARAM_ENCODED_PUBLIC_KEY, NULL, 0),
    OSSL_PARAM_utf8_string(OSSL_PKEY_PARAM_GROUP_NAME, NULL, 0),
    OSSL_PARAM_utf8_string(OSSL_PKEY_PARAM_DEFAULT_DIGEST, NULL, 0),
    OSSL_PARAM_utf8_string(OSSL_PKEY_PARAM_MANDATORY_DIGEST, NULL, 0),
    OSSL_PARAM_END
};

static const OSSL_PARAM *ngcc_keymgmt_gettable_params(void *provctx)
{
    (void)provctx;
    return ngcc_gettable_params;
}

static int ngcc_keymgmt_get_params(void *keydata, OSSL_PARAM params[])
{
    NGCC_KEY *key = keydata;
    NGCC_SCHEME *scheme;
    OSSL_PARAM *p;

    if (key == NULL)
        return 0;
    scheme = key->scheme;
    p = OSSL_PARAM_locate(params, OSSL_PKEY_PARAM_BITS);
    if (p != NULL && !OSSL_PARAM_set_int(p, scheme->security_bits))
        return 0;
    p = OSSL_PARAM_locate(params, OSSL_PKEY_PARAM_SECURITY_BITS);
    if (p != NULL && !OSSL_PARAM_set_int(p, scheme->security_bits))
        return 0;
    p = OSSL_PARAM_locate(params, OSSL_PKEY_PARAM_MAX_SIZE);
    if (p != NULL && !OSSL_PARAM_set_int(p, (int)scheme->output_len))
        return 0;
    p = OSSL_PARAM_locate(params, OSSL_PKEY_PARAM_PUB_KEY);
    if (p != NULL && (key->pk == NULL
                     || !OSSL_PARAM_set_octet_string(p, key->pk, scheme->pk_len)))
        return 0;
    p = OSSL_PARAM_locate(params, OSSL_PKEY_PARAM_PRIV_KEY);
    if (p != NULL && (key->sk == NULL
                     || !OSSL_PARAM_set_octet_string(p, key->sk, scheme->sk_len)))
        return 0;
    p = OSSL_PARAM_locate(params, OSSL_PKEY_PARAM_ENCODED_PUBLIC_KEY);
    if (p != NULL && (key->pk == NULL
                     || !OSSL_PARAM_set_octet_string(p, key->pk, scheme->pk_len)))
        return 0;
    p = OSSL_PARAM_locate(params, OSSL_PKEY_PARAM_GROUP_NAME);
    if (p != NULL && !OSSL_PARAM_set_utf8_string(p, ngcc_group_name(scheme)))
        return 0;
    p = OSSL_PARAM_locate(params, OSSL_PKEY_PARAM_DEFAULT_DIGEST);
    if (p != NULL && !OSSL_PARAM_set_utf8_string(p, ""))
        return 0;
    p = OSSL_PARAM_locate(params, OSSL_PKEY_PARAM_MANDATORY_DIGEST);
    if (p != NULL && !OSSL_PARAM_set_utf8_string(p, ""))
        return 0;
    return 1;
}

static void *ngcc_keymgmt_gen(void *genctx, OSSL_CALLBACK *cb, void *cbarg)
{
    NGCC_OP *ctx = genctx;
    NGCC_SCHEME *scheme = ctx->scheme;
    NGCC_KEY *key = ngcc_key_new(scheme);
    unsigned long long pk_len = scheme->pk_len, sk_len = scheme->sk_len;
    int ret;

    (void)cb;
    (void)cbarg;
    if (key == NULL)
        return NULL;
    /* TLS paramgen creates the scheme-only key used to import a peer share. */
    if ((ctx->selection & OSSL_KEYMGMT_SELECT_KEYPAIR) == 0)
        return key;
    key->pk = OPENSSL_zalloc(scheme->pk_len);
    key->sk = OPENSSL_zalloc(scheme->sk_len);
    if (key->pk == NULL || key->sk == NULL)
        goto err;
    pthread_mutex_lock(scheme->lock);
    if (!ngcc_random_seed(scheme, ctx->libctx)) {
        pthread_mutex_unlock(scheme->lock);
        goto err;
    }
    ret = scheme->keygen(key->pk, &pk_len, key->sk, &sk_len);
    pthread_mutex_unlock(scheme->lock);
    if (ret != 0 || pk_len != scheme->pk_len || sk_len != scheme->sk_len)
        goto err;
    return key;
err:
    ngcc_keymgmt_free(key);
    return NULL;
}

/* TLS creates a parameter-only key, then imports the peer's wire public key. */
static const OSSL_PARAM *ngcc_keymgmt_settable_params(void *provctx)
{
    static const OSSL_PARAM types[] = {
        OSSL_PARAM_octet_string(OSSL_PKEY_PARAM_ENCODED_PUBLIC_KEY, NULL, 0),
        OSSL_PARAM_utf8_string(OSSL_PKEY_PARAM_GROUP_NAME, NULL, 0),
        OSSL_PARAM_END
    };

    (void)provctx;
    return types;
}

static int ngcc_keymgmt_set_params(void *v, const OSSL_PARAM params[])
{
    NGCC_KEY *key = v;
    const OSSL_PARAM *p;
    unsigned char *copy;

    if (key == NULL || !ngcc_group_params_match(key->scheme, params))
        return 0;
    if (params == NULL)
        return 1;
    p = OSSL_PARAM_locate_const(params, OSSL_PKEY_PARAM_ENCODED_PUBLIC_KEY);
    if (p == NULL)
        return 1;
    if (p->data_type != OSSL_PARAM_OCTET_STRING || p->data == NULL
        || p->data_size != key->scheme->pk_len)
        return 0;
    copy = OPENSSL_memdup(p->data, p->data_size);
    if (copy == NULL)
        return 0;
    OPENSSL_clear_free(key->pk, key->scheme->pk_len);
    key->pk = copy;
    return 1;
}

static const OSSL_PARAM *ngcc_keymgmt_gen_settable_params(void *genctx, void *provctx)
{
    static const OSSL_PARAM types[] = {
        OSSL_PARAM_utf8_string(OSSL_PKEY_PARAM_GROUP_NAME, NULL, 0),
        OSSL_PARAM_END
    };

    (void)genctx;
    (void)provctx;
    return types;
}

static int ngcc_keymgmt_gen_set_params(void *v, const OSSL_PARAM params[])
{
    NGCC_OP *ctx = v;

    return ctx != NULL && ngcc_group_params_match(ctx->scheme, params);
}

/* One small constructor per instance; all ownership/encoding logic stays above. */
#define NGCC_KEYMGMT(tag, getter, names, primary)                                                   \
static void *ngcc_##tag##_keymgmt_new(void *provctx)                                                \
{                                                                                                   \
    (void)provctx;                                                                                  \
    return ngcc_key_new(getter());                                                                  \
}                                                                                                   \
static const char *ngcc_##tag##_query_operation_name(int operation)                                 \
{                                                                                                   \
    NGCC_SCHEME *s = getter();                                                                      \
    return operation == (s->type == 1 ? OSSL_OP_KEM : OSSL_OP_SIGNATURE) ? primary : NULL;          \
}                                                                                                   \
static void *ngcc_##tag##_keymgmt_gen_init(void *provctx, int selection, const OSSL_PARAM params[]) \
{                                                                                                   \
    NGCC_OP *ctx = ngcc_op_new(getter(), provctx);                                                  \
    if (ctx == NULL)                                                                                \
        return NULL;                                                                                \
    ctx->selection = selection;                                                                     \
    if (!ngcc_keymgmt_gen_set_params(ctx, params)) {                                                \
        ngcc_op_free(ctx);                                                                          \
        return NULL;                                                                                \
    }                                                                                               \
    return ctx;                                                                                     \
}                                                                                                   \
const OSSL_DISPATCH ngcc_##tag##_kmgmt_functions[] = {                                              \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_NEW, ngcc_##tag##_keymgmt_new),                                 \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_FREE, ngcc_keymgmt_free),                                       \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_LOAD, ngcc_keymgmt_load),                                       \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_DUP, ngcc_keymgmt_dup),                                         \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_HAS, ngcc_keymgmt_has),                                         \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_MATCH, ngcc_keymgmt_match),                                     \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_VALIDATE, ngcc_keymgmt_validate),                               \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_IMPORT, ngcc_keymgmt_import),                                   \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_IMPORT_TYPES, ngcc_keymgmt_io_types),                           \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_EXPORT, ngcc_keymgmt_export),                                   \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_EXPORT_TYPES, ngcc_keymgmt_io_types),                           \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_GET_PARAMS, ngcc_keymgmt_get_params),                           \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_GETTABLE_PARAMS, ngcc_keymgmt_gettable_params),                 \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_SET_PARAMS, ngcc_keymgmt_set_params),                           \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_SETTABLE_PARAMS, ngcc_keymgmt_settable_params),                 \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_QUERY_OPERATION_NAME, ngcc_##tag##_query_operation_name),       \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_GEN_INIT, ngcc_##tag##_keymgmt_gen_init),                       \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_GEN, ngcc_keymgmt_gen),                                         \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_GEN_CLEANUP, ngcc_op_free),                                     \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_GEN_SET_PARAMS, ngcc_keymgmt_gen_set_params),                   \
    NGCC_DISPATCH(OSSL_FUNC_KEYMGMT_GEN_SETTABLE_PARAMS, ngcc_keymgmt_gen_settable_params),         \
    { 0, NULL }                                                                                     \
};

#define NGCC_KEM NGCC_KEYMGMT
#define NGCC_SIG NGCC_KEYMGMT
#include "ngcc_kem_algorithms.inc"
#include "ngcc_sig_algorithms.inc"
#undef NGCC_SIG
#undef NGCC_KEM
#undef NGCC_KEYMGMT
