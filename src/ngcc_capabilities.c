/*
 * Copyright 2026 The Tongsuo Project Authors. All Rights Reserved.
 * Licensed under the Apache License 2.0; see LICENSE.
 */
#include <string.h>
#include <openssl/ssl.h>
#include "ngcc_prov.h"
#include "ngcc_capabilities.h"
#include "ngcc_oid.h"

/* Fixed private-use IDs live in metadata/tls-groups.json. Never derive an ID
 * from registry position: excluding one proposal must not renumber the others. */
typedef struct {
    const char *name;
    const char *algorithm;
    unsigned int id;
    unsigned int security_bits;
} NGCC_TLS_GROUP;

static const NGCC_TLS_GROUP ngcc_tls_groups[] = {
#define NGCC_TLS_GROUP_ENTRY(name, algorithm, id, bits) { name, algorithm, id, bits },
#include "ngcc_tls_groups.inc"
#undef NGCC_TLS_GROUP_ENTRY
    { NULL, NULL, 0, 0 }
};

/* s identifies the instance; return a stable name with provider lifetime. */
const char *ngcc_group_name(const NGCC_SCHEME *s)
{
    size_t i;

    for (i = 0; ngcc_tls_groups[i].name != NULL; ++i)
        if (strcmp(s->name, ngcc_tls_groups[i].algorithm) == 0)
            return ngcc_tls_groups[i].name;
    return s->name;
}

/* s is the fixed instance, params are caller-supplied optional parameters.
 * An instance cannot silently accept a different TLS group. */
int ngcc_group_params_match(const NGCC_SCHEME *s, const OSSL_PARAM params[])
{
    const OSSL_PARAM *p;
    const char *group;
    size_t n;

    if (params == NULL)
        return 1;
    p = OSSL_PARAM_locate_const(params, OSSL_PKEY_PARAM_GROUP_NAME);
    if (p == NULL)
        return 1;
    if (p->data_type != OSSL_PARAM_UTF8_STRING || p->data == NULL)
        return 0;
    group = p->data;
    n = p->data_size;
    if (n > 0 && group[n - 1] == '\0')
        --n;
    return (n == strlen(s->name) && memcmp(group, s->name, n) == 0)
        || (n == strlen(ngcc_group_name(s))
            && memcmp(group, ngcc_group_name(s), n) == 0);
}

/* Enumerate TLS-only KEM groups. cb must consume/copy params during the call;
 * arg is its caller-owned state, and provctx is the NGCC provider context. */
static int ngcc_get_tls_groups(OSSL_CALLBACK *cb, void *arg)
{
    unsigned int is_kem = 1;
    int min_tls = TLS1_3_VERSION, max_tls = TLS1_3_VERSION;
    int no_dtls = -1;
    size_t i;

    for (i = 0; ngcc_tls_groups[i].name != NULL; ++i) {
        const NGCC_TLS_GROUP *g = &ngcc_tls_groups[i];
        unsigned int id = g->id, bits = g->security_bits;
        OSSL_PARAM params[] = {
            OSSL_PARAM_utf8_string(OSSL_CAPABILITY_TLS_GROUP_NAME,
                                   (char *)g->name, 0),
            OSSL_PARAM_utf8_string(OSSL_CAPABILITY_TLS_GROUP_NAME_INTERNAL,
                                   (char *)g->algorithm, 0),
            OSSL_PARAM_utf8_string(OSSL_CAPABILITY_TLS_GROUP_ALG,
                                   (char *)g->algorithm, 0),
            OSSL_PARAM_uint(OSSL_CAPABILITY_TLS_GROUP_ID, &id),
            OSSL_PARAM_uint(OSSL_CAPABILITY_TLS_GROUP_SECURITY_BITS, &bits),
            OSSL_PARAM_int(OSSL_CAPABILITY_TLS_GROUP_MIN_TLS, &min_tls),
            OSSL_PARAM_int(OSSL_CAPABILITY_TLS_GROUP_MAX_TLS, &max_tls),
            OSSL_PARAM_int(OSSL_CAPABILITY_TLS_GROUP_MIN_DTLS, &no_dtls),
            OSSL_PARAM_int(OSSL_CAPABILITY_TLS_GROUP_MAX_DTLS, &no_dtls),
            OSSL_PARAM_uint(OSSL_CAPABILITY_TLS_GROUP_IS_KEM, &is_kem),
            OSSL_PARAM_END
        };

        if (!cb(params, arg))
            return 0;
    }
    return 1;
}

typedef struct {
    const char *name;
    const char *algorithm;
    const char *oid;
    unsigned int code_point;
    unsigned int security_bits;
} NGCC_TLS_SIGALG;

static const NGCC_TLS_SIGALG ngcc_tls_sigalgs[] = {
#define NGCC_TLS_SIGALG_ENTRY(name, algorithm, oid, code, bits)                \
    { name, algorithm, oid, code, bits },
#include "ngcc_tls_sigalgs.inc"
#undef NGCC_TLS_SIGALG_ENTRY
    { NULL, NULL, NULL, 0, 0 }
};

static int ngcc_get_tls_sigalgs(OSSL_CALLBACK *cb, void *arg)
{
    int min_tls = TLS1_3_VERSION, max_tls = TLS1_3_VERSION;
    int no_dtls = -1;
    size_t i;

    for (i = 0; ngcc_tls_sigalgs[i].name != NULL; ++i) {
        const NGCC_TLS_SIGALG *s = &ngcc_tls_sigalgs[i];
        unsigned int code = s->code_point, bits = s->security_bits;
        OSSL_PARAM params[] = {
            OSSL_PARAM_utf8_string(OSSL_CAPABILITY_TLS_SIGALG_IANA_NAME,
                                   (char *)s->name, 0),
            OSSL_PARAM_utf8_string(OSSL_CAPABILITY_TLS_SIGALG_NAME,
                                   (char *)s->algorithm, 0),
            OSSL_PARAM_utf8_string(OSSL_CAPABILITY_TLS_SIGALG_OID,
                                   (char *)s->oid, 0),
            OSSL_PARAM_uint(OSSL_CAPABILITY_TLS_SIGALG_CODE_POINT, &code),
            OSSL_PARAM_uint(OSSL_CAPABILITY_TLS_SIGALG_SECURITY_BITS, &bits),
            OSSL_PARAM_int(OSSL_CAPABILITY_TLS_SIGALG_MIN_TLS, &min_tls),
            OSSL_PARAM_int(OSSL_CAPABILITY_TLS_SIGALG_MAX_TLS, &max_tls),
            OSSL_PARAM_int(OSSL_CAPABILITY_TLS_SIGALG_MIN_DTLS, &no_dtls),
            OSSL_PARAM_int(OSSL_CAPABILITY_TLS_SIGALG_MAX_DTLS, &no_dtls),
            OSSL_PARAM_END
        };

        if (!cb(params, arg))
            return 0;
    }
    return 1;
}

/* cb consumes each parameter array during the call; arg is callback state. */
int ngcc_provider_get_capabilities(void *provctx, const char *capability,
                                  OSSL_CALLBACK *cb, void *arg)
{
    (void)provctx;
    if (capability == NULL || cb == NULL)
        return 0;
    if (OPENSSL_strcasecmp(capability, "TLS-GROUP") == 0)
        return ngcc_get_tls_groups(cb, arg);
    if (OPENSSL_strcasecmp(capability, "TLS-SIGALG") == 0)
        return ngcc_get_tls_sigalgs(cb, arg);
    return 0;
}
