#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "symmetric.h"
#include "fips202.h"

void weaver_hash_h(uint8_t out[WEAVER_HBYTES], const uint8_t *in, size_t inlen)
{
#if WEAVER_HBYTES == 32
  sha3_256(out, in, inlen);
#elif WEAVER_HBYTES == 64
  sha3_512(out, in, inlen);
#elif WEAVER_HBYTES == 128
  shake256(out, WEAVER_HBYTES, in, inlen);
#else
#error "Unsupported WEAVER_HBYTES"
#endif
}

void weaver_hash_g(uint8_t out[WEAVER_GBYTES], const uint8_t *in, size_t inlen)
{
  shake256(out, WEAVER_GBYTES, in, inlen);
}

void weaver_expand_keypair_seeds(uint8_t out[2 * WEAVER_SYMBYTES], const uint8_t *in, size_t inlen)
{
  shake256(out, 2 * WEAVER_SYMBYTES, in, inlen);
}

/*************************************************
* Name:        weaver_shake128_absorb
*
* Description: Absorb step of the mode-selected XOF.
*
* Arguments:   - keccak_state *state: pointer to (uninitialized) output Keccak state
*              - const uint8_t *seed: pointer to WEAVER_SYMBYTES input to be absorbed into state
*              - uint8_t i: additional byte of input
*              - uint8_t j: additional byte of input
**************************************************/
void weaver_shake128_absorb(keccak_state *state,
                           const uint8_t seed[WEAVER_SYMBYTES],
                           uint8_t x,
                           uint8_t y)
{
  uint8_t extseed[WEAVER_SYMBYTES+2];

  memcpy(extseed, seed, WEAVER_SYMBYTES);
  extseed[WEAVER_SYMBYTES+0] = x;
  extseed[WEAVER_SYMBYTES+1] = y;

#if WEAVER_MODE == 1
  shake128_absorb_once(state, extseed, sizeof(extseed));
#else
  shake256_absorb_once(state, extseed, sizeof(extseed));
#endif
}

/*************************************************
* Name:        weaver_shake256_prf
*
* Description: Usage of SHAKE256 as a PRF, concatenates secret and public input
*              and then generates outlen bytes of SHAKE256 output
*
* Arguments:   - uint8_t *out: pointer to output
*              - size_t outlen: number of requested output bytes
*              - const uint8_t *key: pointer to the key (of length WEAVER_SYMBYTES)
*              - uint8_t nonce: single-byte nonce (public PRF input)
**************************************************/
void weaver_shake256_prf(uint8_t *out, size_t outlen, const uint8_t key[WEAVER_SYMBYTES], uint8_t nonce)
{
  uint8_t extkey[WEAVER_SYMBYTES+1];

  memcpy(extkey, key, WEAVER_SYMBYTES);
  extkey[WEAVER_SYMBYTES] = nonce;

  shake256(out, outlen, extkey, sizeof(extkey));
}

/*************************************************
* Name:        weaver_shake256_rkprf
*
* Description: Usage of SHAKE256 as rejection-key PRF.
**************************************************/
void weaver_shake256_rkprf(uint8_t out[WEAVER_SSBYTES], const uint8_t key[WEAVER_SYMBYTES], const uint8_t input[WEAVER_CIPHERTEXTBYTES])
{
  keccak_state s;

  shake256_init(&s);
  shake256_absorb(&s, key, WEAVER_SYMBYTES);
  shake256_absorb(&s, input, WEAVER_CIPHERTEXTBYTES);
  shake256_finalize(&s);
  shake256_squeeze(out, WEAVER_SSBYTES, &s);
}
