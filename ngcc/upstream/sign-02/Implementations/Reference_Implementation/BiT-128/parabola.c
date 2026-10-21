/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 *
 * Fixed-point Gaussian routines adapted from the HAETAE 2.0 reference
 * implementation, Copyright (c) 2023 Team HAETAE, under the MIT license.
 * Four auxiliary coordinates give density proportional to L^2-||y||_2^2.
 */
#include "../../common/ct_audit.h"
#include "parabola.h"

#include "endian.h"
#include "params.h"
#include "symmetric.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if BIT_PARABOLA_AUX_DIM != 4
#error "The paraboloid projection requires exactly four auxiliary dimensions"
#endif

#if BIT_PARABOLA_BLOCKS < BIT_PARABOLA_AUX_DIM
#error "Not enough polynomial chunks for the auxiliary coordinates"
#endif

#define PARABOLA_TOTAL_DIM (BIT_PARABOLA_DIM + BIT_PARABOLA_AUX_DIM)

/* Inverse-square-root constants for D+4 = 1796. */
#if PARABOLA_TOTAL_DIM != 1796
#error "Regenerate the inverse-square-root constants for the new dimension"
#endif

#if BIT_PARABOLA_DIM % 4 != 0
#error "The scaled cell-integral correction requires D/4 to be integral"
#endif

typedef struct {
    uint64_t limb48[2];
} fp96_76;

#if defined(__SIZEOF_INT128__)
__extension__ typedef __int128 bit_int128;
__extension__ typedef unsigned __int128 bit_uint128;
#endif

static void fp_renormalize(fp96_76 *x)
{
    x->limb48[1] += x->limb48[0] >> 48;
    x->limb48[0] &= (1ULL << 48) - 1ULL;
}

/* Wide multiplication using native 128-bit integers or 32-bit limbs. */
static void mul64(uint64_t r[2], uint64_t b, uint64_t a)
{
#if defined(__SIZEOF_INT128__)
    bit_uint128 product = (bit_uint128)a * (bit_uint128)b;

    r[0] = (uint64_t)product;
    r[1] = (uint64_t)(product >> 64);
#else
    uint64_t al = a & 0xFFFFFFFFULL;
    uint64_t bl = b & 0xFFFFFFFFULL;
    uint64_t ah = a >> 32;
    uint64_t bh = b >> 32;

    uint64_t cross0 = ah * bl;
    uint64_t cross1 = al * bh;
    /* Sum only 32-bit limbs here: full cross-products may overflow. */
    uint64_t carry = ((al * bl) >> 32) + (cross0 & 0xFFFFFFFFULL) + (cross1 & 0xFFFFFFFFULL);

    r[0] = a * b;
    r[1] = ah * bh + (cross0 >> 32) + (cross1 >> 32) + (carry >> 32);
#endif
}

static void sq64(uint64_t r[2], uint64_t a)
{
#if defined(__SIZEOF_INT128__)
    bit_uint128 square = (bit_uint128)a * (bit_uint128)a;

    r[0] = (uint64_t)square;
    r[1] = (uint64_t)(square >> 64);
#else
    uint64_t al = a & 0xFFFFFFFFULL;
    uint64_t ah = a >> 32;

    uint64_t cross = ah * al;
    /* Include the low-product carry without overflowing the doubled cross. */
    uint64_t carry = ((al * al) >> 32) + ((cross & 0xFFFFFFFFULL) << 1);

    r[0] = a * a;
    r[1] = ah * ah + ((cross >> 32) << 1) + (carry >> 32);
#endif
}

static void mul48(uint64_t r[2], uint64_t b, uint64_t a)
{
    mul64(r, b, a);
    r[1] = (r[1] << 16) ^ (r[0] >> 48);
    r[0] &= (1ULL << 48) - 1ULL;
}

static void mulacc48(uint64_t r[2], uint64_t b, uint64_t a)
{
    uint64_t t[2];
    mul48(t, b, a);
    r[0] += t[0];
    r[1] += t[1];
}

static void sq48(uint64_t r[2], uint64_t a)
{
    sq64(r, a);
    r[1] = (r[1] << 16) ^ (r[0] >> 48);
    r[0] &= (1ULL << 48) - 1ULL;
}

