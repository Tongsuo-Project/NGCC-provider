#include <string.h>
#include <stdlib.h>

#include "KEM_AlgorithmInstance.h"
#include "auxfunc.h"
#include "drng.h"

#include "trike_types.h"
#include "decoder.h"
#include "sample.h"
#include "gf2x.h"

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

#define PUBLIC_KEY_SIZE_BYTES (R_SIZE_BYTES + M_SIZE_BYTES)
#define SECRET_KEY_SIZE_BYTES \
	(4 * DIVIDE_AND_CEIL(12 * PARAM_D + 3 * R_SIZE_BYTES + 2 * M_SIZE_BYTES, 4))
#define CIPHERTEXT_SIZE_BYTES (2 * R_SIZE_BYTES + M_SIZE_BYTES + SALT_SIZE_BYTES)

// Returns the public key size in bytes.
unsigned long long kem_get_pk_len_bytes(void)
{
	return PUBLIC_KEY_SIZE_BYTES;
}

// Returns the secret key size in bytes.
unsigned long long kem_get_sk_len_bytes(void)
{
	return SECRET_KEY_SIZE_BYTES;
}

// Returns the shared secret size in bytes.
unsigned long long kem_get_ss_len_bytes(void)
{
	return M_SIZE_BYTES;
}

// Returns the ciphertext size in bytes.
unsigned long long kem_get_ct_len_bytes(void)
{
	return CIPHERTEXT_SIZE_BYTES;
}

// Serializes polynomials with unused high bits cleared.
static void encode_poly(uint8_t *out, const uint8_t *in)
{
	memcpy(out, in, R_SIZE_BYTES);
	out[R_SIZE_BYTES - 1] &= R_8_LAST_MASK;
}

// Serializes the public key without relying on structure layout.
static void encode_pk(uint8_t *out, const public_key_t *key)
{
	encode_poly(out, key->r2);
	memcpy(out + R_SIZE_BYTES, key->sigma, M_SIZE_BYTES);
}

// Reads the public-key polynomial and seed from their encoded positions.
static void decode_pk(public_key_t *key, const uint8_t *in)
{
	memcpy(key->r2, in, R_SIZE_BYTES);
	memcpy(key->sigma, in + R_SIZE_BYTES, M_SIZE_BYTES);
}

// Serializes both ciphertext polynomials, the masked message, and the salt.
static void encode_ct(uint8_t *out, const ciphertext_t *value)
{
	encode_poly(out, value->u);
	encode_poly(out + R_SIZE_BYTES, value->v);
	memcpy(out + 2 * R_SIZE_BYTES, value->c2, M_SIZE_BYTES);
	memcpy(out + 2 * R_SIZE_BYTES + M_SIZE_BYTES, value->salt, SALT_SIZE_BYTES);
}

// Reads the ciphertext fields, preserving noncanonical bits for verification.
static void decode_ct(ciphertext_t *value, const uint8_t *in)
{
	memcpy(value->u, in, R_SIZE_BYTES);
	memcpy(value->v, in + R_SIZE_BYTES, R_SIZE_BYTES);
	memcpy(value->c2, in + 2 * R_SIZE_BYTES, M_SIZE_BYTES);
	memcpy(value->salt, in + 2 * R_SIZE_BYTES + M_SIZE_BYTES, SALT_SIZE_BYTES);
}

// Serializes indices in little-endian order and clears trailing key padding.
static void encode_sk(uint8_t *out, const secret_key_t *key)
{
	const uint32_t *idx[3] = {key->h0_idx, key->h1_idx, key->h2_idx};
	memset(out, 0, SECRET_KEY_SIZE_BYTES);
	for (size_t block = 0; block < 3; ++block)
		for (size_t i = 0; i < PARAM_D; ++i)
			for (size_t byte = 0; byte < 4; ++byte)
				*out++ = (uint8_t)(idx[block][i] >> (8 * byte));
	encode_poly(out, key->h0); out += R_SIZE_BYTES;
	encode_poly(out, key->t0); out += R_SIZE_BYTES;
	encode_poly(out, key->r2); out += R_SIZE_BYTES;
	memcpy(out, key->sigma, M_SIZE_BYTES); out += M_SIZE_BYTES;
	memcpy(out, key->sigma2, M_SIZE_BYTES);
}

