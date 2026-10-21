/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */

#include <stdint.h>
#include "ntt.h"
#include "reduce.h"

#if BIT_Q != 7681 || (BIT_N != 256 && BIT_N != 512)
#error "ntt_ref.c supports q=7681 with N=256 or incomplete N=512"
#endif

#if 4 * BIT_Q > 32768
#error "lazy NTT requires every value in [0,4q) to fit in int16_t"
#endif

#define Beta 65536
#define INVERSE_Q -7679
#define INVERSE_N_BETA 256
#define MONT_R2 5569
#define NTT_COMPONENTS 256

static const int16_t M[BIT_N] = {
    4088, 3904, 4056, 3182, 5487, 5225, 1100, 3696, 1414, 5980, 2876, 5362, 5186, 834,  5431, 121,
    7064, 1921, 2830, 3364, 6385, 1483, 1525, 5124, 3706, 2006, 6082, 5688, 5444, 5695, 2816, 2088,
    4877, 103,  2043, 6250, 6360, 1399, 7167, 4725, 7132, 1535, 3153, 5371, 5146, 3772, 5241, 2555,
    7285, 4507, 5800, 4126, 438,  7002, 5921, 6376, 810,  5794, 7,    638,  3992, 1738, 3600, 4415,
    2665, 5882, 6898, 1056, 5548, 4201, 3310, 6513, 2579, 6822, 2649, 5521, 1919, 1532, 7195, 7277,
    727,  1521, 1533, 1464, 6295, 4253, 4938, 6760, 3750, 4919, 6291, 6083, 2835, 4917, 3865, 2233,
    5109, 1497, 3145, 1350, 1681, 2883, 2224, 4093, 373,  2175, 3692, 730,  5290, 7021, 5925, 4546,
    3417, 7487, 2789, 2919, 4949, 5568, 2385, 3405, 6627, 5983, 6515, 3456, 2,    6766, 3850, 5255,
    2005, 592,  3763, 2812, 6278, 509,  2937, 4338, 3929, 3677, 5221, 4024, 6012, 6989, 5514, 4394,
    7493, 1519, 6788, 5295, 2070, 5419, 5992, 777,  6669, 2130, 2874, 6277, 6240, 2532, 6597, 4346,
    2345, 6343, 5378, 5166, 83,   4273, 6155, 6855, 1837, 642,  2965, 6890, 4369, 2083, 7311, 293,
    4473, 589,  124,  4718, 3781, 2258, 4518, 6885, 4467, 3334, 3936, 4315, 7279, 7252, 1931, 3723,
    2891, 2340, 4131, 3434, 4367, 2998, 3461, 2719, 2815, 6386, 3770, 3450, 2589, 2247, 6537, 1072,
    2786, 451,  1712, 222,  4134, 5902, 434,  1151, 6172, 2918, 6274, 2339, 4113, 3988, 6095, 3581,
    1121, 5610, 7245, 7445, 7023, 1476, 715,  7011, 151,  3887, 6478, 1181, 4139, 7455, 2378, 2767,
    7664, 3937, 5680, 5259, 2230, 1348, 6752, 6403, 3177, 2072, 1649, 2161, 6611, 5622, 6439, 7502};

