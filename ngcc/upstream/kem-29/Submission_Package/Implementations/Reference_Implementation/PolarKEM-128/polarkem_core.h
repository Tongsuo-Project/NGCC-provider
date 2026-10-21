#ifndef POLARKEM_CORE_H
#define POLARKEM_CORE_H

#include "drng.h"
#include "polarkem_params.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Successful functional-profile operation. */
#define POLARKEM_SUCCESS 0
/** Ciphertext verification failure; a rejection secret was still written. */
#define POLARKEM_ERROR_CIPHERTEXT (-1)
/** Non-canonical serialized public or secret key. */
#define POLARKEM_ERROR_FORMAT (-3)
/** Official DRNG, SM3, or pseudoXOF helper failure. */
#define POLARKEM_ERROR_EXTERNAL (-4)

/**
 * Validate a complete canonical public-key encoding.
 *
 * @param[in] pk POLARKEM_PK_BYTES public-key bytes.
 * @return 0 if canonical, -3 if malformed, or -4 on helper failure.
 */
int polarkem_validate_public_key(
    const unsigned char pk[POLARKEM_PK_BYTES]);

/**
 * Validate a complete secret key and its embedded public key.
 *
 * @param[in] sk POLARKEM_SK_BYTES secret-key bytes.
 * @return 0 if canonical, -3 if malformed, or -4 on helper failure.
 */
int polarkem_validate_secret_key(
    const unsigned char sk[POLARKEM_SK_BYTES]);

/**
 * Generate a serialized public/secret key pair.
 *
 * The official DRNG is called twice: first for the 256-bit public transform
 * seed and then for the 256-bit secret rejection seed.
 *
 * @param[out] pk   POLARKEM_PK_BYTES public-key bytes.
 * @param[out] sk   POLARKEM_SK_BYTES secret-key bytes.
 * @param[in,out] drng Initialized official algorithm DRNG context.
 * @return 0 on success, or -4 on an official helper failure.
 */
int polarkem_keygen(
    unsigned char pk[POLARKEM_PK_BYTES],
    unsigned char sk[POLARKEM_SK_BYTES],
    DRNG_ctx *drng);

/**
 * Encapsulate using a  message obtained from the official algorithm DRNG.
 *
 * @param[in]  pk   POLARKEM_PK_BYTES public-key bytes.
 * @param[out] ss   POLARKEM_SS_BYTES shared-secret bytes.
 * @param[out] ct   POLARKEM_CT_BYTES ciphertext bytes.
 * @param[in,out] drng Initialized official algorithm DRNG context.
 * The public API wrapper validates the PK with polarkem_validate_public_key()
 * before calling this internal primitive.
 *
 * @return 0 on success, or -4 on an official helper failure.
 */
int polarkem_enc(
    const unsigned char pk[POLARKEM_PK_BYTES],
    unsigned char ss[POLARKEM_SS_BYTES],
    unsigned char ct[POLARKEM_CT_BYTES],
    DRNG_ctx *drng);

/**
 * Decapsulate, re-encrypt, and compare the complete ciphertext in constant time.
 *
 * @param[in]  sk POLARKEM_SK_BYTES secret-key bytes.
 * @param[in]  ct POLARKEM_CT_BYTES ciphertext bytes.
 * @param[out] ss POLARKEM_SS_BYTES valid or implicit-rejection secret bytes.
 * The public API wrapper validates the SK with polarkem_validate_secret_key()
 * before calling this internal primitive.
 *
 * @return 0 for a valid ciphertext, -1 after writing a rejection secret for an
 *         invalid ciphertext, or -4 on an official helper failure.
 */
int polarkem_dec(
    const unsigned char sk[POLARKEM_SK_BYTES],
    const unsigned char ct[POLARKEM_CT_BYTES],
    unsigned char ss[POLARKEM_SS_BYTES]);

#ifdef __cplusplus
}
#endif

#endif