// Rejects indices outside the polynomial before they reach the decoder.
static int decode_sk(secret_key_t *key, const uint8_t *in)
{
	uint32_t *idx[3] = {key->h0_idx, key->h1_idx, key->h2_idx};
	for (size_t block = 0; block < 3; ++block) {
		for (size_t i = 0; i < PARAM_D; ++i) {
			uint32_t value = 0;
			for (size_t byte = 0; byte < 4; ++byte) value |= (uint32_t)*in++ << (8 * byte);
			if (value >= PARAM_R) return -1;
			idx[block][i] = value;
		}
	}
	memcpy(key->h0, in, R_SIZE_BYTES); in += R_SIZE_BYTES;
	memcpy(key->t0, in, R_SIZE_BYTES); in += R_SIZE_BYTES;
	memcpy(key->r2, in, R_SIZE_BYTES); in += R_SIZE_BYTES;
	memcpy(key->sigma, in, M_SIZE_BYTES); in += M_SIZE_BYTES;
	memcpy(key->sigma2, in, M_SIZE_BYTES);
	return 0;
}

// Combines the error-vector and ciphertext comparisons into a rejection mask.
static uint8_t rejection_mask(uint8_t errors, uint8_t u, uint8_t v)
{
	return errors | u | v;
}

// Computes t0 = (h0*r1 + h1)/(t1 + r1) over GF2X.
static inline int calculate_t0(
	uint8_t *t0, const uint8_t *h0, const uint8_t *h1,
	const uint8_t *t1, const uint8_t *r1)
{
	uint8_t *tmp1 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *tmp2 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	int rc = -1;
	if (!tmp1 || !tmp2) goto cleanup;

	trike_setz(tmp1, PADDED_R_SIZE_BYTES);
	trike_setz(tmp2, PADDED_R_SIZE_BYTES);

	gf2x_add(tmp1, t1, r1);
	if (gf2x_inv(tmp2, tmp1)) goto cleanup;

	if (gf2x_mul(tmp1, h0, r1)) goto cleanup;
	gf2x_add(tmp1, tmp1, h1);

	if (gf2x_mul(t0, tmp1, tmp2)) goto cleanup;

	rc = 0;
cleanup:
	free(tmp2);
	free(tmp1);
	return rc;
}

// Computes r2 = (t0*t2 + h2)/(t0 + h0) over GF2X.
static inline int calculate_r2(
	uint8_t *r2, const uint8_t *h0, const uint8_t *h2,
	const uint8_t *t0, const uint8_t *t2)
{
	uint8_t *tmp1 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *tmp2 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	int rc = -1;
	if (!tmp1 || !tmp2) goto cleanup;

	trike_setz(tmp1, PADDED_R_SIZE_BYTES);
	trike_setz(tmp2, PADDED_R_SIZE_BYTES);

	gf2x_add(tmp1, t0, h0);
	if (gf2x_inv(tmp2, tmp1)) goto cleanup;

	if (gf2x_mul(tmp1, t0, t2)) goto cleanup;
	gf2x_add(tmp1, tmp1, h2);

	if (gf2x_mul(r2, tmp1, tmp2)) goto cleanup;

	rc = 0;
cleanup:
	free(tmp2);
	free(tmp1);
	return rc;
}

// XORs two message buffers of length M_SIZE_BYTES.
static inline void msgxor(uint8_t *out, const uint8_t *in1, const uint8_t *in2)
{
	for (size_t i = 0; i < M_SIZE_BYTES; i++)
	{
		out[i] = in1[i] ^ in2[i];
	}
}