static const int16_t Mn[BIT_N] = {
    4088, 3777, 4499, 3625, 3985, 6581, 2456, 2194, 7560, 2250, 6847, 2495, 2319, 4805, 1701, 6267,
    5593, 4865, 1986, 2237, 1993, 1599, 5675, 3975, 2557, 6156, 6198, 1296, 4317, 4851, 5760, 617,
    3266, 4081, 5943, 3689, 7043, 7674, 1887, 6871, 1305, 1760, 679,  7243, 3555, 1881, 3174, 396,
    5126, 2440, 3909, 2535, 2310, 4528, 6146, 549,  2956, 514,  6282, 1321, 1431, 5638, 7578, 2804,
    2426, 3831, 915,  7679, 4225, 1166, 1698, 1054, 4276, 5296, 2113, 2732, 4762, 4892, 194,  4264,
    3135, 1756, 660,  2391, 6951, 3989, 5506, 7308, 3588, 5457, 4798, 6000, 6331, 4536, 6184, 2572,
    5448, 3816, 2764, 4846, 1598, 1390, 2762, 3931, 921,  2743, 3428, 1386, 6217, 6148, 6160, 6954,
    404,  486,  6149, 5762, 2160, 5032, 859,  5102, 1168, 4371, 3480, 2133, 6625, 783,  1799, 5016,
    179,  1242, 2059, 1070, 5520, 6032, 5609, 4504, 1278, 929,  6333, 5451, 2422, 2001, 3744, 17,
    4914, 5303, 226,  3542, 6500, 1203, 3794, 7530, 670,  6966, 6205, 658,  236,  436,  2071, 6560,
    4100, 1586, 3693, 3568, 5342, 1407, 4763, 1509, 6530, 7247, 1779, 3547, 7459, 5969, 7230, 4895,
    6609, 1144, 5434, 5092, 4231, 3911, 1295, 4866, 4962, 4220, 4683, 3314, 4247, 3550, 5341, 4790,
    3958, 5750, 429,  402,  3366, 3745, 4347, 3214, 796,  3163, 5423, 3900, 2963, 7557, 7092, 3208,
    7388, 370,  5598, 3312, 791,  4716, 7039, 5844, 826,  1526, 3408, 7598, 2515, 2303, 1338, 5336,
    3335, 1084, 5149, 1441, 1404, 4807, 5551, 1012, 6904, 1689, 2262, 5611, 2386, 893,  6162, 188,
    3287, 2167, 692,  1669, 3657, 2460, 4004, 3752, 3343, 4744, 7172, 1403, 4869, 3918, 7089, 5676};

static inline int32_t MontgomerymapFull(int32_t a)
{
    uint32_t m;
    int32_t t, r;

    m = ((uint32_t)a * (uint32_t)INVERSE_Q) & 0xFFFF;
    t = (int32_t)(m * (uint32_t)BIT_Q);
    r = (a >> 16) - (t >> 16);
    /* Signed lazy inputs can make the unsigned-radix Montgomery result fall
     * below -q.  The supported |a| < 3q^2 range is above -2q, so two fixed
     * corrections produce the canonical representative. */
    r = r + ((r >> 31) & BIT_Q);
    r = r + ((r >> 31) & BIT_Q);
    return r;
}

static inline int32_t mul_mod(int16_t a, int16_t b)
{
    return MontgomerymapFull((int32_t)a * (int32_t)b);
}

/* Reduce a nonnegative representative in [0, 4q) to [0, q). */
static inline int16_t reduce_4q_to_q(int32_t a)
{
    a -= 2 * BIT_Q;
    a += (a >> 31) & (2 * BIT_Q);
    return reduce_modq(a);
}

static inline int NTT_Lazy(int16_t *a)
{
    int t, m, i, j, s, e;
    unsigned int lazy_layers = 0;
    int32_t U = 0, V = 0;
    int16_t S;

    t = BIT_N;
    for (m = 1; m < NTT_COMPONENTS; m <<= 1) {
        int reduce_layer;

        t = (t >> 1);
        lazy_layers++;
        reduce_layer = lazy_layers == 3 || m == NTT_COMPONENTS / 2;
        for (i = 0; i < m; i++) {
            s = (i * t) << 1;
            e = s + t;
            S = M[m + i];
            for (j = s; j < e; j++) {
                U = a[j];
                V = a[j + t];
                V = mul_mod(V, S);

                /*
                 * V is canonical after Montgomery reduction.  Starting from
                 * [0,q), each lazy layer therefore increases the bound by q:
                 * [0,2q), [0,3q), [0,4q).  Since 4q < INT16_MAX for
                 * q=7681, three layers can be stored safely; a fourth cannot.
                 */
                if (reduce_layer) {
                    a[j] = reduce_4q_to_q(U + V);
                    a[j + t] = reduce_4q_to_q(U - V + BIT_Q);
                } else {
                    a[j] = (int16_t)(U + V);
                    a[j + t] = (int16_t)(U - V + BIT_Q);
                }
            }
        }

        if (reduce_layer) {
            lazy_layers = 0;
        }
    }

    return 0;
}

