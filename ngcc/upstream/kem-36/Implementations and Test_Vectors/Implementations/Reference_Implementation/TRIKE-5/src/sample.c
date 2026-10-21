#include <string.h>
#include <stdlib.h>

#include "sample.h"
#include "drng.h"

// Checks repeated within-block distances and overlaps between pairs of blocks.
int trike_weak_key_test(const uint32_t *a, const uint32_t *b, const uint32_t *c,
                        uint32_t cycle_limit, uint32_t overlap_limit)
{
    const uint32_t *blocks[3] = {a, b, c};
    if (a == NULL || b == NULL || c == NULL) return -1;
    uint8_t *counts = calloc(PARAM_R, 1);
    if (counts == NULL) return -1;
    unsigned reject = 0;
    for (size_t block = 0; block < 3; ++block) {
        uint64_t score = 0;
        memset(counts, 0, PARAM_R);
        for (size_t i = 0; i < PARAM_D; ++i) {
            for (size_t j = i + 1; j < PARAM_D; ++j) {
                uint32_t x = blocks[block][i], y = blocks[block][j];
                uint32_t distance = x > y ? x - y : y - x;
                if (distance > PARAM_R / 2) distance = PARAM_R - distance;
                score += counts[distance]++;
            }
        }
        reject |= score > cycle_limit;
    }
    for (size_t left = 0; left < 3; ++left) {
        for (size_t right = left + 1; right < 3; ++right) {
            memset(counts, 0, PARAM_R);
            for (size_t i = 0; i < PARAM_D; ++i) {
                for (size_t j = 0; j < PARAM_D; ++j) {
                    uint32_t shift = blocks[right][j] + PARAM_R - blocks[left][i];
                    if (shift >= PARAM_R) shift -= PARAM_R;
                    ++counts[shift];
                }
            }
            for (size_t shift = 0; shift < PARAM_R; ++shift)
                reject |= counts[shift] >= overlap_limit;
        }
    }
    free(counts);
    return (int)reject;
}

