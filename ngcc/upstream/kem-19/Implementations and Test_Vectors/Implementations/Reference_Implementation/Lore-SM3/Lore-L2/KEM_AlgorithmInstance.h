#ifndef KEM_ALGORITHM_INSTANCE_H
#define KEM_ALGORITHM_INSTANCE_H

#include "params.h"

/* Generate test vectors rather than blank templates. */
#define OUTPUT_BLANK_TEST_VECTORS 0

/* API_PKC instance name for each parameter set. */
#if LORE_LEVEL == 1
#define ALGORITHM_INSTANCE "Lore-SM3-L1"
#elif LORE_LEVEL == 2
#define ALGORITHM_INSTANCE "Lore-SM3-L2"
#elif LORE_LEVEL == 3
#define ALGORITHM_INSTANCE "Lore-SM3-L3"
#elif LORE_LEVEL == 4
#define ALGORITHM_INSTANCE "Lore-SM3-L4"
#else
#error "Unsupported Lore level"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    /* Obtain the claimed byte length of the public key */
    unsigned long long kem_get_pk_len_bytes(void);

    /* Obtain the claimed byte length of the private key */
    unsigned long long kem_get_sk_len_bytes(void);

    /* Obtain the claimed byte length of the shared secret key */
    unsigned long long kem_get_ss_len_bytes(void);

    /* Obtain the claimed byte length of the ciphertext */
    unsigned long long kem_get_ct_len_bytes(void);

    /*
     * Key generation
     * Return 0 on success; otherwise return a self-defined negative error code.
     */
    int kem_keygen(
        unsigned char *pk, unsigned long long *pk_len_bytes,
        unsigned char *sk, unsigned long long *sk_len_bytes);

    /*
     * Encapsulation
     * Return 0 on success; otherwise return a self-defined negative error code.
     */
    int kem_enc(
        unsigned char *pk, unsigned long long pk_len_bytes,
        unsigned char *ss, unsigned long long *ss_len_bytes,
        unsigned char *ct, unsigned long long *ct_len_bytes);

    /*
     * Decapsulation
     * Return 0 after producing a shared secret, including implicit rejection:
     * an invalid ciphertext produces a rejection secret without exposing validity.
     * This instance does not return -1 for ciphertext verification failure.
     * Parameter/internal errors return a negative error code <= -2.
     */
    int kem_dec(
        unsigned char *sk, unsigned long long sk_len_bytes,
        unsigned char *ct, unsigned long long ct_len_bytes,
        unsigned char *ss, unsigned long long *ss_len_bytes);

#ifdef __cplusplus
}
#endif

#endif