// Computes u = e0 + e1*r1 + e2*r2 or v = e0 + e1*t1 + e2*t2 over GF2X.
static inline int calculate_uv(
	uint8_t *u, const uint8_t *e0, const uint8_t *e1, const uint8_t *e2,
	const uint8_t *r1, const uint8_t *r2)
{
	uint8_t *tmp = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	int rc = -1;
	if (!tmp) goto cleanup;

	trike_setz(tmp, PADDED_R_SIZE_BYTES);

	trike_assign(e0, u, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);

	if (gf2x_mul(tmp, e1, r1)) goto cleanup;
	gf2x_add(u, u, tmp);
	if (gf2x_mul(tmp, e2, r2)) goto cleanup;
	gf2x_add(u, u, tmp);

	rc = 0;
cleanup:
	free(tmp);
	return rc;
}

// Computes s = (h0 + t0)*u + t0*v over GF2X.
static inline int calculate_s(
	uint8_t *s, const uint8_t *u, const uint8_t *v,
	const uint8_t *h0, const uint8_t *t0)
{
	uint8_t *tmp = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	int rc = -1;
	if (!tmp) goto cleanup;

	trike_setz(tmp, PADDED_R_SIZE_BYTES);

	gf2x_add(tmp, h0, t0);
	if (gf2x_mul(s, tmp, u)) goto cleanup;
	if (gf2x_mul(tmp, t0, v)) goto cleanup;
	gf2x_add(s, s, tmp);

	rc = 0;
cleanup:
	free(tmp);
	return rc;
}

// Compares two byte vectors and returns 0 only when they are equal.
static inline uint8_t compare_vec(const uint8_t *vec1, const uint8_t *vec2, size_t len_bytes)
{
	uint32_t diff = 0;
	for (size_t i = 0; i < len_bytes; ++i) diff |= vec1[i] ^ vec2[i];
	return (uint8_t)(0u - ((diff | (0u - diff)) >> 31));
}

// Hashes input bytes and truncates the result to M_SIZE_BYTES.
static inline int hash_length_m(const uint8_t *input, size_t input_len, uint8_t *output)
{
	uint8_t temp[64] = {0};
	if (pseudohash(512, input, (unsigned long long)input_len * 8, temp)) return -1;
	memcpy(output, temp, M_SIZE_BYTES);
	return 0;
}


// Generates the public and secret keys.
int kem_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	if (!pk || !sk || !pk_len_bytes || !sk_len_bytes) return -1;
	*pk_len_bytes = 0;
	*sk_len_bytes = 0;
	public_key_t pk_value = {0};
	secret_key_t sk_value = {0};
	public_key_t *local_pk = &pk_value;
	secret_key_t *local_sk = &sk_value;

	uint8_t *h0 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *h1 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *h2 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *t0 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *t1 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *t2 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *r1 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *r2 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	int rc = -1;
	if (!h0 || !h1 || !h2 || !t0 || !t1 || !t2 || !r1 || !r2) goto cleanup;


	trike_setz(h0, PADDED_R_SIZE_BYTES);
	trike_setz(h1, PADDED_R_SIZE_BYTES);
	trike_setz(h2, PADDED_R_SIZE_BYTES);
	trike_setz(t0, PADDED_R_SIZE_BYTES);
	trike_setz(t1, PADDED_R_SIZE_BYTES);
	trike_setz(t2, PADDED_R_SIZE_BYTES);
	trike_setz(r1, PADDED_R_SIZE_BYTES);
	trike_setz(r2, PADDED_R_SIZE_BYTES);

	uint8_t seed[M_SIZE_BYTES] = {0};
	uint8_t sigma[M_SIZE_BYTES] = {0};
	uint8_t sigma2[M_SIZE_BYTES] = {0};
	uint32_t h0_idx[PARAM_D] = {0};
	uint32_t h1_idx[PARAM_D] = {0};
	uint32_t h2_idx[PARAM_D] = {0};

	if (get_random_number(&drng_algorithm, seed, PARAM_M)) goto cleanup;
	if (get_random_number(&drng_algorithm, sigma2, PARAM_M)) goto cleanup;
	if (get_random_number(&drng_algorithm, sigma, PARAM_M)) goto cleanup;


	if (generate_secret_key(h0, h1, h2, h0_idx, h1_idx, h2_idx, seed)) goto cleanup;


	if (generate_hash_vectors(t1, t2, r1, sigma)) goto cleanup;

	if (calculate_t0(t0, h0, h1, t1, r1)) goto cleanup;
	if (calculate_r2(r2, h0, h2, t0, t2)) goto cleanup;

	memcpy(local_pk->r2, r2, R_SIZE_BYTES);
	memcpy(local_pk->sigma, sigma, M_SIZE_BYTES);

	memcpy(local_sk->h0_idx, h0_idx, PARAM_D * sizeof(uint32_t));
	memcpy(local_sk->h1_idx, h1_idx, PARAM_D * sizeof(uint32_t));
	memcpy(local_sk->h2_idx, h2_idx, PARAM_D * sizeof(uint32_t));
	memcpy(local_sk->h0, h0, R_SIZE_BYTES);
	memcpy(local_sk->t0, t0, R_SIZE_BYTES);
	memcpy(local_sk->r2, r2, R_SIZE_BYTES);
	memcpy(local_sk->sigma, sigma, M_SIZE_BYTES);
	memcpy(local_sk->sigma2, sigma2, M_SIZE_BYTES);

	encode_pk(pk, local_pk);
	encode_sk(sk, local_sk);
	*pk_len_bytes = PUBLIC_KEY_SIZE_BYTES;
	*sk_len_bytes = SECRET_KEY_SIZE_BYTES;
	rc = 0;
