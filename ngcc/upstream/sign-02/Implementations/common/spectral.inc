/* Copyright (c) 2026 Hang Zhang. */
#include "spectral.h"
#include "symmetric.h"
#include "spectral_roots.h"

#if BIT_N > 1024 || BIT_L + BIT_K != 6
#error "Spectral arithmetic bounds require N <= 1024 and six secret blocks"
#endif

#define SPECTRAL_Q30 (INT64_C(1) << 30)
#define SPECTRAL_Q40 (UINT64_C(1) << 40)

static int32_t spectral_cos(unsigned index)
{
    index &= 2047U;
    if (index <= 512U)
        return bit_spectral_cos_q30[index];
    if (index <= 1024U)
        return -bit_spectral_cos_q30[1024U - index];
    if (index <= 1536U)
        return -bit_spectral_cos_q30[index - 1024U];
    return bit_spectral_cos_q30[2048U - index];
}

static uint64_t spectral_abs(int32_t value)
{
    uint32_t mask = 0U - (uint32_t)(value < 0);
    return ((uint32_t)value ^ mask) - mask;
}

static void spectral_accumulate(uint64_t energy[BIT_N], const poly *a, uint32_t error)
{
    int32_t re[BIT_N], im[BIT_N];
    for (unsigned j = 0; j < BIT_N; j++) {
        unsigned phase = j * (1024U / BIT_N);
        int32_t cosine = spectral_cos(phase);
        int32_t sine = -spectral_cos(2560U - phase);
        re[j] = (int32_t)((int64_t)a->coeffs[j] * cosine / 1024);
        im[j] = (int32_t)((int64_t)a->coeffs[j] * sine / 1024);
    }
    for (unsigned i = 1, j = 0; i < BIT_N; i++) {
        unsigned bit = BIT_N >> 1;
        for (; (j & bit) != 0; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j) {
            int32_t value = re[i]; re[i] = re[j]; re[j] = value;
            value = im[i]; im[i] = im[j]; im[j] = value;
        }
    }
    for (unsigned len = 2; len <= BIT_N; len <<= 1) {
        for (unsigned i = 0; i < BIT_N; i += len) {
            for (unsigned j = 0; j < len / 2; j++) {
                unsigned u = i + j, v = u + len / 2;
                unsigned phase = j * (2048U / len);
                int32_t cosine = spectral_cos(phase);
                int32_t sine = -spectral_cos(2560U - phase);
                int32_t vr = (int32_t)(((int64_t)re[v] * cosine -
                                        (int64_t)im[v] * sine) / SPECTRAL_Q30);
                int32_t vi = (int32_t)(((int64_t)re[v] * sine +
                                        (int64_t)im[v] * cosine) / SPECTRAL_Q30);
                int32_t ur = re[u], ui = im[u];
                re[u] = ur + vr; im[u] = ui + vi;
                re[v] = ur - vr; im[v] = ui - vi;
            }
        }
    }
    for (unsigned j = 0; j < BIT_N; j++) {
        uint64_t real = spectral_abs(re[j]) + error;
        uint64_t imaginary = spectral_abs(im[j]) + error;
        energy[j] += real * real + imaginary * imaginary;
    }
    bit_secure_zero(re, sizeof(re));
    bit_secure_zero(im, sizeof(im));
}

uint64_t bit_keygen_spectral_bound_sq(const polyvecl *s_0, const polyveck *e)
{
    uint64_t energy[BIT_N] = {0};
    uint64_t top[BIT_N / BIT_TAU + 1] = {0};
    uint64_t weighted = 0;
    uint32_t error = 2;
    /* A Q30 root component errs by at most 2^-31. Each stage has one
     * truncation per rotation component and |cos|+|sin| <= 3/2. */
    for (unsigned len = 2; len <= BIT_N; len <<= 1)
        error += (3U * error + 1U) / 2U + 2U;

    for (unsigned i = 0; i < BIT_L; i++)
        spectral_accumulate(energy, &s_0->vec[i], error);
    for (unsigned i = 0; i < BIT_K; i++)
        spectral_accumulate(energy, &e->vec[i], error);

    /* Six blocks fit uint64_t before division: at N=1024 the bound is
     * 12*(2^30+2*32552)^2 < 2^64. Round before multiplying by tau^2. */
    for (unsigned j = 0; j < BIT_N; j++) {
        uint64_t value = energy[j] / SPECTRAL_Q40 + (energy[j] % SPECTRAL_Q40 != 0);
        for (unsigned k = 0; k <= BIT_N / BIT_TAU; k++) {
            uint64_t mask = UINT64_C(0) - (uint64_t)(value > top[k]);
            uint64_t swap = (value ^ top[k]) & mask;
            top[k] ^= swap;
            value ^= swap;
        }
    }
    for (unsigned k = 0; k < BIT_N / BIT_TAU; k++)
        weighted += (uint64_t)BIT_TAU * BIT_TAU * top[k];
    weighted += (uint64_t)BIT_TAU * (BIT_N % BIT_TAU) * top[BIT_N / BIT_TAU];
    uint64_t bound = BIT_TAU + weighted / BIT_N + (weighted % BIT_N != 0);
    bit_secure_zero(energy, sizeof(energy));
    bit_secure_zero(top, sizeof(top));
    return bound;
}
