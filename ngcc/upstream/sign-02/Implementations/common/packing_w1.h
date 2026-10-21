/* Copyright (c) 2026 Hang Zhang. */
#ifndef BIT_PACKING_W1_H
#define BIT_PACKING_W1_H
#include "endian.h"
#if defined(__AVX2__)
#include <immintrin.h>
#endif

static inline void bit_pack_w1_5(uint8_t *out, const int16_t *coeffs)
{
#if defined(__AVX2__)
    const __m128i pair_weights = _mm_set1_epi16(0x2001);
    const __m128i quad_weights = _mm_set1_epi32(0x04000001);

    for (int i = 0; i < BIT_N; i += 16) {
        __m256i x = _mm256_loadu_si256((const __m256i *)(coeffs + i));
        __m128i bytes = _mm_packus_epi16(
            _mm256_castsi256_si128(x), _mm256_extracti128_si256(x, 1));
        __m128i quads = _mm_madd_epi16(_mm_maddubs_epi16(bytes, pair_weights), quad_weights);
        uint64_t lo = (uint32_t)_mm_cvtsi128_si32(quads) |
            ((uint64_t)(uint32_t)_mm_extract_epi32(quads, 1) << 20);
        uint64_t hi = (uint32_t)_mm_extract_epi32(quads, 2) |
            ((uint64_t)(uint32_t)_mm_extract_epi32(quads, 3) << 20);

        store32_le(out, (uint32_t)lo);
        out[4] = (uint8_t)(lo >> 32);
        store32_le(out + 5, (uint32_t)hi);
        out[9] = (uint8_t)(hi >> 32);
        out += 10;
    }
#else
    for (int i = 0; i < BIT_N; i += 8) {
        uint64_t value = 0;
        for (unsigned int j = 0; j < 8; j++)
            value |= (uint64_t)(uint16_t)coeffs[i + j] << (5 * j);
        store32_le(out, (uint32_t)value);
        out[4] = (uint8_t)(value >> 32);
        out += 5;
    }
#endif
}
#endif
