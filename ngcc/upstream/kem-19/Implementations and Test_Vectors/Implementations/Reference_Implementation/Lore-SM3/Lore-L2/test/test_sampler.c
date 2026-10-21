#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../sampler.h"
#include "../sampler_internal.h"
#include "../symmetric.h"

static int check_intervals(void)
{
    uint32_t counts[LORE_K * LORE_N];
    for (unsigned int bound = 1; bound <= LORE_K * LORE_N; ++bound) {
        memset(counts, 0, sizeof counts);
        unsigned int rejected = 0;
        for (unsigned int word = 0; word < 65536; ++word) {
            uint16_t index = 0;
            if (!lore_sample_index16((uint16_t)word, (uint16_t)bound, &index)) {
                ++rejected;
                continue;
            }
            if (index >= bound) return 1;
            ++counts[index];
        }
        if (rejected != 65536u % bound) return 1;
        for (unsigned int j = 0; j < bound; ++j)
            if (counts[j] != 65536u / bound) return 1;
    }
    return 0;
}

/* A large one-shot PRF buffer gives an independent check of all streaming
 * refill boundaries in the sampler. The reference uses division and remainder
 * to check the interval mapping separately from the optimized helper. */
static int check_stream_and_sparse(void)
{
    uint8_t seed[LORE_SYMBYTES], bytes[16384];
    uint16_t positions[LORE_K * LORE_N];
    const unsigned int quantities[4] = {
        LORE_K * LORE_HWT_P1, LORE_K * LORE_HWT_M1,
        LORE_K * LORE_HWT_P2, LORE_K * LORE_HWT_M2
    };
    const int16_t values[4] = {1, -1, 2, -2};
    for (unsigned int trial = 0; trial < 64; ++trial) {
        poly_crt_vec actual, expected = {0};
        poly_sparse sparse[LORE_K];
        for (unsigned int i = 0; i < sizeof seed; ++i)
            seed[i] = (uint8_t)(trial * 17u + i);
        const uint8_t nonce = (uint8_t)(trial == 63 ? 255 : trial);
        prf(bytes, sizeof bytes, seed, nonce);
        sample_fixed_weight(&actual, sparse, seed, nonce);
        for (unsigned int i = 0; i < LORE_K * LORE_N; ++i) positions[i] = (uint16_t)i;
        unsigned int bound = LORE_K * LORE_N;
        size_t cursor = 0;
        for (unsigned int type = 0; type < 4; ++type) {
            for (unsigned int j = 0; j < quantities[type]; ++j) {
                uint32_t word, index, low;
                do {
                    if (cursor + 2 > sizeof bytes) return 1;
                    word = bytes[cursor] | ((uint32_t)bytes[cursor + 1] << 8);
                    cursor += 2;
                    /* Quotient/remainder implementation, separate from helper. */
                    index = (word * bound) / 65536u;
                    low = word * bound - index * 65536u;
                } while (low < 65536u % bound);
                unsigned int pos = positions[index];
                positions[index] = positions[--bound];
                expected.vec[pos / LORE_N].q_poly.coeffs[pos % LORE_N] = values[type];
                expected.vec[pos / LORE_N].t_poly.coeffs[pos % LORE_N] = values[type];
            }
        }
        if (memcmp(&actual, &expected, sizeof actual)) return 1;
        unsigned int total = 0;
        for (unsigned int p = 0; p < LORE_K; ++p) {
            poly reconstructed;
            poly_from_sparse(&reconstructed, &sparse[p]);
            if (memcmp(&reconstructed, &actual.vec[p].t_poly, sizeof reconstructed)) return 1;
            total += (unsigned int)sparse[p].n_coeffs;
        }
        if (total != LORE_K * LORE_HWT_TOTAL) return 1;
    }
    return 0;
}

int main(void)
{
    if (check_intervals()) {
        fputs("interval sampler has unequal preimage counts\n", stderr);
        return 1;
    }
    if (check_stream_and_sparse()) {
        fputs("sampler stream, permutation, or sparse representation mismatch\n", stderr);
        return 1;
    }
    puts("Sampler: exhaustive uniform interval mapping and 64 stream/sparse checks passed");
    return 0;
}
