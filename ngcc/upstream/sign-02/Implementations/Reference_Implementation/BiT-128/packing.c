/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#include "packing.h"
#include "../../common/packing_checks.h"
#include "polyvec.h"
#include "endian.h"
#include <stdint.h>
#include <string.h>

#define TRACE_RANS(model_ok) ((void)0)

static uint32_t load32_le_local(const unsigned char *p)
{
    return load32_le(p);
}
static void store32_le_local(unsigned char *p, uint32_t x)
{
    store32_le(p, x);
}

#include "../../common/packing_public.h"

static void pack_polyveck_public(unsigned char *out, const polyveck *v)
{
    for (int i = 0; i < BIT_K; i++)
        pack_poly_public(out + i * BIT_POLY_PUBLIC_BYTES, &v->vec[i]);
}

static int unpack_polyveck_public(polyveck *v, const unsigned char *in)
{
    for (int i = 0; i < BIT_K; i++)
        if (unpack_poly_public(&v->vec[i], in + i * BIT_POLY_PUBLIC_BYTES) != 0)
            return -1;
    return 0;
}

void pack_pk(unsigned char *pk, const unsigned char *seed_A, const polyveck *public_vector)
{
    memcpy(pk, seed_A, BIT_SEEDBYTES);
    pk += BIT_SEEDBYTES;
    pack_polyveck_public(pk, public_vector);
}

void pack_sk(unsigned char *sk, const unsigned char *seed_A, const polyveck *public_vector,
             const unsigned char *secret_seed, const unsigned char *tr, const polyvecl *s_0,
             const polyveck *e)
{
    memcpy(sk, seed_A, BIT_SEEDBYTES);
    sk += BIT_SEEDBYTES;
    pack_polyveck_public(sk, public_vector);
    sk += BIT_POLYVECK_PUBLIC_BYTES;
    memcpy(sk, secret_seed, BIT_SEEDBYTES);
    sk += BIT_SEEDBYTES;
    memcpy(sk, tr, BIT_TRBYTES);
    sk += BIT_TRBYTES;
    for (int i = 0; i < BIT_L; ++i) {
        const int16_t *coeffs = s_0->vec[i].coeffs;
        for (int j = 0; j < BIT_N; j += 4) {
            *sk++ = (unsigned char)((coeffs[j] & 0x03) | ((coeffs[j + 1] & 0x03) << 2) |
                                    ((coeffs[j + 2] & 0x03) << 4) | ((coeffs[j + 3] & 0x03) << 6));
        }
    }

    for (int i = 0; i < BIT_K; i++) {
        const int16_t *coeffs = e->vec[i].coeffs;
        for (int j = 0; j < BIT_N; j += 4) {
            *sk++ = (unsigned char)((coeffs[j] & 0x03) | ((coeffs[j + 1] & 0x03) << 2) |
                                    ((coeffs[j + 2] & 0x03) << 4) | ((coeffs[j + 3] & 0x03) << 6));
        }
    }
}

int unpack_sk(unsigned char *seed_A, polyveck *public_vector, unsigned char *secret_seed,
              unsigned char *tr, polyvecl *s_0, polyveck *e, const unsigned char *sk)
{
    const size_t small_offset = 2 * BIT_SEEDBYTES + BIT_POLYVECK_PUBLIC_BYTES + BIT_TRBYTES;
    if (small2_encoding_invalid(sk + small_offset))
        return -1;

    memcpy(seed_A, sk, BIT_SEEDBYTES);
    sk += BIT_SEEDBYTES;
    if (unpack_polyveck_public(public_vector, sk) != 0)
        return -1;
    sk += BIT_POLYVECK_PUBLIC_BYTES;
    memcpy(secret_seed, sk, BIT_SEEDBYTES);
    sk += BIT_SEEDBYTES;
    memcpy(tr, sk, BIT_TRBYTES);
    sk += BIT_TRBYTES;

    for (int i = 0; i < BIT_L; ++i) {
        int16_t *coeffs = s_0->vec[i].coeffs;
        for (int j = 0; j < BIT_N; j += 4) {
            uint8_t byte = *sk++;
            coeffs[j] = (int16_t)((uint16_t)(byte & 0x03) << 14) >> 14;
            coeffs[j + 1] = (int16_t)((uint16_t)((byte >> 2) & 0x03) << 14) >> 14;
            coeffs[j + 2] = (int16_t)((uint16_t)((byte >> 4) & 0x03) << 14) >> 14;
            coeffs[j + 3] = (int16_t)((uint16_t)((byte >> 6) & 0x03) << 14) >> 14;
        }
    }

    for (int i = 0; i < BIT_K; i++) {
        int16_t *coeffs = e->vec[i].coeffs;
        for (int j = 0; j < BIT_N; j += 4) {
            uint8_t byte = *sk++;
            coeffs[j] = (int16_t)((uint16_t)(byte & 0x03) << 14) >> 14;
            coeffs[j + 1] = (int16_t)((uint16_t)((byte >> 2) & 0x03) << 14) >> 14;
            coeffs[j + 2] = (int16_t)((uint16_t)((byte >> 4) & 0x03) << 14) >> 14;
            coeffs[j + 3] = (int16_t)((uint16_t)((byte >> 6) & 0x03) << 14) >> 14;
        }
    }

    return 0;
}

int unpack_pk(unsigned char *seed_A, polyveck *public_vector, const unsigned char *pk)
{
    memcpy(seed_A, pk, BIT_SEEDBYTES);
    pk += BIT_SEEDBYTES;

    return unpack_polyveck_public(public_vector, pk);
}

/* Byte-aligned rANS with frequencies summing to 2^16. */
#include "../../common/signature_codec_128.inc"
