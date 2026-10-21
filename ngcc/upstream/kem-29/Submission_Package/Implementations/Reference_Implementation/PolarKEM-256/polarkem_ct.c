#include "polarkem_ct.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "auxfunc.h"
#include "polarkem_pack.h"
#include "polarkem_polar.h"

void polarkem_secure_clear(void *buffer, size_t length)
{
    volatile unsigned char *p = (volatile unsigned char *)buffer;

    while (length != 0u) {
        *p++ = 0u;
        --length;
    }
}

/**
 * Compare a fixed public length without a data-dependent early exit.
 *
 * @param[in] left   First byte string.
 * @param[in] right  Second byte string.
 * @param[in] length Number of bytes in both strings.
 * @return 1 if all bytes match, otherwise 0.
 */
static unsigned char polarkem_equal_bytes(
    const unsigned char *left,
    const unsigned char *right,
    size_t length)
{
    uint32_t difference = 0u;
    size_t i;

    for (i = 0u; i < length; ++i) {
        difference |= (uint32_t)(left[i] ^ right[i]);
    }
    return (unsigned char)(((difference - UINT32_C(1)) >> 8) & UINT32_C(1));
}

/**
 * Write the canonical common object header.
 *
 * @param[out] object POLARKEM_HEADER_BYTES initialized header bytes.
 * @return This function has no return value.
 */
static void polarkem_write_header(unsigned char object[POLARKEM_HEADER_BYTES])
{
    static const unsigned char magic[8] = {
        'P', 'O', 'L', 'K', 'E', 'M', '1', 0
    };

    memset(object, 0, POLARKEM_HEADER_BYTES);
    memcpy(object, magic, sizeof(magic));
    object[8] = (unsigned char)POLARKEM_INSTANCE_ID;
}

/**
 * Compute SM3 over a complete ciphertext.
 *
 * @param[in]  ct     POLARKEM_CT_BYTES input bytes.
 * @param[out] digest 32 digest bytes.
 * @return 0 on success, or -4 if official SM3 reports an error.
 */
static int polarkem_hash_ciphertext(
    const unsigned char ct[POLARKEM_CT_BYTES],
    unsigned char digest[32])
{
    if (sm3hash(256,
                ct,
                (unsigned long long)POLARKEM_CT_BYTES * 8ull,
                digest) != 0) {
        memset(digest, 0, 32u);
        return -4;
    }
    return 0;
}

/**
 * Derive the deterministic two-bit-per-coordinate error vector.
 *
 * @param[in]  mu      POLARKEM_MESSAGE_BYTES message bytes.
 * @param[in]  pk_hash 32-byte SM3 public-key digest.
 * @param[out] error   POLARKEM_N signed values, each in {-1,0,+1}.
 * @return 0 on success, or -4 if pseudoXOF reports an error.
 */
static int polarkem_error_vector(
    const unsigned char mu[POLARKEM_MESSAGE_BYTES],
    const unsigned char pk_hash[32],
    int8_t error[POLARKEM_N])
{
    static const unsigned char domain[] = "PolarKEM-ERR-v1";
    unsigned char input[(sizeof(domain) - 1u) +
                        POLARKEM_MESSAGE_BYTES + 32u];
    unsigned char pairs[(2u * POLARKEM_N + 7u) / 8u];
    size_t offset = 0u;
    size_t i;

    memcpy(input + offset, domain, sizeof(domain) - 1u);
    offset += sizeof(domain) - 1u;
    memcpy(input + offset, mu, POLARKEM_MESSAGE_BYTES);
    offset += POLARKEM_MESSAGE_BYTES;
    memcpy(input + offset, pk_hash, 32u);

    if (pseudoXOF((unsigned long long)(2u * POLARKEM_N),
                  input,
                  (unsigned long long)sizeof(input) * 8ull,
                  pairs) != 0) {
        memset(error, 0, POLARKEM_N * sizeof(error[0]));
        polarkem_secure_clear(input, sizeof(input));
        polarkem_secure_clear(pairs, sizeof(pairs));
        return -4;
    }
    for (i = 0u; i < POLARKEM_N; ++i) {
        unsigned int two_bits =
            (unsigned int)((pairs[i >> 2] >> ((i & 3u) << 1)) & 3u);
        if (two_bits == 1u) {
            error[i] = (int8_t)1;
        } else if (two_bits == 2u) {
            error[i] = (int8_t)-1;
        } else {
            error[i] = (int8_t)0;
        }
    }
    polarkem_secure_clear(input, sizeof(input));
    polarkem_secure_clear(pairs, sizeof(pairs));
    return 0;
}