static int64_t smulh48(int64_t a, uint64_t b)
{
#if defined(__SIZEOF_INT128__)
    return (int64_t)(((bit_int128)a * (bit_int128)b + ((bit_int128)1 << 47)) >> 48);
#else
    int64_t ah = a >> 24;
    int64_t al = a - ah * (1LL << 24);
    int64_t bl = (int64_t)(b & ((1ULL << 24) - 1ULL));
    int64_t bh = (int64_t)(b >> 24);
    int64_t r = (al * bl) >> 24;

    r += al * bh + ah * bl + (1LL << 23);
    r >>= 24;
    return r + ah * bh;
#endif
}

static void fp_add(fp96_76 *r, const fp96_76 *a, const fp96_76 *b)
{
    r->limb48[0] = a->limb48[0] + b->limb48[0];
    r->limb48[1] = a->limb48[1] + b->limb48[1];
}

static void fp_copy_cneg(fp96_76 *r, const fp96_76 *a, uint8_t sign)
{
    uint64_t mask = (uint64_t)-(int64_t)sign;
    r->limb48[0] = (a->limb48[0] ^ (mask & ((1ULL << 48) - 1ULL))) + sign;
    r->limb48[1] = a->limb48[1] ^ mask;
    fp_renormalize(r);
}

static void fp_cneg(fp96_76 *a, uint8_t sign)
{
    uint64_t mask = (uint64_t)-(int64_t)sign;
    a->limb48[0] = (a->limb48[0] ^ (mask & ((1ULL << 48) - 1ULL))) + sign;
    a->limb48[1] ^= mask;
    fp_renormalize(a);
}

static inline __attribute__((always_inline)) void fp_mul(fp96_76 *r, const fp96_76 *a,
                                                         const fp96_76 *b)
{
    uint64_t t[2];

    mul48(&r->limb48[0], a->limb48[0], b->limb48[0]);
    r->limb48[0] = r->limb48[1] + (((r->limb48[0] >> 47) + 1ULL) >> 1);

    mul48(t, a->limb48[0], b->limb48[1]);
    r->limb48[0] += t[0];
    r->limb48[1] = t[1];
    mulacc48(&r->limb48[0], a->limb48[1], b->limb48[0]);

    r->limb48[0] += 1ULL << 27;
    r->limb48[0] >>= 28;
    r->limb48[0] += (r->limb48[1] << 20) & ((1ULL << 48) - 1ULL);
    r->limb48[1] >>= 28;

    mul64(t, a->limb48[1], b->limb48[1]);
    r->limb48[0] += (t[0] << 20) & ((1ULL << 48) - 1ULL);
    r->limb48[1] += (t[0] >> 28) + (t[1] << 36);
    fp_renormalize(r);
}

static inline __attribute__((always_inline)) void fp_square(fp96_76 *r, const fp96_76 *a)
{
    uint64_t t[2];

    sq48(&r->limb48[0], a->limb48[0]);
    r->limb48[0] >>= 48;
    r->limb48[0] += r->limb48[1];

    mul48(t, a->limb48[0], a->limb48[1]);
    r->limb48[0] += t[0] << 1;
    r->limb48[1] = t[1] << 1;

    r->limb48[0] >>= 28;
    r->limb48[0] += (r->limb48[1] << 20) & ((1ULL << 48) - 1ULL);
    r->limb48[1] >>= 28;

    sq64(t, a->limb48[1]);
    r->limb48[0] += (t[0] << 20) & ((1ULL << 48) - 1ULL);
    r->limb48[1] += (t[0] >> 28) + (t[1] << 36);
    fp_renormalize(r);
}

static void fp_sub(fp96_76 *r, const fp96_76 *a, const fp96_76 *b)
{
    fp96_76 neg;
    fp_copy_cneg(&neg, b, 1);
    fp_add(r, a, &neg);
}

static void fp_sub_from_threehalves(fp96_76 *a)
{
    fp_cneg(a, 1);
    a->limb48[1] += 3ULL << 27;
    fp_renormalize(a);
}