// Generates w distinct random indices in the range [0, len).
static int generate_random_idx(uint32_t *idx, uint32_t len, uint32_t w, DRNG_ctx *drng)
{
    if (idx == NULL || w > len) return -1;
    for (uint32_t pos = w; pos-- > 0;) {
        uint8_t bytes[4];
        // ICCS takes output lengths in bits; decode the index in little-endian order.
        if (get_random_number(drng, bytes, sizeof(bytes) * 8)) return -1;
        uint32_t random_number = (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 |
                                 (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
        random_number = pos + (uint32_t)((uint64_t)random_number * (len - pos) >> 32);
        uint32_t mask = 0;
        for (uint32_t j = pos + 1; j < w; j++)
            mask |= -(uint32_t)(idx[j] == random_number);
        idx[pos] = (pos & mask) | (random_number & ~mask);
    }
    return 0;
}

// Generates the three secret key components from a seed.
int generate_secret_key(uint8_t *h0, uint8_t *h1, uint8_t *h2,
                        uint32_t *h0_idx, uint32_t *h1_idx, uint32_t *h2_idx,
                        const uint8_t *seed)
{
    DRNG_ctx local_drng = {0};
    int rc = -1;
    if (!h0 || !h1 || !h2 || !h0_idx || !h1_idx || !h2_idx || !seed) return -1;
    if (init_random_number(&local_drng, seed, M_SIZE_BYTES)) goto done;
    for (;;) {
        if (generate_random_idx(h0_idx, PARAM_R, PARAM_D, &local_drng) ||
            generate_random_idx(h1_idx, PARAM_R, PARAM_D, &local_drng) ||
            generate_random_idx(h2_idx, PARAM_R, PARAM_D, &local_drng)) goto done;
        int weak = trike_weak_key_test(h0_idx, h1_idx, h2_idx, PARAM_S, MAX_OVERLAP);
        if (weak < 0) goto done;
        if (weak) continue;
        break;
    }
    for (size_t i = 0; i < PARAM_D; i++) {
        h0[h0_idx[i] >> 3] |= (1u << (h0_idx[i] & 7));
        h1[h1_idx[i] >> 3] |= (1u << (h1_idx[i] & 7));
        h2[h2_idx[i] >> 3] |= (1u << (h2_idx[i] & 7));
    }
    rc = 0;
done:
    return rc;
}

// Adjusts the last valid bit to give the vector the requested parity.
static void set_hamming_weight(uint8_t *vec, uint8_t parity)
{
    size_t low_bits = (PARAM_R - 1) & 7;
    vec[R_SIZE_BYTES - 1] &= ((1u << low_bits) - 1);
    for (size_t i = 0; i < R_SIZE_BYTES; i++) {
        uint8_t value = vec[i];
        value ^= value >> 4;
        value ^= value >> 2;
        value ^= value >> 1;
        parity ^= value & 1;
    }
    vec[R_SIZE_BYTES - 1] |= (parity << low_bits);
}

// Generates t1, t2, and r1 from a hash-based DRNG stream.
int generate_hash_vectors(uint8_t *t1, uint8_t *t2, uint8_t *r1, const uint8_t *seed)
{
    DRNG_ctx local_drng = {0};
    int rc = -1;
    if (!t1 || !t2 || !r1 || !seed) return -1;
    if (init_random_number(&local_drng, seed, M_SIZE_BYTES) ||
        get_random_number(&local_drng, t1, R_SIZE_BYTES * 8) ||
        get_random_number(&local_drng, t2, R_SIZE_BYTES * 8) ||
        get_random_number(&local_drng, r1, R_SIZE_BYTES * 8)) goto done;
    set_hamming_weight(t1, 0);
    set_hamming_weight(t2, 0);
    set_hamming_weight(r1, 1);
    rc = 0;
done:
    return rc;
}

// Generates the error vector from the message, r2 value, and salt.
int generate_error_vector(uint8_t *e0, uint8_t *e1, uint8_t *e2,
                          const uint8_t *msg, const uint8_t *r2, const uint8_t *salt)
{
    DRNG_ctx local_drng = {0};
    uint8_t hash_input[M_SIZE_BYTES + R_SIZE_BYTES + SALT_SIZE_BYTES];
    uint32_t idx[PARAM_T];
    int rc = -1;
    if (!e0 || !e1 || !e2 || !msg || !r2 || !salt) return -1;
    memcpy(hash_input, msg, M_SIZE_BYTES);
    memcpy(hash_input + M_SIZE_BYTES, r2, R_SIZE_BYTES);
    memcpy(hash_input + M_SIZE_BYTES + R_SIZE_BYTES, salt, SALT_SIZE_BYTES);
    if (init_random_number(&local_drng, hash_input, sizeof(hash_input)) ||
        generate_random_idx(idx, PARAM_N, PARAM_T, &local_drng)) goto done;
    for (uint32_t i = 0; i < PARAM_T; ++i)
        idx[i] = (idx[i] / PARAM_R) * (64 * R_SIZE_64) + idx[i] % PARAM_R;
    // Load each support position unconditionally before selecting bits.
    const volatile uint32_t *positions = idx;
    // Process eight public word positions per scan of the support.
    for (uint32_t word = 0; word < R_SIZE_64; word += 8) {
        uint64_t value0[8] = {0}, value1[8] = {0}, value2[8] = {0};
        for (uint32_t i = 0; i < PARAM_T; ++i) {
            uint32_t pos = positions[i];
            uint64_t bit = UINT64_C(1) << (pos & 63);
            uint32_t from = pos >> 6;
            for (uint32_t lane = 0; lane < 8; ++lane) {
                uint32_t to = word + lane;
                value0[lane] |= bit & (0 - (uint64_t)(from == to));
                value1[lane] |= bit & (0 - (uint64_t)(from == to + R_SIZE_64));
                value2[lane] |= bit & (0 - (uint64_t)(from == to + 2 * R_SIZE_64));
            }
        }
        for (uint32_t lane = 0; lane < 8 && word + lane < R_SIZE_64; ++lane) {
            for (uint32_t byte = 0; byte < 8 && 8 * (word + lane) + byte < R_SIZE_BYTES; ++byte) {
                e0[8 * (word + lane) + byte] |= (uint8_t)(value0[lane] >> (8 * byte));
                e1[8 * (word + lane) + byte] |= (uint8_t)(value1[lane] >> (8 * byte));
                e2[8 * (word + lane) + byte] |= (uint8_t)(value2[lane] >> (8 * byte));
            }
        }
    }
    rc = 0;
done:
    return rc;
}
