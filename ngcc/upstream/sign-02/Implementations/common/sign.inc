/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#include "ct_audit.h"
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "polyvec.h"
#include "drng.h"
#include "packing.h"
#include "sample.h"
#include "sign.h"
#include "symmetric.h"
#include "spectral.h"

extern DRNG_ctx drng_algorithm;

int bit_sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes, unsigned char *sk,
                   unsigned long long *sk_len_bytes)
{
    unsigned char keygen_seedbuf[2 * BIT_SEEDBYTES];
    unsigned char *seed_A = keygen_seedbuf;
    unsigned char *secret_seed = keygen_seedbuf + BIT_SEEDBYTES;
    unsigned char tr[BIT_TRBYTES];
    poly_matrix_ntt A_0_ntt;
    polyvecl_ntt s_0_ntt;
    polyvecl s_0;
    polyveck e, public_vector;

    int status = -1;
    uint16_t nonce = 0;
    unsigned attempt;

    if (pk == NULL || sk == NULL || pk_len_bytes == NULL || sk_len_bytes == NULL)
        return -1;
    *pk_len_bytes = 0;
    *sk_len_bytes = 0;

    if (get_random_number(&drng_algorithm, keygen_seedbuf,
                          (unsigned long long)sizeof(keygen_seedbuf) * 8ULL) != 0) {
        bit_secure_zero(keygen_seedbuf, sizeof(keygen_seedbuf));
        return -1;
    }
    for (attempt = 0; attempt < BIT_KEYGEN_MAX_ITERS; attempt++) {
        if (attempt != 0 && get_random_number(&drng_algorithm, secret_seed,
                                             BIT_SEEDBYTES * 8ULL) != 0)
            goto cleanup;
        nonce = 0;
#if BIT_IMPL_AVX && BIT_N != 1024
        if (polyvec_sample_S1_keypair(&s_0, &e, secret_seed, &nonce) != 0)
            goto cleanup;
#else
        if (polyvecl_sample_S1(&s_0, secret_seed, &nonce) != 0 ||
            polyveck_sample_S1(&e, secret_seed, &nonce) != 0)
            goto cleanup;
#endif
        int reject = bit_keygen_spectral_bound_sq(&s_0, &e) > BIT_PARABOLA_BETA2;
        BIT_CT_DECLASSIFY(&reject, sizeof(reject));
        if (!reject)
            break;
    }
    if (attempt == BIT_KEYGEN_MAX_ITERS)
        goto cleanup;

    if (poly_matrix_expand_ntt(&A_0_ntt, seed_A) != 0) {
        goto cleanup;
    }
    polyvecl_to_ntt(&s_0_ntt, &s_0);
    poly_matrix_mul_vector_ntt(&public_vector, &A_0_ntt, &s_0_ntt);
    polyveck_add_small(&public_vector, &public_vector, &e);
    polyveck_canonicalize(&public_vector);

    pack_pk(pk, seed_A, &public_vector);
    if (bit_hash(tr, pk, BIT_PUBLICKEYBYTES) != 0) {
        goto cleanup;
    }
    pack_sk(sk, seed_A, &public_vector, secret_seed, tr, &s_0, &e);

    *pk_len_bytes = BIT_PUBLICKEYBYTES;
    *sk_len_bytes = BIT_SECRETKEYBYTES;

    status = 0;
cleanup:
    bit_secure_zero(keygen_seedbuf, sizeof(keygen_seedbuf));
    bit_secure_zero(&s_0_ntt, sizeof(s_0_ntt));
    bit_secure_zero(&s_0, sizeof(s_0));
    bit_secure_zero(&e, sizeof(e));
    return status;
}

