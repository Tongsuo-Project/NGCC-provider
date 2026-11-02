/* ABI adapter only. All cryptographic operations use the author's Galas API. */
#include "api.hpp"
#include "parameters.hpp"
#include <climits>

extern "C" {
#include "drng.h"
extern DRNG_ctx drng_algorithm;
}

using Params = faest::galas::NGCC_GALAS_PARAMS;
using Scheme = faest::faest_scheme<Params>;

extern "C" int rand_bytes(uint8_t *out, size_t len)
{
    if (len > ULLONG_MAX / 8) return -1;
    return get_random_number(&drng_algorithm, out, len * 8);
}

extern "C" unsigned long long sig_get_pk_len_bytes(void)
{ return Scheme::CRYPTO_PUBLICKEYBYTES; }
extern "C" unsigned long long sig_get_sk_len_bytes(void)
{ return Scheme::CRYPTO_SECRETKEYBYTES; }
extern "C" unsigned long long sig_get_sn_len_bytes(void)
{ return Scheme::CRYPTO_BYTES; }

extern "C" int sig_keygen(unsigned char *pk, unsigned long long *pklen,
                           unsigned char *sk, unsigned long long *sklen)
{
    const int result = Scheme::crypto_sign_keypair(pk, sk);
    if (result == 0) {
        *pklen = Scheme::CRYPTO_PUBLICKEYBYTES;
        *sklen = Scheme::CRYPTO_SECRETKEYBYTES;
    }
    return result;
}

extern "C" int sig_sign(unsigned char *sk, unsigned long long sklen,
                         unsigned char *message, unsigned long long message_len,
                         unsigned char *signature, unsigned long long *signature_len)
{
    if (sklen != Scheme::CRYPTO_SECRETKEYBYTES ||
        !faest::faest_sign<Params>(signature, message, message_len, sk, nullptr, 0))
        return -1;
    *signature_len = Scheme::CRYPTO_BYTES;
    return 0;
}

extern "C" int sig_verify(unsigned char *pk, unsigned long long pklen,
                           unsigned char *signature, unsigned long long signature_len,
                           unsigned char *message, unsigned long long message_len)
{
    if (pklen != Scheme::CRYPTO_PUBLICKEYBYTES || signature_len != Scheme::CRYPTO_BYTES)
        return -1;
    return faest::faest_verify<Params>(signature, message, message_len, pk) ? 0 : -1;
}