cleanup:
	free(r2);
	free(r1);
	free(t2);
	free(t1);
	free(t0);
	free(h2);
	free(h1);
	free(h0);

	return rc;
}

// Encapsulates a shared secret using the given public key.
int kem_enc(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes)
{
	if (!pk || !ss || !ct || !ss_len_bytes || !ct_len_bytes ||
		pk_len_bytes != PUBLIC_KEY_SIZE_BYTES) return -1;
	*ss_len_bytes = 0;
	*ct_len_bytes = 0;
	public_key_t pk_value;
	ciphertext_t ct_value = {0};
	public_key_t *local_pk = &pk_value;
	ciphertext_t *local_ct = &ct_value;
	decode_pk(local_pk, pk);

	uint8_t *e = (uint8_t *)trike_alloc(N_ZMM_SIZE_BYTES);
	uint8_t *e0 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *e1 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *e2 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *t1 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *t2 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *r1 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *r2 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *u = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *v = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *hash_input = (uint8_t *)calloc(M_SIZE_BYTES + CIPHERTEXT_SIZE_BYTES, sizeof(uint8_t));
	int rc = -1;
	if (!e || !e0 || !e1 || !e2 || !t1 || !t2 || !r1 || !r2 || !u || !v || !hash_input) goto cleanup;


	trike_setz(e, N_ZMM_SIZE_BYTES);
	trike_setz(t1, PADDED_R_SIZE_BYTES);
	trike_setz(t2, PADDED_R_SIZE_BYTES);
	trike_setz(r1, PADDED_R_SIZE_BYTES);
	trike_setz(r2, PADDED_R_SIZE_BYTES);

	uint8_t msg[M_SIZE_BYTES] = {0}, msg2[M_SIZE_BYTES] = {0}, err_hash[M_SIZE_BYTES] = {0};
	uint8_t sigma[M_SIZE_BYTES] = {0};

	if (get_random_number(&drng_algorithm, msg, PARAM_M)) goto cleanup;
	if (get_random_number(&drng_algorithm, local_ct->salt, SALT_SIZE_BYTES * 8)) goto cleanup;

	memcpy(sigma, local_pk->sigma, M_SIZE_BYTES);
	memcpy(r2, local_pk->r2, R_SIZE_BYTES);

	if (generate_hash_vectors(t1, t2, r1, sigma)) goto cleanup;

	if (generate_error_vector(e, e + R_ZMM_SIZE_BYTES, e + 2 * R_ZMM_SIZE_BYTES, msg, r2, local_ct->salt)) goto cleanup;

	trike_assign(e, e0, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);
	trike_assign(e + R_ZMM_SIZE_BYTES, e1, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);
	trike_assign(e + 2 * R_ZMM_SIZE_BYTES, e2, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);

	if (calculate_uv(u, e0, e1, e2, r1, r2)) goto cleanup;

	if (calculate_uv(v, e0, e1, e2, t1, t2)) goto cleanup;

	if (hash_length_m(e, N_ZMM_SIZE_BYTES, err_hash)) goto cleanup;

	msgxor(msg2, msg, err_hash);

	memcpy(local_ct->u, u, R_SIZE_BYTES);
	memcpy(local_ct->v, v, R_SIZE_BYTES);
	memcpy(local_ct->c2, msg2, M_SIZE_BYTES);

	encode_ct(ct, local_ct);
	memcpy(hash_input, msg, M_SIZE_BYTES);
	memcpy(hash_input + M_SIZE_BYTES, ct, CIPHERTEXT_SIZE_BYTES);

	if (hash_length_m(hash_input, M_SIZE_BYTES + CIPHERTEXT_SIZE_BYTES, ss)) goto cleanup;

	*ss_len_bytes = M_SIZE_BYTES;
	*ct_len_bytes = CIPHERTEXT_SIZE_BYTES;
	rc = 0;
cleanup:
	free(hash_input);
	free(v);
	free(u);
	free(r2);
	free(r1);
	free(t2);
	free(t1);
	free(e2);
	free(e1);
	free(e0);
	free(e);


	return rc;
}