int bit_sig_sign(unsigned char *sk, unsigned long long sk_len_bytes, const unsigned char *m,
                 unsigned long long mlen, unsigned char *sig, unsigned long long *sig_len_bytes)
{
    unsigned char seed_A[BIT_SEEDBYTES];
    unsigned char challenge[BIT_CHALLENGEBYTES];
    unsigned char message_hash[BIT_MESSAGEBYTES];
    unsigned char secret_seed[BIT_SEEDBYTES];
    unsigned char tr[BIT_TRBYTES];
    unsigned char b_batch[32];
    size_t b_bit_pos = sizeof(b_batch) * 8U;
    unsigned char rnd[BIT_SEEDBYTES];
    unsigned char temp_seed_y[BIT_SEEDBYTES];
#if BIT_SECURITY_BITS == 512
    bit_challenge_ctx challenge_ctx = {0};
    unsigned char packed_w1[BIT_POLYVECK_W1_BYTES];
#endif
    int status = -1;
    int rejection;

    poly_matrix_ntt A_0_ntt;
    polyvecl s_0, c_s;
    polyvecl_ntt y1_ntt;
    polyveck_ntt public_ntt;
    poly_ntt y0_ntt;
    polyveck e, public_vector;
    polyvecy y;
    polyveck w, w1;
    /* Proposal and response are disjoint signing phases. */
    union {
        struct {
            bit_parabola_fixed mask;
            polyveck shift;
        } proposal;
        struct {
            polyvecm1 response;
            polyveck tail;
            polyveck hint;
        } accepted;
    } phase;
    poly c_poly;
    sparse_challenge c_sparse;
#if BIT_IMPL_AVX
    bit_pspm_se_precomp se_pspm;
#endif

    if (sk == NULL || (m == NULL && mlen != 0) || sig == NULL || sig_len_bytes == NULL ||
        mlen > SIZE_MAX - BIT_TRBYTES || sk_len_bytes != BIT_SECRETKEYBYTES) {
        return -1;
    }
    *sig_len_bytes = 0;
    if (m == NULL)
        m = (const unsigned char *)"";

    if (unpack_sk(seed_A, &public_vector, secret_seed, tr, &s_0, &e, sk) != 0) {
        goto cleanup;
    }

    if (poly_matrix_expand_ntt(&A_0_ntt, seed_A) != 0) {
        goto cleanup;
    }
    polyveck_public_to_ntt(&public_ntt, &public_vector);

    if (bit_hash_message(message_hash, tr, BIT_TRBYTES, m, (size_t)mlen) != 0) {
        goto cleanup;
    }
#if BIT_SECURITY_BITS == 512
    if (bit_challenge_init(&challenge_ctx, tr, m, (size_t)mlen) != 0)
        goto cleanup;
#else
    unsigned char hash_input[BIT_MESSAGEBYTES + BIT_POLYVECK_W1_BYTES];
    memcpy(hash_input, message_hash, BIT_MESSAGEBYTES);
#endif

    if (get_random_number(&drng_algorithm, rnd, BIT_SEEDBYTES * 8ULL) != 0) {
        goto cleanup;
    }
    if (bit_xof256_3(temp_seed_y, BIT_SEEDBYTES, secret_seed, BIT_SEEDBYTES, rnd, BIT_SEEDBYTES,
                     message_hash, BIT_MESSAGEBYTES) != 0)
        goto cleanup;

#if BIT_IMPL_AVX
    polyvec_prepare_challenge_mul_se(&se_pspm, &s_0, &e);
#endif

    uint16_t nonce_y = 0;
    if (polyvecy_sample_mask_q13(&y, &phase.proposal.mask, temp_seed_y, &nonce_y) != 0)
        goto cleanup;

    uint32_t sign_iter = 0;
    while (1) {
        if (++sign_iter > BIT_SIGN_MAX_ITERS)
            goto cleanup;
        polyvecy_y1_to_ntt(&y1_ntt, &y);
        poly_to_ntt(&y0_ntt, &y.vec[0]);
        polyveck_compute_w_ntt(&w, &A_0_ntt, &public_ntt, &y1_ntt, &y0_ntt, &y);

        polyveck_highbits(&w1, &w);
#if BIT_SECURITY_BITS == 512
        polyveck_pack_w1(packed_w1, &w1);
        if (bit_challenge_hash(challenge, &challenge_ctx, packed_w1) != 0)
            goto cleanup;
#else
        polyveck_pack_w1(hash_input + BIT_MESSAGEBYTES, &w1);
        if (bit_hash(challenge, hash_input, sizeof(hash_input)) != 0) {
            goto cleanup;
        }
#endif

        if (b_bit_pos == sizeof(b_batch) * 8U) {
            if (get_random_number(&drng_algorithm, b_batch,
                                  (unsigned long long)sizeof(b_batch) * 8ULL) != 0) {
                goto cleanup;
            }
            b_bit_pos = 0;
        }
        uint8_t b_val = (uint8_t)((b_batch[b_bit_pos >> 3] >> (b_bit_pos & 7U)) & 1U);
        b_bit_pos++;

        if (poly_challenge(&c_poly, challenge) != 0) {
            goto cleanup;
        }
        poly_challenge_to_sparse(&c_sparse, &c_poly);
        c_sparse.bflip = b_val;
        poly_cneg(&c_poly, b_val);

        /* Apply one global sign to every block of (c, c*s0, c*e). */
#if BIT_IMPL_AVX
        polyvec_mul_challenge_no_reduce_se_pspm(&c_s, &phase.proposal.shift, &se_pspm, &c_sparse);
#else
        polyvecl_mul_challenge_no_reduce(&c_s, &s_0, &c_sparse);
        polyveck_mul_challenge_no_reduce(&phase.proposal.shift, &e, &c_sparse);
#endif
        poly_add_no_reduce(&y.vec[0], &y.vec[0], &c_poly);

        for (int i = 0; i < BIT_L; i++) {
            poly_add_no_reduce(&y.vec[1 + i], &y.vec[1 + i], &c_s.vec[i]);
        }

        for (int i = 0; i < BIT_K; i++) {
            poly_add_no_reduce(&y.vec[1 + BIT_L + i], &y.vec[1 + BIT_L + i],
                               &phase.proposal.shift.vec[i]);
        }

        rejection = bit_parabola_reject_response(&c_poly, &c_s, &phase.proposal.shift,
                                                 &phase.proposal.mask, temp_seed_y, &nonce_y);
        if (rejection < 0)
            goto cleanup;
        if (rejection != 0) {
            goto retry;
        }

        /* Reject if the signed challenge changes HighBits in the first block. */
        if (poly_reject_highbits_bimodal(&w1.vec[0], &w.vec[0], &c_sparse)) {
            goto retry;
        }

        for (int i = 0; i < BIT_L + 1; i++) {
            phase.accepted.response.vec[i] = y.vec[i];
        }
        for (int i = 0; i < BIT_K; i++) {
            phase.accepted.tail.vec[i] = y.vec[1 + BIT_L + i];
        }

        polyveck_make_hint(&phase.accepted.hint, &w1, &w, &phase.accepted.tail,
                          &c_poly, b_val);

        /* TIMECOP assumes these candidates public, including later rejections. */
        BIT_CT_HAETAE_PUBLIC(&phase.accepted.response, sizeof phase.accepted.response);
        BIT_CT_HAETAE_PUBLIC(&phase.accepted.hint, sizeof phase.accepted.hint);

        if (check_signature_bounds(&phase.accepted.response, &phase.accepted.hint)) {
            goto retry;
        }

        /* Reject unsupported tails and oversized entropy streams. */
        if (pack_sig(sig, &phase.accepted.response, &phase.accepted.hint, challenge) != 0) {
            goto retry;
        }

        break;
    retry:;
        if (polyvecy_sample_mask_q13(&y, &phase.proposal.mask, temp_seed_y, &nonce_y) != 0)
            goto cleanup;
    }
    status = 0;
    *sig_len_bytes = BIT_SIGNBYTES;
cleanup:;
#if BIT_SECURITY_BITS == 512
    bit_challenge_release(&challenge_ctx);
#endif
    bit_secure_zero(temp_seed_y, sizeof(temp_seed_y));
    bit_secure_zero(secret_seed, sizeof(secret_seed));
    bit_secure_zero(rnd, sizeof(rnd));
    bit_secure_zero(b_batch, sizeof(b_batch));
    bit_secure_zero(&s_0, sizeof(s_0));
    bit_secure_zero(&e, sizeof(e));
    bit_secure_zero(&c_s, sizeof(c_s));
    bit_secure_zero(&y, sizeof(y));
    bit_secure_zero(&phase, sizeof(phase));
#if BIT_IMPL_AVX
    bit_secure_zero(&se_pspm, sizeof(se_pspm));
#endif

    bit_secure_zero(&y1_ntt, sizeof(y1_ntt));
    bit_secure_zero(&y0_ntt, sizeof(y0_ntt));
    bit_secure_zero(&w, sizeof(w));
    bit_secure_zero(&w1, sizeof(w1));
    bit_secure_zero(&c_poly, sizeof(c_poly));
    bit_secure_zero(&c_sparse, sizeof(c_sparse));
    return status;
}

