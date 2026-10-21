#include "sampler.h"
#include "params.h"
#include "symmetric.h"
#include "sampler_internal.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

static void clear_bytes(void *p, size_t n)
{
    volatile uint8_t *q = p;
    while (n--) *q++ = 0;
}

/* Sample the fixed global counts by partial Fisher-Yates permutation. */
void sample_fixed_weight(poly_crt_vec *r_crt_vec, poly_sparse *r_sparse_vec, const unsigned char *seed, unsigned char nonce)
{
    uint8_t initial_buf[2u * LORE_K * LORE_HWT_TOTAL + 128u];
    size_t buf_size = sizeof initial_buf;
    uint8_t *buf = initial_buf;
    uint16_t positions[LORE_K * LORE_N];

    prf(buf, buf_size, seed, nonce);
    size_t buf_pos = 0;

    memset(r_crt_vec, 0, sizeof(poly_crt_vec));
    for (int i = 0; i < LORE_K; ++i) {
        memset(&r_sparse_vec[i], 0, sizeof(poly_sparse));
    }

    const int hwt_counts[] = {LORE_HWT_P1, LORE_HWT_M1, LORE_HWT_P2, LORE_HWT_M2};
    const int16_t hwt_values[] = {1, -1, 2, -2};

    for(uint16_t j = 0; j < LORE_K * LORE_N; ++j) {
        positions[j] = j;
    }
    uint16_t current_len = LORE_K * LORE_N;

    for (int type = 0; type < 4; ++type) {
        int16_t total_count = (int16_t)(hwt_counts[type] * LORE_K);
        int16_t value = hwt_values[type];

        for (int j = 0; j < total_count; ++j) {

            uint16_t rand_val;
            uint16_t choice_idx;

            do {
                if (buf_pos + 2 > buf_size) {
                    size_t new_size = buf_size * 2;
                    uint8_t *next = malloc(new_size);
                    if (next == NULL) abort();
                    prf(next, new_size, seed, nonce);
                    clear_bytes(buf, buf_size);
                    if (buf != initial_buf) free(buf);
                    buf = next;
                    buf_size = new_size;
                }
                rand_val = (uint16_t)(buf[buf_pos] | ((uint16_t)buf[buf_pos + 1] << 8));
                buf_pos += 2;
            } while (!lore_sample_index16(rand_val, current_len, &choice_idx));

            uint16_t global_pos = positions[choice_idx];

            positions[choice_idx] = positions[current_len - 1];
            current_len--;

            int poly_idx = global_pos / LORE_N;
            uint16_t local_pos = global_pos % LORE_N;
            poly_crt *crt = &r_crt_vec->vec[poly_idx];
            poly_sparse *sparse = &r_sparse_vec[poly_idx];

            crt->q_poly.coeffs[local_pos] = value;
            crt->t_poly.coeffs[local_pos] = value;

            sparse->pos[sparse->n_coeffs] = local_pos;
            sparse->val[sparse->n_coeffs] = (int8_t)value;
            sparse->n_coeffs++;
        }
    }

    /* These buffers contain the secret permutation and unused PRF output. */
    clear_bytes(buf, buf_size);
    if (buf != initial_buf) free(buf);
    clear_bytes(positions, sizeof positions);
}


