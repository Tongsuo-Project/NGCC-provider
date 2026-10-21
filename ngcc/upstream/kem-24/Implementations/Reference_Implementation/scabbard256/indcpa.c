#include <stddef.h>
#include <stdint.h>

#include "indcpa.h"
#include "drng.h"
#include "auxfunc.h"
#include "polyvec.h"
#include "poly.h"
#include "packing.h"

extern DRNG_ctx drng_algorithm;

void indcpa_keypair(
    uint8_t pk[SCABBARD_INDCPA_PUBLICKEYBYTES], 
    uint8_t sk[SCABBARD_INDCPA_SECRETKEYBYTES])
{
    size_t i, j;
    uint8_t buf[2 * SCABBARD_SYMBYTES];
    const uint8_t *publicseed = buf;
    const uint8_t *noiseseed = buf + SCABBARD_SYMBYTES;
    polyvec a[SCABBARD_L], pkpv, skpv;
    
    get_random_number(&drng_algorithm, buf, SCABBARD_SYMBYTES * 8);
    pseudoXOF(SCABBARD_SYMBYTES * 8, buf, SCABBARD_SYMBYTES * 8, buf); /* for not revealing RNG state when publicseed is revealed */
    get_random_number(&drng_algorithm, buf + SCABBARD_SYMBYTES, SCABBARD_SYMBYTES * 8);

    gen_a(a, publicseed);
    gen_s(&skpv, noiseseed);

    matrix_vector_mul(&pkpv, a, &skpv, 1);
    /* rounding */
    for (i = 0; i < SCABBARD_L; i++) {
        for (j = 0; j < SCABBARD_N; j++) {
            pkpv.vec[i].coeffs[j] = (pkpv.vec[i].coeffs[j] + h1) & SCABBARD_Q;
            pkpv.vec[i].coeffs[j] = (pkpv.vec[i].coeffs[j] >> (SCABBARD_EQ - SCABBARD_EP));
        }
    }
    
    pack_sk(sk, &skpv);
    pack_pk(pk, &pkpv, publicseed);
}

void indcpa_enc(
    uint8_t ct[SCABBARD_INDCPA_BYTES], 
    const uint8_t msg[SCABBARD_INDCPA_MSGBYTES], 
    const uint8_t pk[SCABBARD_INDCPA_PUBLICKEYBYTES], 
    const uint8_t r[SCABBARD_SYMBYTES])
{
    size_t i, j;

    uint8_t seed[SCABBARD_SYMBYTES];
    poly v, m;
    polyvec a[SCABBARD_L], pkpv, skpv, u;
    
    unpack_pk(&pkpv, seed, pk);
    poly_frommsg(&m, msg);
    
    gen_a(a, seed);
    gen_s(&skpv, r);

    matrix_vector_mul(&u, a, &skpv, 0);

    /* rounding */
    for (i = 0; i < SCABBARD_L; i++) {
        for (j = 0; j < SCABBARD_N; j++) {
            u.vec[i].coeffs[j] = (u.vec[i].coeffs[j]) & SCABBARD_Q;
            u.vec[i].coeffs[j] = (u.vec[i].coeffs[j] >> (SCABBARD_EQ - SCABBARD_EP));
        }
    }

    inner_prod(&v, &pkpv, &skpv);

    // /* encode message & rounding */
    for (i = 0; i < SCABBARD_N; i++) {
        v.coeffs[i] = (v.coeffs[i]) & SCABBARD_P;
        v.coeffs[i] <<= (SCABBARD_EQ - SCABBARD_EP);
        v.coeffs[i] = ((v.coeffs[i] - m.coeffs[i]) & SCABBARD_Q) >> (SCABBARD_EQ - (SCABBARD_ET + SCABBARD_B));
    }
    pack_ciphertext(ct, &u, &v);
}
void indcpa_dec(
    uint8_t msg[SCABBARD_INDCPA_MSGBYTES], 
    const uint8_t ct[SCABBARD_INDCPA_BYTES], 
    const uint8_t sk[SCABBARD_INDCPA_SECRETKEYBYTES])
{
    size_t i;
    polyvec skpv, u;
    poly v, v_prime;

    unpack_ciphertext(&u, &v, ct);
    unpack_sk(&skpv, sk);
    
    inner_prod(&v_prime, &u, &skpv);

    /* message decoding */
    for (i = 0; i < SCABBARD_N; i++) {
        v.coeffs[i] = (v.coeffs[i] & SCABBARD_Q) << (SCABBARD_EQ - (SCABBARD_ET + SCABBARD_B));
        v_prime.coeffs[i] = ((v_prime.coeffs[i]) & SCABBARD_P) << (SCABBARD_EQ - SCABBARD_EP);
        v_prime.coeffs[i] = (v_prime.coeffs[i] - v.coeffs[i]) & SCABBARD_Q;
    }

    poly_tomsg(msg, &v_prime);
}