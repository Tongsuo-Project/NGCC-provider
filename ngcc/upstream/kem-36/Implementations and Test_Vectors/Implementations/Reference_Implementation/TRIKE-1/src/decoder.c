
#include <string.h>
#include <stdlib.h>

#include "decoder.h"
#include "gf2x.h"

// Returns the Hamming weight.
static inline uint32_t get_hamming_weight(const uint8_t *w)
{
    uint32_t weight = 0;
    for (size_t i = 0; i < R_SIZE_BYTES; i++)
    {
        uint32_t byte = w[i];
        byte -= (byte >> 1) & UINT32_C(0x55);
        byte = (byte & UINT32_C(0x33)) + ((byte >> 2) & UINT32_C(0x33));
        weight += (byte + (byte >> 4)) & UINT32_C(0x0f);
    }
    return weight;
}

// Returns the iteration threshold.
static inline uint32_t get_threshold(uint32_t sum_init, uint32_t unsat, size_t iter)
{
    const uint64_t scale = UINT64_C(1000000000000000);
    const uint64_t midpoint = (PARAM_D + 1) / 2;
    const uint64_t init = (COA / 1000) * (uint64_t)sum_init + COB;
    const uint64_t init_tail = (COA % 1000) * (uint64_t)sum_init;
    uint32_t t;

    if (iter == 0)
    {
        t = (uint32_t)((init + (init_tail + 999) / 1000 + scale - 1) / scale);
    }
    else if (iter == 1)
    {
        t = (uint32_t)((2 * init + midpoint * scale + (2 * init_tail + 999) / 1000
                        + 3 * scale - 1) / (3 * scale));
    }
    else if (iter == 2)
    {
        t = (uint32_t)((init + 2 * midpoint * scale + (init_tail + 999) / 1000
                        + 3 * scale - 1) / (3 * scale));
    }
    else
    {
        t = (uint32_t)midpoint;
    }
    t += PARAM_DELTA;

    const uint64_t current = (COA / 1000) * (uint64_t)unsat + COB;
    const uint64_t current_tail = (COA % 1000) * (uint64_t)unsat;
    uint32_t fs = (uint32_t)((current + (current_tail + 999) / 1000 + scale - 1) / scale);

    uint32_t mask = -(fs < t);

    return (t & mask) | (fs & ~mask);
}

// Counts unsatisfied parity checks.
void calc_upc_block(
    const uint8_t *s, const uint32_t *idx, uint32_t *unsat_cnt)
{
    for (size_t i = 0; i < PARAM_R; i++)
    {
        uint32_t count = 0;
        for (size_t k = 0; k < PARAM_D; k++)
        {
            uint32_t idx_to = (i + idx[k]) % PARAM_R;
            count += (s[idx_to >> 3] >> (idx_to & 7)) & 1;
        }
        unsat_cnt[i] = count;
    }
}

// Flips bits meeting the threshold.
void flip_bits(uint8_t *e, const uint32_t *unsat_cnt, uint32_t threshold)
{
    size_t idx = 0;
    for (size_t i = 0; i < R_SIZE_BYTES; i++)
    {
        uint8_t byte = 0;
        for (size_t j = 0; j < 8 && idx < PARAM_R; j++, idx++)
        {
            byte |= (uint8_t)(unsat_cnt[idx] >= threshold) << j;
        }
        e[i] ^= byte;
    }
}

// Recomputes the syndrome.
int calculate_syndrome(
    uint8_t *s, const uint8_t *s_init, const uint8_t *e0, const uint8_t *e1, const uint8_t *e2,
    const uint32_t *idx0, const uint32_t *idx1, const uint32_t *idx2)
{
    memcpy(s, s_init, R_SIZE_BYTES);
    uint8_t *temp = (uint8_t *)calloc(R_ZMM_SIZE_BYTES, sizeof(uint8_t));
    int rc = -1;
    if (temp == NULL) return -1;

    for (size_t k = 0; k < PARAM_D; k++)
    {
        if (gf2x_shift(temp, e0, idx0[k])) goto cleanup;
        gf2x_add(s, s, temp);
    }

    for (size_t k = 0; k < PARAM_D; k++)
    {
        if (gf2x_shift(temp, e1, idx1[k])) goto cleanup;
        gf2x_add(s, s, temp);
    }

    for (size_t k = 0; k < PARAM_D; k++)
    {
        if (gf2x_shift(temp, e2, idx2[k])) goto cleanup;
        gf2x_add(s, s, temp);
    }

    rc = 0;
cleanup:
    free(temp);
    return rc;
}

// Reference bit-flipping decoder.
int bit_flip_decode(
    uint8_t *e0, uint8_t *e1, uint8_t *e2,
    const uint8_t *s_init, const uint32_t *idx0, const uint32_t *idx1, const uint32_t *idx2)
{
    uint32_t *unsat_cnt = (uint32_t *)calloc(PARAM_N, sizeof(uint32_t));
    uint8_t *s = (uint8_t *)calloc(R_ZMM_SIZE_BYTES, sizeof(uint8_t));
    int rc = -1;
    if (unsat_cnt == NULL || s == NULL)
    {
        free(unsat_cnt);
        free(s);
        return -1;
    }

    memcpy(s, s_init, R_ZMM_SIZE_BYTES);
    uint32_t sum_init = get_hamming_weight(s);
    uint32_t unsat = sum_init;

    for (size_t iter = 0; iter < BIT_FLIP_ITER; iter++)
    {
        uint32_t threshold = get_threshold(sum_init, unsat, iter);

        calc_upc_block(s, idx0, unsat_cnt);
        calc_upc_block(s, idx1, unsat_cnt + PARAM_R);
        calc_upc_block(s, idx2, unsat_cnt + PARAM_R * 2);

        flip_bits(e0, unsat_cnt, threshold);
        flip_bits(e1, unsat_cnt + PARAM_R, threshold);
        flip_bits(e2, unsat_cnt + PARAM_R * 2, threshold);

        if (calculate_syndrome(s, s_init, e0, e1, e2, idx0, idx1, idx2)) goto cleanup;
        unsat = get_hamming_weight(s);
    }

    rc = 0;
cleanup:
    free(unsat_cnt);
    free(s);
    return rc;
}

// Decodes the error vector.
int decode(
    uint8_t *e0, uint8_t *e1, uint8_t *e2,
    const uint8_t *s, const uint32_t *idx0, const uint32_t *idx1, const uint32_t *idx2)
{
    return bit_flip_decode(e0, e1, e2, s, idx0, idx1, idx2);
}