int bit_sig_verify(const unsigned char *sig, unsigned long long siglen, const unsigned char *m,
                   unsigned long long mlen, const unsigned char *pk,
                   unsigned long long pk_len_bytes)
{
    unsigned char seed_A[BIT_SEEDBYTES];
    unsigned char challenge[BIT_CHALLENGEBYTES];
    unsigned char c_prime[BIT_CHALLENGEBYTES];
#if BIT_SECURITY_BITS != 512
    unsigned char message_hash[BIT_MESSAGEBYTES];
#endif

    unsigned char tr[BIT_TRBYTES];
    poly_matrix_ntt A_0_ntt;
    polyveck public_vector;
    polyveck_ntt public_ntt;
    polyvecl_ntt z1_tail_ntt;
    poly_ntt z0_ntt;
    polyvecm1 z1;
    polyveck h;
    poly c_poly;

    polyveck w_hat_prime;

    if (sig == NULL || (m == NULL && mlen != 0) || pk == NULL || mlen > SIZE_MAX - BIT_TRBYTES ||
        siglen != BIT_SIGNBYTES || pk_len_bytes != BIT_PUBLICKEYBYTES) {
        return -1;
    }
    if (m == NULL)
        m = (const unsigned char *)"";

    if (unpack_pk(seed_A, &public_vector, pk) != 0)
        return -1;

    if (unpack_sig(&z1, &h, challenge, sig) != 0)
        return -1;

    if (check_signature_verification_bounds(&z1, &h)) {
        return -1;
    }

    for (int i = 0; i < BIT_L; i++)
        poly_to_ntt_centered(&z1_tail_ntt.vec[i], &z1.vec[i + 1]);
    poly_to_ntt_centered(&z0_ntt, &z1.vec[0]);

    if (bit_hash(tr, pk, BIT_PUBLICKEYBYTES) != 0) {
        return -1;
    }
#if BIT_SECURITY_BITS != 512
    if (bit_hash_message(message_hash, tr, BIT_TRBYTES, m, (size_t)mlen) != 0)
        return -1;
#endif
    if (poly_matrix_expand_ntt(&A_0_ntt, seed_A) != 0) {
        return -1;
    }
    polyveck_public_to_ntt(&public_ntt, &public_vector);
    if (poly_challenge(&c_poly, challenge) != 0) {
        return -1;
    }

    polyveck_compute_w_hat_prime_ntt(&w_hat_prime, &A_0_ntt, &public_ntt, &z1_tail_ntt, &z0_ntt,
                                     &z1.vec[0], &c_poly);

    polyveck_compute_w_prime(&w_hat_prime, &w_hat_prime, &h);

#if BIT_SECURITY_BITS == 512
    unsigned char packed_w1[BIT_POLYVECK_W1_BYTES];
    bit_challenge_ctx challenge_ctx = {0};
    polyveck_pack_w1(packed_w1, &w_hat_prime);
    if (bit_challenge_init(&challenge_ctx, tr, m, (size_t)mlen) != 0)
        return -1;
    int hash_status = bit_challenge_hash(c_prime, &challenge_ctx, packed_w1);
    bit_challenge_release(&challenge_ctx);
    if (hash_status != 0)
        return -1;
#else
    unsigned char hash_input[BIT_MESSAGEBYTES + BIT_POLYVECK_W1_BYTES];
    memcpy(hash_input, message_hash, BIT_MESSAGEBYTES);
    polyveck_pack_w1(hash_input + BIT_MESSAGEBYTES, &w_hat_prime);

    if (bit_hash(c_prime, hash_input, sizeof(hash_input)) != 0) {
        return -1;
    }
#endif
    if (memcmp(challenge, c_prime, BIT_CHALLENGEBYTES) != 0) {
        return -1;
    }

    return 0;
}
