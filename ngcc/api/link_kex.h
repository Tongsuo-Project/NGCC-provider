/*
 * NGCC uniform link interface: key exchange protocol (KEX).
 * Official API: API_PKC/.../KEX_AlgorithmInstance.h
 * Passes 1..3 use the fixed names below; passes 4.. are resolved by the harness
 * as kex_generate_pass<k>_msg_<a|b> (odd k: initiator), per the official driver.
 */
#ifndef NGCC_LINK_KEX_H
#define NGCC_LINK_KEX_H

#include "link_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Metadata block returned by ngcc_meta() when h.type == NGCC_TYPE_KEX. */
typedef struct ngcc_meta_kex {
    ngcc_meta_common_t h;
    uint64_t passes;        /* claimed number of passes                    */
    uint64_t pk_len;        /* long-term public key length (0 if none)     */
    uint64_t sk_len;        /* long-term private key length (0 if none)    */
    uint64_t sta_len;       /* max initiator state buffer, bytes           */
    uint64_t stb_len;       /* max responder state buffer, bytes           */
    uint64_t ss_len;        /* shared secret length, bytes                 */
    uint64_t total_msg_len; /* claimed total length of all messages, bytes */
} ngcc_meta_kex_t;

typedef unsigned long long (*ngcc_kex_get_len_fn)(void);
typedef int (*ngcc_kex_init_fn)(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes,
    unsigned char *st, unsigned long long *st_len_bytes);
/* pass1: (ska, pkb, sta, m1)  pass3: (ska, pkb, m2, sta, m3) */
typedef int (*ngcc_kex_pass1_fn)(
    unsigned char *ska, unsigned long long ska_len_bytes,
    unsigned char *pkb, unsigned long long pkb_len_bytes,
    unsigned char *sta, unsigned long long *sta_len_bytes,
    unsigned char *m1, unsigned long long *m1_len_bytes);
typedef int (*ngcc_kex_passn_fn)(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *min, unsigned long long min_len_bytes,
    unsigned char *st, unsigned long long *st_len_bytes,
    unsigned char *mout, unsigned long long *mout_len_bytes);
typedef int (*ngcc_kex_derive_fn)(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes,
    unsigned char *st, unsigned long long st_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes);

typedef struct ngcc_kex_api {
    ngcc_kex_get_len_fn get_passes_num;
    ngcc_kex_get_len_fn get_pk_len_bytes;
    ngcc_kex_get_len_fn get_sk_len_bytes;
    ngcc_kex_get_len_fn get_sta_len_bytes;
    ngcc_kex_get_len_fn get_stb_len_bytes;
    ngcc_kex_get_len_fn get_ss_len_bytes;
    ngcc_kex_get_len_fn get_total_msg_len_bytes;
    ngcc_kex_init_fn    init_a;
    ngcc_kex_init_fn    init_b;
    ngcc_kex_pass1_fn   pass1_a;
    ngcc_kex_passn_fn   pass2_b;   /* may be NULL for 1-pass protocols */
    ngcc_kex_passn_fn   pass3_a;   /* may be NULL for 1- and 2-pass    */
    ngcc_kex_derive_fn  derive_a;
    ngcc_kex_derive_fn  derive_b;
} ngcc_kex_api_t;

/* Required symbols. pass2_b/pass3_a are resolved optionally. */
#define NGCC_KEX_SYMBOLS \
    X(get_passes_num,         "kex_get_passes_num") \
    X(get_pk_len_bytes,       "kex_get_pk_len_bytes") \
    X(get_sk_len_bytes,       "kex_get_sk_len_bytes") \
    X(get_sta_len_bytes,      "kex_get_sta_len_bytes") \
    X(get_stb_len_bytes,      "kex_get_stb_len_bytes") \
    X(get_ss_len_bytes,       "kex_get_ss_len_bytes") \
    X(get_total_msg_len_bytes,"kex_get_total_msg_len_bytes") \
    X(init_a,                 "kex_init_a") \
    X(init_b,                 "kex_init_b") \
    X(pass1_a,                "kex_generate_pass1_msg_a") \
    X(derive_a,               "kex_derive_ss_a") \
    X(derive_b,               "kex_derive_ss_b")
#define NGCC_KEX_OPTIONAL_SYMBOLS \
    X(pass2_b,                "kex_generate_pass2_msg_b") \
    X(pass3_a,                "kex_generate_pass3_msg_a")

#ifdef __cplusplus
}
#endif
#endif /* NGCC_LINK_KEX_H */
