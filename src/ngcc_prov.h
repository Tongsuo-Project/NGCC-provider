/*
 * Copyright 2026 The Tongsuo Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef NGCC_PROVIDER_H
#define NGCC_PROVIDER_H

#include <openssl/core.h>
#include <openssl/core_dispatch.h>
#include <openssl/core_names.h>
#include <openssl/params.h>
#include "ngcc_config.h"
#include "ngcc_ref.h"

/* The original provider context: child libctx follows the loading host. */
typedef struct ngcc_prov_ctx_st {
    OSSL_LIB_CTX *libctx;
    const OSSL_CORE_HANDLE *handle;
} NGCC_PROV_CTX;

/* Provider-owned key buffers; scheme points to one immutable author descriptor. */
typedef struct {
    NGCC_SCHEME *scheme;
    unsigned char *pk;
    unsigned char *sk;
    int references;
    CRYPTO_RWLOCK *lock;
} NGCC_KEY;

/* Key generation and KEM operations retain their provider context and key. */
typedef struct {
    OSSL_LIB_CTX *libctx;
    NGCC_SCHEME *scheme;
    NGCC_KEY *key;
    int selection;
} NGCC_OP;

/* Allocate a blank key for scheme s. Returns NULL on allocation failure. */
NGCC_KEY *ngcc_key_new(NGCC_SCHEME *s);
/* Retain key by adding a reference; key must already be owned by the caller. */
int ngcc_key_up_ref(NGCC_KEY *key);
/* Release key v, clearing its buffers on the last reference; NULL is permitted. */
void ngcc_keymgmt_free(void *v);
/* Copy the selected public/private components of key v. */
void *ngcc_keymgmt_dup(const void *v, int selection);
/* Allocate an operation for scheme s in provider context provctx. */
void *ngcc_op_new(NGCC_SCHEME *s, void *provctx);
/* Release operation v and its key reference. */
void ngcc_op_free(void *v);
/* Duplicate operation v, retaining another reference to its key. */
void *ngcc_op_dup(void *v);
/* Initialize operation v from key; params contains optional EVP parameters. */
int ngcc_op_init(void *v, void *key, const OSSL_PARAM params[]);
/* Seed scheme s from libctx. Caller holds s->lock across seed and operation. */
int ngcc_random_seed(NGCC_SCHEME *s, OSSL_LIB_CTX *libctx);
/* Return the configured TLS group name for s, or its algorithm name. */
const char *ngcc_group_name(const NGCC_SCHEME *s);
/* Validate optional GROUP_NAME in params against scheme s. */
int ngcc_group_params_match(const NGCC_SCHEME *s, const OSSL_PARAM params[]);
/* Look up the sig OID for algorithm; return NID_undef if absent. */
int ngcc_sig_nid(const char *algorithm);

#define NGCC_DISPATCH(id, fn) { id, (void (*)(void))fn }

/* Declarations only. The lists contain names, not provider implementation code. */
#define NGCC_KEM(tag, getter, names, primary)                                  \
    extern NGCC_SCHEME *getter(void);                                          \
    extern const OSSL_DISPATCH ngcc_##tag##_kmgmt_functions[];                 \
    extern const OSSL_DISPATCH ngcc_##tag##_functions[];
#include "ngcc_kem_algorithms.inc"
#define NGCC_SIG NGCC_KEM
#include "ngcc_sig_algorithms.inc"
#undef NGCC_SIG
#undef NGCC_KEM

#define NGCC_SIG(tag, getter, names, primary)                                  \
    extern const OSSL_DISPATCH ngcc_sig_##tag##_encoder_text_functions[];      \
    extern const OSSL_DISPATCH ngcc_sig_##tag##_encoder_spki_pem_functions[];  \
    extern const OSSL_DISPATCH ngcc_sig_##tag##_encoder_pkcs8_pem_functions[]; \
    extern const OSSL_DISPATCH ngcc_sig_##tag##_encoder_spki_der_functions[];  \
    extern const OSSL_DISPATCH ngcc_sig_##tag##_encoder_pkcs8_der_functions[]; \
    extern const OSSL_DISPATCH ngcc_sig_##tag##_decoder_spki_der_functions[];  \
    extern const OSSL_DISPATCH ngcc_sig_##tag##_decoder_pkcs8_der_functions[];
#include "ngcc_sig_algorithms.inc"
#undef NGCC_SIG

#endif
