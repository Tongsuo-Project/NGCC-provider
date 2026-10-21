/*
 * NGCC uniform link interface: digital signature (SIG).
 * Official API: API_PKC/.../SIG_AlgorithmInstance.h
 */
#ifndef NGCC_LINK_SIG_H
#define NGCC_LINK_SIG_H

#include "link_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Metadata block returned by ngcc_meta() when h.type == NGCC_TYPE_SIG. */
typedef struct ngcc_meta_sig {
    ngcc_meta_common_t h;
    uint64_t pk_len;   /* claimed public key length, bytes  */
    uint64_t sk_len;   /* claimed private key length, bytes */
    uint64_t sn_len;   /* claimed signature length, bytes   */
} ngcc_meta_sig_t;

typedef unsigned long long (*ngcc_sig_get_len_fn)(void);
typedef int (*ngcc_sig_keygen_fn)(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes);
typedef int (*ngcc_sig_sign_fn)(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes,
    unsigned char *sn, unsigned long long *sn_len_bytes);
typedef int (*ngcc_sig_verify_fn)(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *sn, unsigned long long sn_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes);

typedef struct ngcc_sig_api {
    ngcc_sig_get_len_fn get_pk_len_bytes;
    ngcc_sig_get_len_fn get_sk_len_bytes;
    ngcc_sig_get_len_fn get_sn_len_bytes;
    ngcc_sig_keygen_fn  keygen;
    ngcc_sig_sign_fn    sign;
    ngcc_sig_verify_fn  verify;
} ngcc_sig_api_t;

#define NGCC_SIG_SYMBOLS \
    X(get_pk_len_bytes, "sig_get_pk_len_bytes") \
    X(get_sk_len_bytes, "sig_get_sk_len_bytes") \
    X(get_sn_len_bytes, "sig_get_sn_len_bytes") \
    X(keygen,           "sig_keygen") \
    X(sign,             "sig_sign") \
    X(verify,           "sig_verify")

#ifdef __cplusplus
}
#endif
#endif /* NGCC_LINK_SIG_H */