/**
 * Compute the authentication tag over domain, message, PK hash, and payload.
 *
 * @param[in]  mu      POLARKEM_MESSAGE_BYTES message bytes.
 * @param[in]  pk_hash 32-byte SM3 public-key digest.
 * @param[in]  payload POLARKEM_CT_PAYLOAD_BYTES compressed coefficient bytes.
 * @param[out] tag     POLARKEM_TAG_BYTES authentication-tag bytes.
 * @return 0 on success, or -4 if official SM3 reports an error.
 */
static int polarkem_ciphertext_tag(
    const unsigned char mu[POLARKEM_MESSAGE_BYTES],
    const unsigned char pk_hash[32],
    const unsigned char payload[POLARKEM_CT_PAYLOAD_BYTES],
    unsigned char tag[POLARKEM_TAG_BYTES])
{
    static const unsigned char domain[] = "PolarKEM-TAG-v1";
    unsigned char input[(sizeof(domain) - 1u) +
                        POLARKEM_MESSAGE_BYTES + 32u +
                        POLARKEM_CT_PAYLOAD_BYTES];
    size_t offset = 0u;

    memcpy(input + offset, domain, sizeof(domain) - 1u);
    offset += sizeof(domain) - 1u;
    memcpy(input + offset, mu, POLARKEM_MESSAGE_BYTES);
    offset += POLARKEM_MESSAGE_BYTES;
    memcpy(input + offset, pk_hash, 32u);
    offset += 32u;
    memcpy(input + offset, payload, POLARKEM_CT_PAYLOAD_BYTES);

    if (sm3hash(256,
                input,
                (unsigned long long)sizeof(input) * 8ull,
                tag) != 0) {
        memset(tag, 0, POLARKEM_TAG_BYTES);
        polarkem_secure_clear(input, sizeof(input));
        return -4;
    }
    polarkem_secure_clear(input, sizeof(input));
    return 0;
}

/**
 * Fill deterministic ciphertext padding from the message and tag.
 *
 * @param[in]  mu      POLARKEM_MESSAGE_BYTES message bytes.
 * @param[in]  tag     POLARKEM_TAG_BYTES authentication-tag bytes.
 * @param[out] padding POLARKEM_CT_PAD_BYTES initialized padding bytes.
 * @return 0 on success, or -4 if pseudoXOF reports an error.
 */
static int polarkem_ciphertext_padding(
    const unsigned char mu[POLARKEM_MESSAGE_BYTES],
    const unsigned char tag[POLARKEM_TAG_BYTES],
    unsigned char padding[POLARKEM_CT_PAD_BYTES])
{
    static const unsigned char domain[] = "PolarKEM-PAD-v1";
    unsigned char input[(sizeof(domain) - 1u) +
                        POLARKEM_MESSAGE_BYTES + POLARKEM_TAG_BYTES];
    size_t offset = 0u;

    memcpy(input + offset, domain, sizeof(domain) - 1u);
    offset += sizeof(domain) - 1u;
    memcpy(input + offset, mu, POLARKEM_MESSAGE_BYTES);
    offset += POLARKEM_MESSAGE_BYTES;
    memcpy(input + offset, tag, POLARKEM_TAG_BYTES);

    if (pseudoXOF((unsigned long long)POLARKEM_CT_PAD_BYTES * 8ull,
                  input,
                  (unsigned long long)sizeof(input) * 8ull,
                  padding) != 0) {
        memset(padding, 0, POLARKEM_CT_PAD_BYTES);
        polarkem_secure_clear(input, sizeof(input));
        return -4;
    }
    polarkem_secure_clear(input, sizeof(input));
    return 0;
}

int polarkem_hash_public_key(
    const unsigned char pk[POLARKEM_PK_BYTES],
    unsigned char digest[32])
{
    if (sm3hash(256,
                pk,
                (unsigned long long)POLARKEM_PK_BYTES * 8ull,
                digest) != 0) {
        memset(digest, 0, 32u);
        return -4;
    }
    return 0;
}