static inline int INTT_Lazy(int16_t *a)
{
    int t, m, i, j, s, e, h;
    unsigned int lazy_layers = 0;
    int32_t U = 0, V = 0;
    int16_t S;

    t = BIT_N / NTT_COMPONENTS;
    for (m = NTT_COMPONENTS; m > 1; m >>= 1) {
        int reduce_layer;

        s = 0;
        h = m >> 1;
        lazy_layers++;
        reduce_layer = lazy_layers == 2 && m > 2;
        for (i = 0; i < h; i++) {
            e = s + t;
            S = Mn[h + i];
            for (j = s; j < e; j++) {
                U = a[j];
                V = a[j + t];

                /*
                 * A two-layer group starts in [0,q).  Its sums are bounded
                 * by 2q and then 4q.  Adding 2q makes both layers' differences
                 * nonnegative and smaller than 4q, which is a valid int16_t
                 * input to the Montgomery multiplication for q=7681.
                 */
                if (reduce_layer) {
                    a[j] = reduce_4q_to_q(U + V);
                } else {
                    a[j] = (int16_t)(U + V);
                }
                a[j + t] = (int16_t)mul_mod((int16_t)(U - V + 2 * BIT_Q), S);
            }
            s = s + 2 * t;
        }
        t *= 2;

        if (reduce_layer) {
            lazy_layers = 0;
        }
    }

    for (i = 0; i < BIT_N; i++) {
        a[i] = mul_mod(a[i], INVERSE_N_BETA);
    }

    return 0;
}

void ntt_forward(int16_t a[BIT_N])
{
    (void)NTT_Lazy(a);
}

void ntt_inverse(int16_t a[BIT_N])
{
    (void)INTT_Lazy(a);
}

void ntt_basemul_raw(int16_t r[BIT_N], const int16_t a[BIT_N], const int16_t b[BIT_N])
{
    if (BIT_N == NTT_COMPONENTS) {
        for (int i = 0; i < BIT_N; i++)
            r[i] = (int16_t)mul_mod(a[i], b[i]);
        return;
    }

    for (int i = 0; i < BIT_N; i += 4) {
        int16_t zeta = M[BIT_N / 4 + i / 4];
        int32_t a0b0 = mul_mod(a[i], b[i]);
        int32_t a1b1 = mul_mod(mul_mod(a[i + 1], b[i + 1]), zeta);
        int32_t a0b1 = mul_mod(a[i], b[i + 1]);
        int32_t a1b0 = mul_mod(a[i + 1], b[i]);

        r[i] = reduce_modq(a0b0 + a1b1);
        r[i + 1] = reduce_modq(a0b1 + a1b0);

        zeta = (int16_t)(BIT_Q - zeta);
        a0b0 = mul_mod(a[i + 2], b[i + 2]);
        a1b1 = mul_mod(mul_mod(a[i + 3], b[i + 3]), zeta);
        a0b1 = mul_mod(a[i + 2], b[i + 3]);
        a1b0 = mul_mod(a[i + 3], b[i + 2]);
        r[i + 2] = reduce_modq(a0b0 + a1b1);
        r[i + 3] = reduce_modq(a0b1 + a1b0);
    }
}

void ntt_dot3_raw(int16_t r[BIT_N], const int16_t a[BIT_L][BIT_N], const int16_t b[BIT_L][BIT_N])
{
    int16_t product[BIT_N];

    for (int i = 0; i < BIT_N; i++)
        r[i] = 0;
    for (int j = 0; j < BIT_L; j++) {
        ntt_basemul_raw(product, a[j], b[j]);
        for (int i = 0; i < BIT_N; i++) {
            r[i] = (int16_t)(r[i] + product[i]);
        }
    }
}

void ntt_montgomery_lift(int16_t r[BIT_N])
{
    for (int i = 0; i < BIT_N; i++) {
        r[i] = (int16_t)mul_mod(r[i], MONT_R2);
    }
}
