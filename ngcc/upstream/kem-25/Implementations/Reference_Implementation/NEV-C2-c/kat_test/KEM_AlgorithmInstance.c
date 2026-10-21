/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include <string.h>

#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "../cca.h"
#include "../api.h"

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

unsigned long long kem_get_pk_len_bytes()
{
	return KEM_PUBLICKEYBYTES;
}

unsigned long long kem_get_sk_len_bytes()
{
	return KEM_SECRETKEYBYTES;
}

unsigned long long kem_get_ss_len_bytes()
{
	return KEM_BYTES;
}

unsigned long long kem_get_ct_len_bytes()
{
	return KEM_CIPHERTEXTBYTES;
}

int kem_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	*pk_len_bytes = KEM_PUBLICKEYBYTES;
	*sk_len_bytes = KEM_SECRETKEYBYTES;
	return  kem_cca_keygen(pk, sk);;
}

int kem_enc(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes)
{
	int ret;

	*ss_len_bytes = 0;
	*ct_len_bytes = 0;

	if (pk_len_bytes != KEM_PUBLICKEYBYTES) {
		memset(ss, 0, KEM_BYTES);
		memset(ct, 0, KEM_CIPHERTEXTBYTES);
		return -1;
	}

	ret = kem_cca_enc(ss, ct, pk);
	if (ret != 0) {
		memset(ss, 0, KEM_BYTES);
		memset(ct, 0, KEM_CIPHERTEXTBYTES);
		return -1;
	}

	*ss_len_bytes = KEM_BYTES;
	*ct_len_bytes = KEM_CIPHERTEXTBYTES;
	return 0;
}

int kem_dec(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
	int ret;

	*ss_len_bytes = 0;

	if (sk_len_bytes != KEM_SECRETKEYBYTES || ct_len_bytes != KEM_CIPHERTEXTBYTES) {
		memset(ss, 0, KEM_BYTES);
		return -1;
	}

	ret = kem_cca_dec(ss, ct, sk);

	if (ret != 0) {
		memset(ss, 0, KEM_BYTES);
		return -1;
	}

	*ss_len_bytes = KEM_BYTES;
	return 0;
}
