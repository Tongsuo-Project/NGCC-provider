/* Copyright (c) 2026 Hang Zhang. */

#include "sample.h"
#include "symmetric.h"
#if BIT_IMPL_AVX
#include <immintrin.h>
#endif

#if BIT_N == 1024
int32_t bit_round_q13_to_i32(int32_t x)
#else
int16_t bit_round_q13_to_i16(int32_t x)
#endif
{
    return (x + (int32_t)(BIT_PARABOLA_SCALE >> 1)) >> BIT_PARABOLA_FRAC_BITS;
}

int polyvecy_sample_mask_q13(polyvecy *y, bit_parabola_fixed *y_q13, const uint8_t *seed,
                             uint16_t *nonce)
{
    if (bit_parabola_sample_fixed(y_q13, seed, nonce) != 0)
        return -1;
#if BIT_IMPL_AVX
    const __m256i half = _mm256_set1_epi32(BIT_PARABOLA_SCALE >> 1);
    for (int i = 0; i < BIT_PARABOLA_BLOCKS; i++) {
#if BIT_N == 1024
        for (int j = 0; j < BIT_N; j += 8) {
            __m256i x = _mm256_loadu_si256((const __m256i *)&y_q13->coeffs[i][j]);
            x = _mm256_srai_epi32(_mm256_add_epi32(x, half), BIT_PARABOLA_FRAC_BITS);
            _mm256_store_si256((__m256i *)&y->vec[i].coeffs[j], x);
        }
#else
        for (int j = 0; j < BIT_N; j += 16) {
            __m256i lo = _mm256_loadu_si256((const __m256i *)&y_q13->coeffs[i][j]);
            __m256i hi = _mm256_loadu_si256((const __m256i *)&y_q13->coeffs[i][j + 8]);
            lo = _mm256_srai_epi32(_mm256_add_epi32(lo, half), BIT_PARABOLA_FRAC_BITS);
            hi = _mm256_srai_epi32(_mm256_add_epi32(hi, half), BIT_PARABOLA_FRAC_BITS);
            __m256i packed = _mm256_permute4x64_epi64(_mm256_packs_epi32(lo, hi), 0xD8);
            _mm256_storeu_si256((__m256i *)&y->vec[i].coeffs[j], packed);
        }
#endif
    }
#else
    for (int i = 0; i < BIT_PARABOLA_BLOCKS; i++)
        for (int j = 0; j < BIT_N; j++)
#if BIT_N == 1024
            y->vec[i].coeffs[j] = bit_round_q13_to_i32(y_q13->coeffs[i][j]);
#else
            y->vec[i].coeffs[j] = bit_round_q13_to_i16(y_q13->coeffs[i][j]);
#endif
#endif
    return 0;
}

