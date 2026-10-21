/* Copyright (c) 2026 Hang Zhang. */
#ifndef BIT_PACKING_PUBLIC_H
#define BIT_PACKING_PUBLIC_H

/* Canonical mod-q coefficients, least significant bit first. */
#if BIT_PUBLIC_COEFF_BITS == 13
static void pack_poly_public(unsigned char *out, const poly *a)
{
    for (int i = 0; i < BIT_N; i += 8, out += 13) {
        uint64_t lo = (uint64_t)(uint16_t)a->coeffs[i] |
                      ((uint64_t)(uint16_t)a->coeffs[i + 1] << 13) |
                      ((uint64_t)(uint16_t)a->coeffs[i + 2] << 26) |
                      ((uint64_t)(uint16_t)a->coeffs[i + 3] << 39) |
                      ((uint64_t)(uint16_t)a->coeffs[i + 4] << 52);
        uint64_t hi = ((uint64_t)(uint16_t)a->coeffs[i + 4] >> 12) |
                      ((uint64_t)(uint16_t)a->coeffs[i + 5] << 1) |
                      ((uint64_t)(uint16_t)a->coeffs[i + 6] << 14) |
                      ((uint64_t)(uint16_t)a->coeffs[i + 7] << 27);
        store64_le(out, lo);
        store32_le(out + 8, (uint32_t)hi);
        out[12] = (uint8_t)(hi >> 32);
    }
}

#if !BIT_IMPL_AVX
static int unpack_poly_public(poly *r, const unsigned char *in)
{
    uint32_t bad = 0;
    for (int i = 0; i < BIT_N; i += 8, in += 13) {
        uint64_t lo = load64_le(in);
        uint64_t hi = load32_le(in + 8) | ((uint64_t)in[12] << 32);
        r->coeffs[i] = lo & 8191U;
        r->coeffs[i + 1] = (lo >> 13) & 8191U;
        r->coeffs[i + 2] = (lo >> 26) & 8191U;
        r->coeffs[i + 3] = (lo >> 39) & 8191U;
        r->coeffs[i + 4] = ((lo >> 52) | (hi << 12)) & 8191U;
        r->coeffs[i + 5] = (hi >> 1) & 8191U;
        r->coeffs[i + 6] = (hi >> 14) & 8191U;
        r->coeffs[i + 7] = (hi >> 27) & 8191U;
        for (int j = 0; j < 8; j++)
            bad |= (uint32_t)(r->coeffs[i + j] >= BIT_Q);
    }
    BIT_CT_DECLASSIFY(&bad, sizeof bad);
    return bad != 0;
}
#endif
#elif BIT_PUBLIC_COEFF_BITS == 15
static void pack_poly_public(unsigned char *out, const poly *a)
{
    for (int i = 0; i < BIT_N; i += 8, out += 15) {
        uint64_t lo = (uint64_t)(uint16_t)a->coeffs[i] |
                      ((uint64_t)(uint16_t)a->coeffs[i + 1] << 15) |
                      ((uint64_t)(uint16_t)a->coeffs[i + 2] << 30) |
                      ((uint64_t)(uint16_t)a->coeffs[i + 3] << 45) |
                      ((uint64_t)(uint16_t)a->coeffs[i + 4] << 60);
        uint64_t hi = ((uint64_t)(uint16_t)a->coeffs[i + 4] >> 4) |
                      ((uint64_t)(uint16_t)a->coeffs[i + 5] << 11) |
                      ((uint64_t)(uint16_t)a->coeffs[i + 6] << 26) |
                      ((uint64_t)(uint16_t)a->coeffs[i + 7] << 41);
        store64_le(out, lo);
        store32_le(out + 8, (uint32_t)hi);
        out[12] = (uint8_t)(hi >> 32);
        out[13] = (uint8_t)(hi >> 40);
        out[14] = (uint8_t)(hi >> 48);
    }
}

#if !BIT_IMPL_AVX
static int unpack_poly_public(poly *r, const unsigned char *in)
{
    uint32_t bad = 0;
    for (int i = 0; i < BIT_N; i += 8, in += 15) {
        uint64_t lo = load64_le(in);
        uint64_t hi = load32_le(in + 8) | ((uint64_t)in[12] << 32) |
                      ((uint64_t)in[13] << 40) | ((uint64_t)in[14] << 48);
        r->coeffs[i] = lo & 32767U;
        r->coeffs[i + 1] = (lo >> 15) & 32767U;
        r->coeffs[i + 2] = (lo >> 30) & 32767U;
        r->coeffs[i + 3] = (lo >> 45) & 32767U;
        r->coeffs[i + 4] = ((lo >> 60) | (hi << 4)) & 32767U;
        r->coeffs[i + 5] = (hi >> 11) & 32767U;
        r->coeffs[i + 6] = (hi >> 26) & 32767U;
        r->coeffs[i + 7] = (hi >> 41) & 32767U;
        for (int j = 0; j < 8; j++)
            bad |= (uint32_t)(r->coeffs[i + j] >= BIT_Q);
    }
    BIT_CT_DECLASSIFY(&bad, sizeof bad);
    return bad != 0;
}
#endif
#else
#error "Unsupported public coefficient width"
#endif
#if BIT_IMPL_AVX
#include "../avx/common/packing_public.h"
#endif
#endif
