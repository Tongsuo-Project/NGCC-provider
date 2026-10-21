/*
 * NGCC uniform link interface: cryptographic hash (HASH).
 * Official API: API_CryptHash/.../CryptHash_AlgorithmInstance.h
 */
#ifndef NGCC_LINK_HASH_H
#define NGCC_LINK_HASH_H

#include "link_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Metadata block returned by ngcc_meta() when h.type == NGCC_TYPE_HASH. */
typedef struct ngcc_meta_hash {
    ngcc_meta_common_t h;
    uint64_t digest_bits;  /* DIGEST_BIT_LENGTH of this instance          */
    uint64_t digest_len;   /* digest length in bytes (digest_bits / 8)    */
} ngcc_meta_hash_t;

typedef int (*ngcc_hash_fn)(int digest_len_bits, const unsigned char *msg,
                            unsigned long long msg_len_bits, unsigned char *digest);

typedef struct ngcc_hash_api {
    ngcc_hash_fn hash;
} ngcc_hash_api_t;

#define NGCC_HASH_SYMBOLS \
    X(hash, "CryptHash")

#ifdef __cplusplus
}
#endif
#endif /* NGCC_LINK_HASH_H */
