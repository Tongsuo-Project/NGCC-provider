#include "polarkem_core.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "polarkem_ct.h"

int polarkem_keygen(
    unsigned char pk[POLARKEM_PK_BYTES],
    unsigned char sk[POLARKEM_SK_BYTES],
    DRNG_ctx *drng)
{
    unsigned char seed[POLARKEM_SEED_BYTES];
    unsigned char z[POLARKEM_SEED_BYTES];
    int status = POLARKEM_ERROR_EXTERNAL;

    memset(seed, 0, sizeof(seed));
    memset(z, 0, sizeof(z));
    if (drng == NULL) {
        goto cleanup;
    }
    if (get_random_number(
            drng, seed, (unsigned long long)POLARKEM_SEED_BYTES * 8ull) != 0 ||
        get_random_number(
            drng, z, (unsigned long long)POLARKEM_SEED_BYTES * 8ull) != 0) {
        goto cleanup;
    }
    if (polarkem_serialize_public_key(seed, pk) != 0 ||
        polarkem_serialize_secret_key(z, pk, sk) != 0) {
        goto cleanup;
    }
    status = POLARKEM_SUCCESS;

cleanup:
    polarkem_secure_clear(seed, sizeof(seed));
    polarkem_secure_clear(z, sizeof(z));
    if (status != POLARKEM_SUCCESS) {
        memset(pk, 0, POLARKEM_PK_BYTES);
        memset(sk, 0, POLARKEM_SK_BYTES);
    }
    return status;
}

int polarkem_enc(
    const unsigned char pk[POLARKEM_PK_BYTES],
    unsigned char ss[POLARKEM_SS_BYTES],
    unsigned char ct[POLARKEM_CT_BYTES],
    DRNG_ctx *drng)
{
    unsigned char mu[POLARKEM_MESSAGE_BYTES];

    memset(mu, 0, sizeof(mu));
    if (drng == NULL ||
        get_random_number(
            drng,
            mu,
            (unsigned long long)POLARKEM_MESSAGE_BITS) != 0) {
        memset(ss, 0, POLARKEM_SS_BYTES);
        memset(ct, 0, POLARKEM_CT_BYTES);
        polarkem_secure_clear(mu, sizeof(mu));
        return POLARKEM_ERROR_EXTERNAL;
    }
    if (polarkem_build_ciphertext(pk, mu, ct) != 0 ||
        polarkem_derive_valid_secret(mu, ct, ss) != 0) {
        memset(ss, 0, POLARKEM_SS_BYTES);
        memset(ct, 0, POLARKEM_CT_BYTES);
        polarkem_secure_clear(mu, sizeof(mu));
        return POLARKEM_ERROR_EXTERNAL;
    }
    polarkem_secure_clear(mu, sizeof(mu));
    return POLARKEM_SUCCESS;
}

int polarkem_dec(
    const unsigned char sk[POLARKEM_SK_BYTES],
    const unsigned char ct[POLARKEM_CT_BYTES],
    unsigned char ss[POLARKEM_SS_BYTES])
{
    const unsigned char *z = sk + POLARKEM_SK_Z_OFFSET;
    const unsigned char *pk = sk + POLARKEM_SK_PK_OFFSET;
    unsigned char mu[POLARKEM_MESSAGE_BYTES];
    unsigned char expected_ct[POLARKEM_CT_BYTES];
    unsigned char valid_ss[POLARKEM_SS_BYTES];
    unsigned char reject_ss[POLARKEM_SS_BYTES];
    unsigned char valid;
    unsigned char mask;
    size_t i;
    int status;

    memset(mu, 0, sizeof(mu));
    memset(expected_ct, 0, sizeof(expected_ct));
    memset(valid_ss, 0, sizeof(valid_ss));
    memset(reject_ss, 0, sizeof(reject_ss));

    if (polarkem_recover_message(pk, ct, mu) != 0 ||
        polarkem_build_ciphertext(pk, mu, expected_ct) != 0 ||
        polarkem_derive_valid_secret(mu, ct, valid_ss) != 0 ||
        polarkem_derive_reject_secret(z, ct, reject_ss) != 0) {
        memset(ss, 0, POLARKEM_SS_BYTES);
        polarkem_secure_clear(mu, sizeof(mu));
        polarkem_secure_clear(expected_ct, sizeof(expected_ct));
        polarkem_secure_clear(valid_ss, sizeof(valid_ss));
        polarkem_secure_clear(reject_ss, sizeof(reject_ss));
        return POLARKEM_ERROR_EXTERNAL;
    }

    valid = polarkem_ciphertext_equal(ct, expected_ct);
    mask = (unsigned char)(0u - (unsigned int)valid);
    for (i = 0u; i < POLARKEM_SS_BYTES; ++i) {
        ss[i] = (unsigned char)((valid_ss[i] & mask) |
                                (reject_ss[i] & (unsigned char)~mask));
    }
    status = valid != 0u ? POLARKEM_SUCCESS : POLARKEM_ERROR_CIPHERTEXT;
    polarkem_secure_clear(mu, sizeof(mu));
    polarkem_secure_clear(expected_ct, sizeof(expected_ct));
    polarkem_secure_clear(valid_ss, sizeof(valid_ss));
    polarkem_secure_clear(reject_ss, sizeof(reject_ss));
    return status;
}