int polarkem_serialize_public_key(
    const unsigned char seed[POLARKEM_SEED_BYTES],
    unsigned char pk[POLARKEM_PK_BYTES])
{
    static const unsigned char domain[] = "PolarKEM-PKPAD-v1";
    unsigned char input[(sizeof(domain) - 1u) + POLARKEM_SEED_BYTES];

    memset(pk, 0, POLARKEM_PK_BYTES);
    polarkem_write_header(pk);
    memcpy(pk + POLARKEM_PK_SEED_OFFSET, seed, POLARKEM_SEED_BYTES);
    memcpy(input, domain, sizeof(domain) - 1u);
    memcpy(input + sizeof(domain) - 1u, seed, POLARKEM_SEED_BYTES);
    if (pseudoXOF((unsigned long long)POLARKEM_PK_PAD_BYTES * 8ull,
                  input,
                  (unsigned long long)sizeof(input) * 8ull,
                  pk + POLARKEM_PK_PAD_OFFSET) != 0) {
        memset(pk, 0, POLARKEM_PK_BYTES);
        return -4;
    }
    return 0;
}

int polarkem_serialize_secret_key(
    const unsigned char z[POLARKEM_SEED_BYTES],
    const unsigned char pk[POLARKEM_PK_BYTES],
    unsigned char sk[POLARKEM_SK_BYTES])
{
    static const unsigned char domain[] = "PolarKEM-SKPAD-v1";
    unsigned char pk_hash[32];
    unsigned char input[(sizeof(domain) - 1u) +
                        POLARKEM_SEED_BYTES + 32u];
    size_t offset = 0u;
    int status;

    if (polarkem_hash_public_key(pk, pk_hash) != 0) {
        memset(sk, 0, POLARKEM_SK_BYTES);
        return -4;
    }
    memset(sk, 0, POLARKEM_SK_BYTES);
    polarkem_write_header(sk);
    memcpy(sk + POLARKEM_SK_Z_OFFSET, z, POLARKEM_SEED_BYTES);
    memcpy(sk + POLARKEM_SK_PK_OFFSET, pk, POLARKEM_PK_BYTES);

    memcpy(input + offset, domain, sizeof(domain) - 1u);
    offset += sizeof(domain) - 1u;
    memcpy(input + offset, z, POLARKEM_SEED_BYTES);
    offset += POLARKEM_SEED_BYTES;
    memcpy(input + offset, pk_hash, 32u);
    status = pseudoXOF((unsigned long long)POLARKEM_SK_PAD_BYTES * 8ull,
                       input,
                       (unsigned long long)sizeof(input) * 8ull,
                       sk + POLARKEM_SK_PAD_OFFSET);
    polarkem_secure_clear(pk_hash, sizeof(pk_hash));
    polarkem_secure_clear(input, sizeof(input));
    if (status != 0) {
        memset(sk, 0, POLARKEM_SK_BYTES);
        return -4;
    }
    return 0;
}

int polarkem_validate_public_key(
    const unsigned char pk[POLARKEM_PK_BYTES])
{
    unsigned char expected[POLARKEM_PK_BYTES];
    int status;

    if (polarkem_serialize_public_key(
            pk + POLARKEM_PK_SEED_OFFSET, expected) != 0) {
        polarkem_secure_clear(expected, sizeof(expected));
        return -4;
    }
    status = polarkem_equal_bytes(pk, expected, POLARKEM_PK_BYTES) != 0u
                 ? 0
                 : -3;
    polarkem_secure_clear(expected, sizeof(expected));
    return status;
}

int polarkem_validate_secret_key(
    const unsigned char sk[POLARKEM_SK_BYTES])
{
    const unsigned char *z = sk + POLARKEM_SK_Z_OFFSET;
    const unsigned char *pk = sk + POLARKEM_SK_PK_OFFSET;
    unsigned char expected[POLARKEM_SK_BYTES];
    int status = polarkem_validate_public_key(pk);

    memset(expected, 0, sizeof(expected));
    if (status != 0) {
        polarkem_secure_clear(expected, sizeof(expected));
        return status;
    }
    if (polarkem_serialize_secret_key(z, pk, expected) != 0) {
        polarkem_secure_clear(expected, sizeof(expected));
        return -4;
    }
    status = polarkem_equal_bytes(sk, expected, POLARKEM_SK_BYTES) != 0u
                 ? 0
                 : -3;
    polarkem_secure_clear(expected, sizeof(expected));
    return status;
}