static void fp_unsigned_signed_mul(fp96_76 *a, const fp96_76 *b)
{
    fp96_76 abs_b, t;
    uint8_t sign = (uint8_t)(b->limb48[1] >> 63);

    fp_copy_cneg(&abs_b, b, sign);
    fp_mul(&t, &abs_b, a);
    fp_copy_cneg(a, &t, sign);
}

/* Q76 tangent-line seed at x = 1796. */
static const fp96_76 invsqrt_start_cube = {{0xCBDC3B2E57A2ULL, 0xDC6ULL}};
static const fp96_76 invsqrt_start_threehalf = {{0x4F96C9AE3254ULL, 0x90FA05ULL}};

static void fp_invsqrt(fp96_76 *r, const fp96_76 *xhalf)
{
    fp96_76 t, t2;

    fp_mul(&t, xhalf, &invsqrt_start_cube);
    fp_sub(r, &invsqrt_start_threehalf, &t);

    for (unsigned int i = 0; i < 6; i++) {
        fp_square(&t, r);
        fp_mul(&t2, xhalf, &t);
        fp_sub_from_threehalves(&t2);
        fp_unsigned_signed_mul(r, &t2);
    }
}

/* r = a * c / 2^28, where a is Q76 and c fits in 48 bits. */
static void fp_mul_high(fp96_76 *r, const fp96_76 *a, uint64_t c)
{
    uint64_t t[2];

    mul48(&r->limb48[0], a->limb48[0], c);
    mul48(t, a->limb48[1], c);
    r->limb48[1] += t[0];

    r->limb48[0] += 1ULL << 27;
    r->limb48[0] >>= 28;
    r->limb48[0] += (r->limb48[1] << 20) & ((1ULL << 48) - 1ULL);
    r->limb48[1] >>= 28;
    r->limb48[1] += t[1] << 20;
    fp_renormalize(r);
}

/* Multiply a Q60 Gaussian coordinate by a Q76 scale and round to Q13. */
static int32_t fp_mul_round13(const uint64_t x, const fp96_76 *scale, const uint8_t sign)
{
    fp96_76 xx, t;
    uint64_t mag;

    xx.limb48[1] = x >> 32;
    xx.limb48[0] = (x & 0xFFFFFFFFULL) << 16;
    fp_mul(&t, &xx, scale);
    mag = (t.limb48[1] + (1ULL << 14)) >> 15;
    return (1 - 2 * (int32_t)sign) * (int32_t)mag;
}

static uint64_t approx_exp(uint64_t x)
{
    int64_t r;
    r = -0x0000B6C6340925AELL;
    r = ((smulh48(r, x) + (1LL << 2)) >> 3) + 0x0000B4BD4DF85227LL;
    r = ((smulh48(r, x) + (1LL << 2)) >> 3) - 0x0000887F727491E2LL;
    r = ((smulh48(r, x) + (1LL << 1)) >> 2) + 0x0000AAAA643C7E8DLL;
    r = ((smulh48(r, x) + (1LL << 1)) >> 2) - 0x0000AAAAA98179E6LL;
    r = ((smulh48(r, x) + 1LL) >> 1) + 0x0000FFFFFFFB2E7ALL;
    r = ((smulh48(r, x) + 1LL) >> 1) - 0x0000FFFFFFFFF85FLL;
    r = smulh48(r, x) + 0x0000FFFFFFFFFFFCLL;
    return (uint64_t)r;
}

#define CDT_LEN 64
static const uint32_t cdt[CDT_LEN] = {
    3266,  6520,  9748,  12938, 16079, 19159, 22168, 25096, 27934, 30674, 33309, 35833, 38241,
    40531, 42698, 44742, 46663, 48460, 50135, 51690, 53128, 54454, 55670, 56781, 57794, 58712,
    59541, 60287, 60956, 61554, 62085, 62556, 62972, 63337, 63657, 63936, 64178, 64388, 64569,
    64724, 64857, 64970, 65066, 65148, 65216, 65273, 65321, 65361, 65394, 65422, 65444, 65463,
    65478, 65490, 65500, 65508, 65514, 65519, 65523, 65527, 65529, 65531, 65533, 65534};

static uint64_t sample_gauss16(uint64_t r16)
{
    uint64_t r = 0;
    for (unsigned int i = 0; i < CDT_LEN; i++)
        r += (((uint64_t)cdt[i] - r16) >> 63) & 1ULL;
    return r;
}

