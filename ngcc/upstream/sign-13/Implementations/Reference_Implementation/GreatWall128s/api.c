#include <assert.h>
#include <string.h>

#include "api.h"
#include "faest.h"
#include "randomness.h"

int crypto_sign_keypair(unsigned char* pk, unsigned char* sk)
{
	do
	{
		rand_bytes(sk, GREATWALL_SECRET_KEY_BYTES);
#if SECURITY_PARAM == 128 || SECURITY_PARAM == 512
		const uint8_t top_mask = 0x01;
#elif SECURITY_PARAM == 192
		const uint8_t top_mask = 0x1F;
#elif SECURITY_PARAM == 256
		const uint8_t top_mask = 0x7F;
#endif
		sk[GREATWALL_FIELD_BYTES - 1] &= top_mask;
		sk[GREATWALL_SECRET_KEY_BYTES - 1] &= top_mask;
	} while (!faest_pubkey(pk, sk));
	return 0;
}

int crypto_sign(
	unsigned char *sm, unsigned long long *smlen,
	const unsigned char *m, unsigned long long mlen,
	const unsigned char *sk)
{
	*smlen = mlen + FAEST_SIGNATURE_BYTES;
	memmove(sm, m, mlen);

	uint8_t random_seed[SECURITY_PARAM / 8];
	rand_bytes(random_seed, sizeof(random_seed));
	faest_sign(sm + mlen, sm, mlen, sk, random_seed, sizeof(random_seed));
	return 0;
}

int crypto_sign_open(
	unsigned char *m, unsigned long long *mlen,
	const unsigned char *sm, unsigned long long smlen,
	const unsigned char *pk)
{
	if (!mlen)
		return -1;
	*mlen = 0;
	/* Signed messages end with exactly FAEST_SIGNATURE_BYTES signature bytes. */
	if (!sm || !pk || smlen < FAEST_SIGNATURE_BYTES || smlen > SIZE_MAX ||
	    smlen > PTRDIFF_MAX)
		return -1;
	const size_t m_length = (size_t) smlen - FAEST_SIGNATURE_BYTES;
	if ((!m && m_length != 0) || !faest_verify(sm + m_length, sm, m_length, pk))
		return -1;

	if (m_length != 0)
		memmove(m, sm, m_length);
	*mlen = m_length;
	return 0;
}