int polarkem_build_ciphertext(
    const unsigned char pk[POLARKEM_PK_BYTES],
    const unsigned char mu[POLARKEM_MESSAGE_BYTES],
    unsigned char ct[POLARKEM_CT_BYTES])
{
    unsigned char pk_hash[32];
    unsigned char codeword[POLARKEM_N];
    uint16_t permutation[POLARKEM_N];
    int8_t sign[POLARKEM_N];
    int8_t error[POLARKEM_N];
    uint16_t coefficients[POLARKEM_N];
    size_t i;
    int status = -4;

    memset(ct, 0, POLARKEM_CT_BYTES);
    memset(pk_hash, 0, sizeof(pk_hash));
    memset(codeword, 0, sizeof(codeword));
    memset(permutation, 0, sizeof(permutation));
    memset(sign, 0, sizeof(sign));
    memset(error, 0, sizeof(error));
    memset(coefficients, 0, sizeof(coefficients));
    if (polarkem_hash_public_key(pk, pk_hash) != 0) {
        goto cleanup;
    }
    polarkem_polar_encode(mu, codeword);
    if (polarkem_signed_permutation(
            pk + POLARKEM_PK_SEED_OFFSET, permutation, sign) != 0) {
        goto cleanup;
    }
    if (polarkem_error_vector(mu, pk_hash, error) != 0) {
        goto cleanup;
    }
    for (i = 0u; i < POLARKEM_N; ++i) {
        int32_t source_value = codeword[permutation[i]] != 0u
                                   ? -(int32_t)(POLARKEM_Q / 4u)
                                   : (int32_t)(POLARKEM_Q / 4u);
        int32_t value = (int32_t)sign[i] * source_value + error[i];
        value %= (int32_t)POLARKEM_Q;
        if (value < 0) {
            value += (int32_t)POLARKEM_Q;
        }
        coefficients[i] = (uint16_t)value;
    }

    polarkem_write_header(ct);
    polarkem_pack_coefficients(
        coefficients, ct + POLARKEM_CT_PAYLOAD_OFFSET);
    if (polarkem_ciphertext_tag(
            mu,
            pk_hash,
            ct + POLARKEM_CT_PAYLOAD_OFFSET,
            ct + POLARKEM_CT_TAG_OFFSET) != 0) {
        goto cleanup;
    }
    if (polarkem_ciphertext_padding(
            mu,
            ct + POLARKEM_CT_TAG_OFFSET,
            ct + POLARKEM_CT_PAD_OFFSET) != 0) {
        goto cleanup;
    }
    status = 0;

cleanup:
    if (status != 0) {
        memset(ct, 0, POLARKEM_CT_BYTES);
    }
    polarkem_secure_clear(pk_hash, sizeof(pk_hash));
    polarkem_secure_clear(codeword, sizeof(codeword));
    polarkem_secure_clear(permutation, sizeof(permutation));
    polarkem_secure_clear(sign, sizeof(sign));
    polarkem_secure_clear(error, sizeof(error));
    polarkem_secure_clear(coefficients, sizeof(coefficients));
    return status;
}

