#ifndef POLARKEM_CT_H
#define POLARKEM_CT_H

#include "polarkem_params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Overwrite a stack or heap buffer through a volatile byte pointer.
 *
 * @param[out] buffer Buffer to overwrite.
 * @param[in]  length Number of bytes to clear.
 * @return This function has no return value.
 */
void polarkem_secure_clear(void *buffer, size_t length);

/**
 * Serialize a public key from its transform seed.
 *
 * @param[in]  seed POLARKEM_SEED_BYTES transform-seed bytes.
 * @param[out] pk   POLARKEM_PK_BYTES fully initialized bytes.
 * @return 0 on success, or -4 if pseudoXOF reports an error.
 */
int polarkem_serialize_public_key(
    const unsigned char seed[POLARKEM_SEED_BYTES],
    unsigned char pk[POLARKEM_PK_BYTES]);

/**
 * Serialize a secret key containing the rejection seed and complete public key.
 *
 * @param[in]  z  POLARKEM_SEED_BYTES implicit-rejection seed bytes.
 * @param[in]  pk POLARKEM_PK_BYTES serialized public-key bytes.
 * @param[out] sk POLARKEM_SK_BYTES fully initialized bytes.
 * @return 0 on success, or -4 if SM3/pseudoXOF reports an error.
 */
int polarkem_serialize_secret_key(
    const unsigned char z[POLARKEM_SEED_BYTES],
    const unsigned char pk[POLARKEM_PK_BYTES],
    unsigned char sk[POLARKEM_SK_BYTES]);

/**
 * Validate the header, transform seed, and deterministic PK padding.
 *
 * @param[in] pk POLARKEM_PK_BYTES serialized public-key bytes.
 * @return 0 for the unique canonical encoding, -3 for malformed content, or
 *         -4 if pseudoXOF reports an error while regenerating the encoding.
 */
int polarkem_validate_public_key(
    const unsigned char pk[POLARKEM_PK_BYTES]);

/**
 * Validate the SK header, embedded canonical PK, and deterministic SK padding.
 *
 * @param[in] sk POLARKEM_SK_BYTES serialized secret-key bytes.
 * @return 0 for the unique canonical encoding, -3 for malformed content, or
 *         -4 if an official hash/XOF helper reports an error.
 */
int polarkem_validate_secret_key(
    const unsigned char sk[POLARKEM_SK_BYTES]);

/**
 * Compute SM3 over the complete serialized public key.
 *
 * @param[in]  pk     POLARKEM_PK_BYTES input bytes.
 * @param[out] digest 32 digest bytes.
 * @return 0 on success, or -4 if SM3 reports an error.
 */
int polarkem_hash_public_key(
    const unsigned char pk[POLARKEM_PK_BYTES],
    unsigned char digest[32]);

/**
 * Deterministically encrypt a supplied message and construct the complete CT.
 *
 * @param[in]  pk POLARKEM_PK_BYTES serialized public-key bytes.
 * @param[in]  mu POLARKEM_MESSAGE_BYTES message bytes.
 * @param[out] ct POLARKEM_CT_BYTES fully initialized ciphertext bytes.
 * @return 0 on success, or -4 if an official hash/XOF call reports an error.
 */
int polarkem_build_ciphertext(
    const unsigned char pk[POLARKEM_PK_BYTES],
    const unsigned char mu[POLARKEM_MESSAGE_BYTES],
    unsigned char ct[POLARKEM_CT_BYTES]);

/**
 * Recover a candidate message from the compressed coefficient payload.
 * Authenticity is deliberately not decided here; the caller re-encrypts and
 * compares the complete serialized ciphertext.
 *
 * @param[in]  pk POLARKEM_PK_BYTES serialized public-key bytes.
 * @param[in]  ct POLARKEM_CT_BYTES ciphertext bytes.
 * @param[out] mu POLARKEM_MESSAGE_BYTES recovered candidate bytes.
 * @return 0 on success, or -4 if permutation expansion reports an XOF error.
 */
int polarkem_recover_message(
    const unsigned char pk[POLARKEM_PK_BYTES],
    const unsigned char ct[POLARKEM_CT_BYTES],
    unsigned char mu[POLARKEM_MESSAGE_BYTES]);

/**
 * Derive the valid shared secret XOF("PolarKEM-SS-v1" || mu || SM3(ct)).
 *
 * @param[in]  mu POLARKEM_MESSAGE_BYTES message bytes.
 * @param[in]  ct POLARKEM_CT_BYTES complete ciphertext bytes.
 * @param[out] ss POLARKEM_SS_BYTES shared-secret bytes.
 * @return 0 on success, or -4 if an official hash/XOF call reports an error.
 */
int polarkem_derive_valid_secret(
    const unsigned char mu[POLARKEM_MESSAGE_BYTES],
    const unsigned char ct[POLARKEM_CT_BYTES],
    unsigned char ss[POLARKEM_SS_BYTES]);

/**
 * Derive XOF("PolarKEM-REJ-v1" || z || SM3(ct)) for implicit rejection.
 *
 * @param[in]  z  POLARKEM_SEED_BYTES rejection-seed bytes.
 * @param[in]  ct POLARKEM_CT_BYTES complete ciphertext bytes.
 * @param[out] ss POLARKEM_SS_BYTES rejection-secret bytes.
 * @return 0 on success, or -4 if an official hash/XOF call reports an error.
 */
int polarkem_derive_reject_secret(
    const unsigned char z[POLARKEM_SEED_BYTES],
    const unsigned char ct[POLARKEM_CT_BYTES],
    unsigned char ss[POLARKEM_SS_BYTES]);

/**
 * Compare complete ciphertexts without data-dependent early exit.
 *
 * @param[in] left  POLARKEM_CT_BYTES input bytes.
 * @param[in] right POLARKEM_CT_BYTES input bytes.
 * @return 1 if all bytes are equal, otherwise 0.
 */
unsigned char polarkem_ciphertext_equal(
    const unsigned char left[POLARKEM_CT_BYTES],
    const unsigned char right[POLARKEM_CT_BYTES]);

#ifdef __cplusplus
}
#endif

#endif