#define GAUSS_RAND_BYTES 17
#define GAUSS_INITIAL_BYTES (GAUSS_RAND_BYTES * BIT_N)
#define GAUSS_INITIAL_CANDIDATES ((GAUSS_INITIAL_BYTES - BIT_N / 8) / GAUSS_RAND_BYTES)
#define GAUSS_STREAM_BATCH_CANDIDATES 127
#define GAUSS_REFILL_CANDIDATES 8
#define GAUSS_BATCH_BYTES (GAUSS_RAND_BYTES * GAUSS_STREAM_BATCH_CANDIDATES)

static int sample_gauss76(uint64_t *sample, fp96_76 *square, const uint8_t rand[GAUSS_RAND_BYTES])
{
    uint64_t r16 = load16_le(rand);
    uint64_t rrej = load64_le(rand + 2) & 0xFFFFFFFFFFFFULL;
    uint64_t x = sample_gauss16(r16);
    uint64_t exp_in;
    fp96_76 y;
    int64_t diff;
    uint64_t nonzero;

    y.limb48[0] = load64_le(rand + 8) & 0xFFFFFFFFFFFFULL;
    y.limb48[1] =
        (uint64_t)rand[14] | ((uint64_t)rand[15] << 8) | ((uint64_t)rand[16] << 16) | (x << 24);

    *sample = (y.limb48[0] >> 15) ^ (y.limb48[1] << 33);
    *sample = (*sample + 1ULL) >> 1;

    fp_square(square, &y);
    exp_in = square->limb48[1] - (x * x << 20);
    exp_in = (exp_in << 20) | (square->limb48[0] >> 28);
    exp_in = (exp_in + 1ULL) >> 1;

    rrej ^= rrej & 1ULL;
    diff = (int64_t)rrej - (int64_t)approx_exp(exp_in);
    nonzero = ((*sample | (0ULL - *sample)) >> 63) | rrej;
    return BIT_CT_GAUSSIAN_ACCEPT((int)((((uint64_t)(diff >> 63)) & nonzero) & 1ULL));
}

/* Sample one polynomial worth of Gaussian coordinates.  If with_aux is set,
 * one additional coordinate contributes to the norm but is not stored. */
static __attribute__((noinline, aligned(64))) int
sample_gauss_chunk(uint64_t samples[BIT_N], uint8_t signs[BIT_N / 8], fp96_76 *sqsum,
                   const uint8_t *seed, uint16_t nonce, int with_aux)
{
    bit_xof256_state state;
    fp96_76 lane_sum[4] = {{{0, 0}}, {{0, 0}}, {{0, 0}}, {{0, 0}}};
    uint8_t randbuf[GAUSS_BATCH_BYTES];
    size_t accepted = 0;
    size_t initial_remaining = GAUSS_INITIAL_CANDIDATES;
    const size_t needed = BIT_N + (size_t)with_aux;
    bit_xof256_init(&state, seed, BIT_SEEDBYTES, nonce);
    if (bit_xof256_squeeze(&state, signs, BIT_N / 8) != 0) {
        bit_xof256_zeroize(&state);
        bit_secure_zero(randbuf, sizeof(randbuf));
        bit_secure_zero(lane_sum, sizeof(lane_sum));
        return -1;
    }

    while (accepted < needed) {
        /* 32 sign bytes and 254 candidates fit in 32 SHAKE256 blocks. */
        size_t candidates;

        if (initial_remaining != 0) {
            candidates = initial_remaining < GAUSS_STREAM_BATCH_CANDIDATES
                             ? initial_remaining
                             : GAUSS_STREAM_BATCH_CANDIDATES;
            initial_remaining -= candidates;
        } else {
            candidates = GAUSS_REFILL_CANDIDATES;
        }

        if (bit_xof256_squeeze(&state, randbuf, candidates * GAUSS_RAND_BYTES) != 0) {
            bit_xof256_zeroize(&state);
            bit_secure_zero(randbuf, sizeof(randbuf));
            bit_secure_zero(lane_sum, sizeof(lane_sum));
            return -1;
        }
        for (size_t i = 0; i < candidates && accepted < needed; i++) {
            uint64_t sample;
            fp96_76 square;
            uint64_t mask;
            int ok = sample_gauss76(&sample, &square, randbuf + i * GAUSS_RAND_BYTES);
            fp96_76 *acc = &lane_sum[i & 3U];

            mask = (uint64_t)-(int64_t)ok;
            acc->limb48[0] += square.limb48[0] & mask;
            acc->limb48[1] += square.limb48[1] & mask;

            if (ok) {
                if (accepted < BIT_N)
                    samples[accepted] = sample;
                accepted++;
            }
        }
    }

    for (size_t i = 0; i < 4; i++) {
        sqsum->limb48[0] += lane_sum[i].limb48[0];
        sqsum->limb48[1] += lane_sum[i].limb48[1];
    }
    fp_renormalize(sqsum);
    bit_xof256_zeroize(&state);
    bit_secure_zero(randbuf, sizeof(randbuf));
    bit_secure_zero(lane_sum, sizeof(lane_sum));
    return 0;
}