// Decapsulates the shared secret from ciphertext using the secret key.
int kem_dec(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
	if (!sk || !ct || !ss || !ss_len_bytes ||
		sk_len_bytes != SECRET_KEY_SIZE_BYTES || ct_len_bytes != CIPHERTEXT_SIZE_BYTES) return -2;
	*ss_len_bytes = 0;
	secret_key_t sk_value = {0};
	ciphertext_t ct_value;
	secret_key_t *local_sk = &sk_value;
	ciphertext_t *local_ct = &ct_value;
	if (decode_sk(local_sk, sk)) return -2;
	decode_ct(local_ct, ct);


	uint32_t h0_idx[PARAM_D] = {0}, h1_idx[PARAM_D] = {0}, h2_idx[PARAM_D] = {0};
	uint8_t sigma[M_SIZE_BYTES] = {0}, sigma2[M_SIZE_BYTES] = {0}, err_hash[M_SIZE_BYTES] = {0};
	uint8_t msg[M_SIZE_BYTES] = {0}, msg2[M_SIZE_BYTES] = {0};

	uint8_t *h0 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *t0 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *t1 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *t2 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *r1 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *r2 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *u = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *v = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *s = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *e = (uint8_t *)trike_alloc(N_ZMM_SIZE_BYTES);
	uint8_t *e_calc = (uint8_t *)trike_alloc(N_ZMM_SIZE_BYTES);
	uint8_t *hash_input = (uint8_t *)calloc(M_SIZE_BYTES + CIPHERTEXT_SIZE_BYTES, sizeof(uint8_t));
	uint8_t *u_prime = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *v_prime = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *e0 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *e1 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	uint8_t *e2 = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
	int rc = -2;
	if (!h0 || !t0 || !t1 || !t2 || !r1 || !r2 || !u || !v || !s || !e || !e_calc || !hash_input || !u_prime || !v_prime || !e0 || !e1 || !e2) goto cleanup;


	trike_setz(h0, PADDED_R_SIZE_BYTES);
	trike_setz(t0, PADDED_R_SIZE_BYTES);
	trike_setz(t1, PADDED_R_SIZE_BYTES);
	trike_setz(t2, PADDED_R_SIZE_BYTES);
	trike_setz(r1, PADDED_R_SIZE_BYTES);
	trike_setz(r2, PADDED_R_SIZE_BYTES);
	trike_setz(u, PADDED_R_SIZE_BYTES);
	trike_setz(v, PADDED_R_SIZE_BYTES);
	trike_setz(u_prime, PADDED_R_SIZE_BYTES);
	trike_setz(v_prime, PADDED_R_SIZE_BYTES);
	trike_setz(s, PADDED_R_SIZE_BYTES);
	trike_setz(e, N_ZMM_SIZE_BYTES);
	trike_setz(e_calc, N_ZMM_SIZE_BYTES);


	memcpy(h0_idx, local_sk->h0_idx, PARAM_D * sizeof(uint32_t));
	memcpy(h1_idx, local_sk->h1_idx, PARAM_D * sizeof(uint32_t));
	memcpy(h2_idx, local_sk->h2_idx, PARAM_D * sizeof(uint32_t));
	memcpy(h0, local_sk->h0, R_SIZE_BYTES);
	memcpy(sigma, local_sk->sigma, M_SIZE_BYTES);
	memcpy(sigma2, local_sk->sigma2, M_SIZE_BYTES);
	memcpy(t0, local_sk->t0, R_SIZE_BYTES);
	memcpy(r2, local_sk->r2, R_SIZE_BYTES);
	memcpy(u, local_ct->u, R_SIZE_BYTES);
	memcpy(v, local_ct->v, R_SIZE_BYTES);
	memcpy(msg2, local_ct->c2, M_SIZE_BYTES);

	if (generate_hash_vectors(t1, t2, r1, sigma)) goto cleanup;

	if (calculate_s(s, u, v, h0, t0)) goto cleanup;


	if (decode(e, e + R_ZMM_SIZE_BYTES, e + 2 * R_ZMM_SIZE_BYTES, s, h0_idx, h1_idx, h2_idx)) goto cleanup;


	if (hash_length_m(e, N_ZMM_SIZE_BYTES, err_hash)) goto cleanup;

	msgxor(msg, msg2, err_hash);

	if (generate_error_vector(e_calc, e_calc + R_ZMM_SIZE_BYTES, e_calc + 2 * R_ZMM_SIZE_BYTES, msg, r2, local_ct->salt)) goto cleanup;

	trike_assign(e_calc, e0, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);
	trike_assign(e_calc + R_ZMM_SIZE_BYTES, e1, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);
	trike_assign(e_calc + 2 * R_ZMM_SIZE_BYTES, e2, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);

	if (calculate_uv(u_prime, e0, e1, e2, r1, r2)) goto cleanup;

	if (calculate_uv(v_prime, e0, e1, e2, t1, t2)) goto cleanup;

	// Invalid ciphertexts select the fallback secret; keep the mask opaque during selection.
	volatile uint8_t mask = rejection_mask(compare_vec(e, e_calc, N_ZMM_SIZE_BYTES),
		compare_vec(u, u_prime, R_ZMM_SIZE_BYTES),
		compare_vec(v, v_prime, R_ZMM_SIZE_BYTES));

	for (size_t i = 0; i < M_SIZE_BYTES; i++)
	{
		msg[i] = (msg[i] & ~mask) | (sigma2[i] & mask);
	}

	memcpy(hash_input, msg, M_SIZE_BYTES);
	memcpy(hash_input + M_SIZE_BYTES, ct, CIPHERTEXT_SIZE_BYTES);

	if (hash_length_m(hash_input, M_SIZE_BYTES + CIPHERTEXT_SIZE_BYTES, ss)) goto cleanup;

	*ss_len_bytes = M_SIZE_BYTES;
	rc = 0;
cleanup:
	free(hash_input);
	free(e_calc);
	free(e);
	free(s);
	free(v_prime);
	free(u_prime);
	free(e0);
	free(e1);
	free(e2);
	free(r2);
	free(r1);
	free(t2);
	free(t1);
	free(t0);
	free(h0);
	free(u);
	free(v);


	return rc;
}
