/*
 * NGCC uniform link interface: key encapsulation mechanism (KEM).
 * Official API: API_PKC/.../KEM_AlgorithmInstance.h
 */
#ifndef NGCC_LINK_KEM_H
#define NGCC_LINK_KEM_H

#include "link_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Metadata block returned by ngcc_meta() when h.type == NGCC_TYPE_KEM. */
typedef struct ngcc_meta_kem {
    ngcc_meta_common_t h;
    uint64_t pk_len;   /* claimed public key length, bytes      */
    uint64_t sk_len;   /* claimed private key length, bytes     */
    uint64_t ct_len;   /* claimed ciphertext length, bytes      */
    uint64_t ss_len;   /* claimed shared secret length, bytes   */
} ngcc_meta_kem_t;

/* Official API function signatures (exported by the library). */
typedef unsigned long long (*ngcc_kem_get_len_fn)(void);
typedef int (*ngcc_kem_keygen_fn)(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes);
typedef int (*ngcc_kem_enc_fn)(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes,
    unsigned char *ct, unsigned long long *ct_len_bytes);
typedef int (*ngcc_kem_dec_fn)(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *ct, unsigned long long ct_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes);

/* Resolved entry points, filled by the harness with dlsym(). */
typedef struct ngcc_kem_api {
    ngcc_kem_get_len_fn get_pk_len_bytes;
    ngcc_kem_get_len_fn get_sk_len_bytes;
    ngcc_kem_get_len_fn get_ss_len_bytes;
    ngcc_kem_get_len_fn get_ct_len_bytes;
    ngcc_kem_keygen_fn  keygen;
    ngcc_kem_enc_fn     enc;
    ngcc_kem_dec_fn     dec;
} ngcc_kem_api_t;

#define NGCC_KEM_SYMBOLS \
    X(get_pk_len_bytes, "kem_get_pk_len_bytes") \
    X(get_sk_len_bytes, "kem_get_sk_len_bytes") \
    X(get_ss_len_bytes, "kem_get_ss_len_bytes") \
    X(get_ct_len_bytes, "kem_get_ct_len_bytes") \
    X(keygen,           "kem_keygen") \
    X(enc,              "kem_enc") \
    X(dec,              "kem_dec")

#ifdef __cplusplus
}
#endif
#endif /* NGCC_LINK_KEM_H */