uint64_t bit_parabola_norm2_fixed(const bit_parabola_fixed *y)
{
    uint64_t norm2 = 0;
    for (int i = 0; i < BIT_PARABOLA_BLOCKS; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int32_t x = y->coeffs[i][j];
            norm2 += (uint64_t)((int64_t)x * x);
        }
    }
    return norm2;
}

#include "../../common/bernoulli.inc"

__attribute__((aligned(64))) int bit_parabola_sample_fixed(bit_parabola_fixed *y,
                                                           const uint8_t *seed, uint16_t *nonce)
{
    uint64_t samples[BIT_PARABOLA_DIM];
    uint8_t signs[BIT_PARABOLA_DIM / 8];
    const uint64_t proposal_radius = BIT_PARABOLA_PROPOSAL_RADIUS_FIXED;
    const uint64_t proposal_weight3_0 =
        3 * proposal_radius * proposal_radius - BIT_PARABOLA_DIM / 4;

    for (;;) {
        if (*nonce > UINT16_MAX - BIT_PARABOLA_BLOCKS - 1U) {
            bit_secure_zero(samples, sizeof(samples));
            bit_secure_zero(signs, sizeof(signs));
            return -1;
        }
        fp96_76 sqsum = {{0, 0}};
        fp96_76 invsqrt, scale;
        uint64_t norm2;
        uint64_t target_weight = 0;
        uint64_t proposal_weight3;

        for (int i = 0; i < BIT_PARABOLA_BLOCKS; i++) {
            if (sample_gauss_chunk(samples + i * BIT_N, signs + i * (BIT_N / 8), &sqsum, seed,
                                   *nonce, i < BIT_PARABOLA_AUX_DIM) != 0) {
                bit_secure_zero(samples, sizeof(samples));
                bit_secure_zero(signs, sizeof(signs));
                return -1;
            }
            *nonce = (uint16_t)(*nonce + 1U);
        }

        /* fp_invsqrt expects x/2 and converges to 1/sqrt(x). */
        sqsum.limb48[0] += 1ULL;
        sqsum.limb48[0] >>= 1;
        sqsum.limb48[0] += (sqsum.limb48[1] & 1ULL) << 47;
        sqsum.limb48[1] >>= 1;
        fp_renormalize(&sqsum);
        fp_invsqrt(&invsqrt, &sqsum);
        fp_mul_high(&scale, &invsqrt, proposal_radius << (28 - BIT_PARABOLA_FRAC_BITS));

        for (int i = 0; i < BIT_PARABOLA_BLOCKS; i++) {
            for (int j = 0; j < BIT_N; j++) {
                size_t idx = (size_t)i * BIT_N + (size_t)j;
                uint8_t sign = (uint8_t)((signs[idx >> 3] >> (idx & 7U)) & 1U);
                y->coeffs[i][j] = fp_mul_round13(samples[idx], &scale, sign);
            }
        }

        norm2 = bit_parabola_norm2_fixed(y);
        target_weight = (BIT_PARABOLA_L2_FIXED - norm2) &
                        (UINT64_C(0) - (uint64_t)(norm2 < BIT_PARABOLA_L2_FIXED));
        proposal_weight3 = (proposal_weight3_0 - 3 * norm2) &
                           (UINT64_C(0) - (uint64_t)(3 * norm2 < proposal_weight3_0));

        /* The Q13 margin contains each target rounding cell inside the proposal.
         * Its integrated quadratic weight includes the D/12 correction,
         * subject to the fixed-point Gaussian approximation. */
        if ((target_weight == 0) | (proposal_weight3 == 0)) {
            continue;
        }
        int accept = sample_bernoulli_ratio(seed, nonce, 3 * target_weight, proposal_weight3);
        if (accept < 0) {
            bit_secure_zero(samples, sizeof(samples));
            bit_secure_zero(signs, sizeof(signs));
            return -1;
        }
        if (accept != 0) {
            bit_secure_zero(samples, sizeof(samples));
            bit_secure_zero(signs, sizeof(signs));
            return 0;
        }
    }
}

