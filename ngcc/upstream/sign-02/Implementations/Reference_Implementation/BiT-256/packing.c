/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#include "../../common/ct_audit.h"
#include "packing.h"
#include "polyvec.h"
#include "endian.h"
#include <stdint.h>
#include <string.h>

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

static void pack_poly_small2(unsigned char out[BIT_N / 4], const poly *a)
{
    for (int i = 0; i < BIT_N; i += 4) {
        out[i / 4] =
            (uint8_t)(((uint16_t)a->coeffs[i] & 3U) | (((uint16_t)a->coeffs[i + 1] & 3U) << 2) |
                      (((uint16_t)a->coeffs[i + 2] & 3U) << 4) |
                      (((uint16_t)a->coeffs[i + 3] & 3U) << 6));
    }
}

static int unpack_poly_small2(poly *r, const unsigned char in[BIT_N / 4])
{
    uint32_t bad = 0;

    for (int i = 0; i < BIT_N; i += 4) {
        uint8_t packed = in[i / 4];

        for (int j = 0; j < 4; j++) {
            uint16_t code = (uint16_t)((packed >> (2 * j)) & 3U);

            bad |= code == 2U;
            r->coeffs[i + j] = (int16_t)(code << 14) >> 14;
        }
    }
    /* Constant over valid generated signing keys; keep decoded coefficients secret. */
    BIT_CT_DECLASSIFY(&bad, sizeof bad);
    return bad != 0 ? -1 : 0;
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
        pack_poly_small2(sk, &s_0->vec[i]);
        sk += BIT_N / 4;
    }

    for (int i = 0; i < BIT_K; i++) {
        pack_poly_small2(sk, &e->vec[i]);
        sk += BIT_N / 4;
    }
}

int unpack_sk(unsigned char *seed_A, polyveck *public_vector, unsigned char *secret_seed,
              unsigned char *tr, polyvecl *s_0, polyveck *e, const unsigned char *sk)
{
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
        if (unpack_poly_small2(&s_0->vec[i], sk) != 0)
            return -1;
        sk += BIT_N / 4;
    }

    for (int i = 0; i < BIT_K; i++) {
        if (unpack_poly_small2(&e->vec[i], sk) != 0)
            return -1;
        sk += BIT_N / 4;
    }

    return 0;
}

int unpack_pk(unsigned char *seed_A, polyveck *public_vector, const unsigned char *pk)
{
    memcpy(seed_A, pk, BIT_SEEDBYTES);
    pk += BIT_SEEDBYTES;

    return unpack_polyveck_public(public_vector, pk);
}

/* Full-support rANS response model with sigma=sqrt((24576^2-185^2)/3588). */
#include "../../common/signature_codec_256.inc"