#if BIT_IMPL_AVX && BIT_N != 1024
static inline int check_signature_bounds_with_limit(const polyvecm1 *z1, const polyveck *h,
                                                    int32_t coeff_bound)
{
    const __m256i response_bound = _mm256_set1_epi16(coeff_bound);
    const __m256i hint_bound = _mm256_set1_epi16(BIT_H_INF);
    const __m256i hint_lower = _mm256_set1_epi16(-BIT_H_INF);
    const __m256i zero = _mm256_setzero_si256();
    const __m256i all_ones = _mm256_cmpeq_epi16(zero, zero);
    __m256i reject = _mm256_setzero_si256();
    __m256i nonzero_mod_q = _mm256_setzero_si256();
    __m256i hint_norm = _mm256_setzero_si256();
    __m256i response_norm = _mm256_setzero_si256();

    for (int i = 0; i < BIT_L + 1; i++) {
        for (int j = 0; j < BIT_N; j += 16) {
            __m256i v = _mm256_loadu_si256((const __m256i *)&z1->vec[i].coeffs[j]);
            /* Zero-extend pairwise squares before accumulating. Even the
             * INT16_MIN pair (0x80000000) is an exact unsigned square sum. */
            __m256i squares = _mm256_madd_epi16(v, v);
            response_norm = _mm256_add_epi64(response_norm,
                _mm256_add_epi64(_mm256_unpacklo_epi32(squares, zero),
                                 _mm256_unpackhi_epi32(squares, zero)));
            __m256i sign = _mm256_srai_epi16(v, 15);
            __m256i abs_v = _mm256_sub_epi16(_mm256_xor_si256(v, sign), sign);
            __m256i zero_residue = _mm256_cmpeq_epi16(abs_v, zero);
            for (int k = 1; k <= coeff_bound / BIT_Q; k++) {
                __m256i multiple = _mm256_set1_epi16(k * BIT_Q);
                zero_residue = _mm256_or_si256(zero_residue,
                    _mm256_cmpeq_epi16(abs_v, multiple));
            }
            nonzero_mod_q =
                _mm256_or_si256(nonzero_mod_q, _mm256_andnot_si256(zero_residue, all_ones));
            reject =
                _mm256_or_si256(reject, _mm256_or_si256(_mm256_cmpgt_epi16(abs_v, response_bound),
                                                        _mm256_cmpgt_epi16(zero, abs_v)));
        }
    }

    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j += 16) {
            __m256i v = _mm256_loadu_si256((const __m256i *)&h->vec[i].coeffs[j]);
            reject = _mm256_or_si256(reject, _mm256_or_si256(_mm256_cmpgt_epi16(hint_lower, v),
                                                             _mm256_cmpgt_epi16(v, hint_bound)));
            __m256i limited = _mm256_min_epi16(_mm256_max_epi16(v, hint_lower), hint_bound);
            hint_norm = _mm256_add_epi32(hint_norm, _mm256_madd_epi16(limited, limited));
        }
    }

    uint64_t response_norm_lanes[4];
    uint64_t response_norm_sq = 0;
    _mm256_storeu_si256((__m256i *)response_norm_lanes, response_norm);
    for (int i = 0; i < 4; i++)
        response_norm_sq += response_norm_lanes[i];

    uint32_t hint_norm_lanes[8];
    uint32_t hint_norm_sq = 0;
    _mm256_storeu_si256((__m256i *)hint_norm_lanes, hint_norm);
    for (int i = 0; i < 8; i++)
        hint_norm_sq += hint_norm_lanes[i];

    uint32_t reject_flag = (uint32_t)!_mm256_testz_si256(reject, reject);
    reject_flag |= (uint32_t)_mm256_testz_si256(nonzero_mod_q, nonzero_mod_q);
    reject_flag |= (uint32_t)(hint_norm_sq > BIT_H_L2_BOUND_SQ);
    reject_flag |= (uint32_t)(response_norm_sq +
        (uint64_t)BIT_GAMMA2 * BIT_GAMMA2 * hint_norm_sq > BIT_SIGNATURE_L2_BOUND_SQ);
    return (int)reject_flag;
}
#else
static inline int check_signature_bounds_with_limit(const polyvecm1 *z1, const polyveck *h,
                                                    int32_t coeff_bound)
{
    uint32_t reject = 0, nonzero = 0, hint_norm_sq = 0;
    uint64_t response_norm_sq = 0;
    for (int i = 0; i < BIT_L + 1; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int32_t value = z1->vec[i].coeffs[j];
            uint32_t bits = (uint32_t)value;
            uint32_t sign = bits >> 31;
            uint32_t magnitude = (bits ^ (0U - sign)) + sign;
            uint32_t valid = magnitude <= (uint32_t)coeff_bound;
            /* Mask invalid coefficients before squaring to prevent accumulator overflow. */
            uint32_t limited = magnitude & (0U - valid);
            reject |= valid ^ 1U;
            response_norm_sq += (uint64_t)limited * limited;
            uint32_t residue_nonzero = magnitude != 0;
            for (uint32_t k = 1; k <= (uint32_t)coeff_bound / BIT_Q; k++)
                residue_nonzero &= magnitude != k * BIT_Q;
            nonzero |= residue_nonzero;
        }
    }
    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            uint32_t bits = (uint32_t)h->vec[i].coeffs[j];
            uint32_t sign = bits >> 31;
            uint32_t magnitude = (bits ^ (0U - sign)) + sign;
            uint32_t valid = magnitude <= BIT_H_INF;
            uint32_t limited = magnitude & (0U - valid);
            reject |= valid ^ 1U;
            hint_norm_sq += limited * limited;
        }
    }
    reject |= hint_norm_sq > BIT_H_L2_BOUND_SQ;
    reject |= nonzero == 0;
    reject |= response_norm_sq + (uint64_t)BIT_GAMMA2 * BIT_GAMMA2 * hint_norm_sq >
              BIT_SIGNATURE_L2_BOUND_SQ;
    return reject != 0;
}
#endif

int check_signature_bounds(const polyvecm1 *z1, const polyveck *h)
{
    return check_signature_bounds_with_limit(z1, h, BIT_RESPONSE_COEFF_BOUND);
}

int check_signature_verification_bounds(const polyvecm1 *z1, const polyveck *h)
{
    return check_signature_bounds_with_limit(z1, h, BIT_Q_HALF);
}