typedef struct {
    uint64_t z_norm2;
    uint64_t v_norm2;
    int64_t dot;
} parabola_fixed_stats;

static void accumulate_response_stats(parabola_fixed_stats *stats, const int32_t mask[BIT_N],
                                      const poly *shift, int32_t shift_scale)
{
    for (int j = 0; j < BIT_N; j++) {
        int64_t vj = (int64_t)shift->coeffs[j] * shift_scale;
        int64_t zj = (int64_t)mask[j] + vj;

        stats->z_norm2 += (uint64_t)(zj * zj);
        stats->v_norm2 += (uint64_t)(vj * vj);
        stats->dot += zj * vj;
    }
}

int bit_parabola_reject_response(const poly *c0, const polyvecl *c_s, const polyveck *c_e,
                                 const bit_parabola_fixed *mask, const uint8_t *seed,
                                 uint16_t *nonce)
{
    const uint64_t beta2 = BIT_PARABOLA_BETA2 * BIT_PARABOLA_SCALE * BIT_PARABOLA_SCALE;
    parabola_fixed_stats stats = {0, 0, 0};
    uint64_t target_weight;
    uint64_t target_twice;
    uint64_t source_twice;
    int64_t source_center;
    int64_t twice_dot;
    int accept;

    /* For z=y+v, the bimodal source weight is w(z-v)+w(z+v),
     * where w(x)=max(L^2-||x||^2,0). Compute one vector-level decision. */
    accumulate_response_stats(&stats, mask->coeffs[0], c0, BIT_PARABOLA_SCALE);
    for (int i = 0; i < BIT_L; i++)
        accumulate_response_stats(&stats, mask->coeffs[1 + i], &c_s->vec[i], BIT_PARABOLA_SCALE);
    for (int i = 0; i < BIT_K; i++)
        accumulate_response_stats(&stats, mask->coeffs[1 + BIT_L + i], &c_e->vec[i],
                                  BIT_PARABOLA_SCALE);

    {
        uint64_t shift_bad = (uint64_t)(stats.v_norm2 > beta2);
        uint64_t support_bad = (uint64_t)(stats.z_norm2 >= BIT_PARABOLA_L2_FIXED - beta2);
        if ((shift_bad | support_bad) != 0)
            return 1;
    }

    target_weight = BIT_PARABOLA_L2_FIXED - beta2 - stats.z_norm2;
    target_twice = 2 * target_weight;
    source_center = (int64_t)(BIT_PARABOLA_L2_FIXED - stats.z_norm2 - stats.v_norm2);
    twice_dot = 2 * stats.dot;

    {
        int64_t w_minus = source_center + twice_dot;
        int64_t w_plus = source_center - twice_dot;
        uint64_t positive_minus = (uint64_t)(w_minus > 0);
        uint64_t positive_plus = (uint64_t)(w_plus > 0);

        source_twice = ((uint64_t)w_minus & (UINT64_C(0) - positive_minus)) +
                       ((uint64_t)w_plus & (UINT64_C(0) - positive_plus));
    }

    /* With ||v||^2 <= beta^2, source_twice >= target_twice > 0. */
    if (target_twice == source_twice) {
        return 0;
    }
    accept = sample_bernoulli_ratio(seed, nonce, target_twice, source_twice);
    if (accept < 0)
        return -1;
    return accept ? 0 : 1;
}