int polarkem_recover_message(
    const unsigned char pk[POLARKEM_PK_BYTES],
    const unsigned char ct[POLARKEM_CT_BYTES],
    unsigned char mu[POLARKEM_MESSAGE_BYTES])
{
    uint16_t coefficients[POLARKEM_N];
    uint16_t permutation[POLARKEM_N];
    int8_t sign[POLARKEM_N];
    unsigned char codeword[POLARKEM_N];
    size_t i;
    int status = -4;

    memset(coefficients, 0, sizeof(coefficients));
    memset(permutation, 0, sizeof(permutation));
    memset(sign, 0, sizeof(sign));
    memset(codeword, 0, sizeof(codeword));
    polarkem_unpack_coefficients(
        ct + POLARKEM_CT_PAYLOAD_OFFSET, coefficients);
    if (polarkem_signed_permutation(
            pk + POLARKEM_PK_SEED_OFFSET, permutation, sign) != 0) {
        memset(mu, 0, POLARKEM_MESSAGE_BYTES);
        goto cleanup;
    }
    for (i = 0u; i < POLARKEM_N; ++i) {
        int32_t centered = coefficients[i] > (uint16_t)(POLARKEM_Q / 2u)
                               ? (int32_t)coefficients[i] - (int32_t)POLARKEM_Q
                               : (int32_t)coefficients[i];
        int32_t source_value = (int32_t)sign[i] * centered;
        codeword[permutation[i]] =
            source_value < 0 ? (unsigned char)1 : (unsigned char)0;
    }
    polarkem_polar_decode(codeword, mu);
    status = 0;

cleanup:
    polarkem_secure_clear(coefficients, sizeof(coefficients));
    polarkem_secure_clear(permutation, sizeof(permutation));
    polarkem_secure_clear(sign, sizeof(sign));
    polarkem_secure_clear(codeword, sizeof(codeword));
    return status;
}

int polarkem_derive_valid_secret(
    const unsigned char mu[POLARKEM_MESSAGE_BYTES],
    const unsigned char ct[POLARKEM_CT_BYTES],
    unsigned char ss[POLARKEM_SS_BYTES])
{
    static const unsigned char domain[] = "PolarKEM-SS-v1";
    unsigned char ct_hash[32];
    unsigned char input[(sizeof(domain) - 1u) +
                        POLARKEM_MESSAGE_BYTES + 32u];
    size_t offset = 0u;
    int status;

    if (polarkem_hash_ciphertext(ct, ct_hash) != 0) {
        memset(ss, 0, POLARKEM_SS_BYTES);
        return -4;
    }
    memcpy(input + offset, domain, sizeof(domain) - 1u);
    offset += sizeof(domain) - 1u;
    memcpy(input + offset, mu, POLARKEM_MESSAGE_BYTES);
    offset += POLARKEM_MESSAGE_BYTES;
    memcpy(input + offset, ct_hash, 32u);
    status = pseudoXOF((unsigned long long)POLARKEM_SS_BYTES * 8ull,
                       input,
                       (unsigned long long)sizeof(input) * 8ull,
                       ss);
    polarkem_secure_clear(ct_hash, sizeof(ct_hash));
    polarkem_secure_clear(input, sizeof(input));
    if (status != 0) {
        memset(ss, 0, POLARKEM_SS_BYTES);
        return -4;
    }
    return 0;
}

int polarkem_derive_reject_secret(
    const unsigned char z[POLARKEM_SEED_BYTES],
    const unsigned char ct[POLARKEM_CT_BYTES],
    unsigned char ss[POLARKEM_SS_BYTES])
{
    static const unsigned char domain[] = "PolarKEM-REJ-v1";
    unsigned char ct_hash[32];
    unsigned char input[(sizeof(domain) - 1u) +
                        POLARKEM_SEED_BYTES + 32u];
    size_t offset = 0u;
    int status;

    if (polarkem_hash_ciphertext(ct, ct_hash) != 0) {
        memset(ss, 0, POLARKEM_SS_BYTES);
        return -4;
    }
    memcpy(input + offset, domain, sizeof(domain) - 1u);
    offset += sizeof(domain) - 1u;
    memcpy(input + offset, z, POLARKEM_SEED_BYTES);
    offset += POLARKEM_SEED_BYTES;
    memcpy(input + offset, ct_hash, 32u);
    status = pseudoXOF((unsigned long long)POLARKEM_SS_BYTES * 8ull,
                       input,
                       (unsigned long long)sizeof(input) * 8ull,
                       ss);
    polarkem_secure_clear(ct_hash, sizeof(ct_hash));
    polarkem_secure_clear(input, sizeof(input));
    if (status != 0) {
        memset(ss, 0, POLARKEM_SS_BYTES);
        return -4;
    }
    return 0;
}

unsigned char polarkem_ciphertext_equal(
    const unsigned char left[POLARKEM_CT_BYTES],
    const unsigned char right[POLARKEM_CT_BYTES])
{
    return polarkem_equal_bytes(left, right, POLARKEM_CT_BYTES);
}
