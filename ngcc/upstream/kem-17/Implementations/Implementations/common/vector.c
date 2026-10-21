/**
 * @file vector.c
 * @brief Implementation of vectors sampling and some utilities for the HEP-QC scheme
 */

#include "vector.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "parameters.h"
#include <auxfunc.h>
#include <drng.h>

static inline uint32_t compare_u32(const uint32_t v1, const uint32_t v2);

/**
 * @brief Constant-time comparison of two integers v1 and v2
 *
 * Returns 1 if v1 is equal to v2 and 0 otherwise
 * https://gist.github.com/sneves/10845247
 *
 * @param[in] v1
 * @param[in] v2
 */
static inline uint32_t compare_u32(const uint32_t v1, const uint32_t v2) {
    return 1 ^ (((v1 - v2) | (v2 - v1)) >> 31);
}

/**
 * @brief Constant-time Barrett reduction modulo PARAM_N.
 *
 * Reduces \p x modulo PARAM_N using the precomputed value PARAM_N_MU = ⌊2^32 / PARAM_N⌋.
 *
 * @param[in] x Input value to reduce.
 * @return x mod PARAM_N in constant time.
 */
static inline uint32_t barrett_reduce(uint32_t x) {
    uint64_t q = ((uint64_t)x * PARAM_N_MU) >> 32;
    uint32_t r = x - (uint32_t)(q * PARAM_N);

    uint32_t reduce_flag = (((r - PARAM_N) >> 31) ^ 1);
    uint32_t mask = -reduce_flag;
    r -= mask & PARAM_N;
    return r;
}

/**
 * @brief Generates a random support set with uniform and unbiased sampling.
 *
 * This function implements a rejection sampling algorithm to generate `weight`
 * distinct indices uniformly at random from the interval [0, PARAM_N).
 * It ensures that the output is non-biased and the values are uniformly distributed.
 *
 * Internally, it samples 24-bit random values and rejects any value ≥ UTILS_REJECTION_THRESHOLD,
 * where the threshold is precomputed as:
 * \f[
 * t = \left\lfloor \frac{2^{24}}{\text{PARAM\_N}} \right\rfloor \times \text{PARAM\_N}
 * \f]
 *
 * @param[in,out] ctx     SHAKE256 XOF context used for random byte generation.
 * @param[out]    support Output array to store the `weight` unique indices.
 * @param[in]     weight  Desired Hamming weight.
 */
void vect_generate_random_support1(shake256_xof_ctx *ctx, uint32_t *support, uint16_t weight) {
    uint8_t rand_bytes[3] = {0};
    uint32_t candidate = 0;

    for (size_t i = 0; i < weight;) {
        xof_get_bytes(ctx, rand_bytes, 3);
        candidate = rand_bytes[0] | (rand_bytes[1] << 8) | (rand_bytes[2] << 16);

        if (candidate >= UTILS_REJECTION_THRESHOLD) {
            continue;
        }
        candidate = barrett_reduce(candidate);

        int is_position_available = 1;
        for (size_t j = 0; j < i; j++) {
            if (candidate == support[j]) {
                is_position_available = 0;
                break;
            }
        }

        if (is_position_available == 1) {
            support[i] = candidate;
            i++;
        }
    }
}

/**
 * @brief Generates a random support set of distinct indices.
 *
 * This implements the **GenerateRandomSupport** algorithm from the specification.
 *
 * @param[in,out] ctx     Initialized SHAKE256 XOF context used for randomness.
 * @param[out]    support Output array of unique indices (the support set).
 * @param[in]     weight  Number of elements to generate (Hamming weight).
 */
void vect_generate_random_support2(shake256_xof_ctx *ctx, uint32_t *support, uint16_t weight) {
    uint32_t rand_u32[PARAM_OMEGA_R] = {0};

    xof_get_bytes(ctx, (uint8_t *)&rand_u32, 4 * weight);

    for (size_t i = 0; i < weight; ++i) {
        uint64_t buff = rand_u32[i];
        support[i] = i + ((buff * (PARAM_N - i)) >> 32);
    }

    for (int32_t i = (weight - 1); i-- > 0;) {
        uint32_t found = 0;

        for (size_t j = i + 1; j < weight; ++j) {
            found |= compare_u32(support[j], support[i]);
        }

        uint32_t mask = -found;
        support[i] = (mask & i) ^ (~mask & support[i]);
    }
}

/**
 * @brief Sets bits in a vector based on a support set.
 *
 * Writes `weight` positions from `support` into the bit-vector `v`.
 * Each index in `support` sets a corresponding bit in `v`.
 *
 * @param[out] v       Output vector.
 * @param[in]  support Array of bit indices to set.
 * @param[in]  weight  Number of positions to set.
 */
void vect_write_support_to_vector(uint64_t *v, uint32_t *support, uint16_t weight) {
    uint32_t index_tab[PARAM_OMEGA_R] = {0};
    uint64_t bit_tab[PARAM_OMEGA_R] = {0};

    for (size_t i = 0; i < weight; i++) {
        index_tab[i] = support[i] >> 6;
        int32_t pos = support[i] & 0x3f;
        bit_tab[i] = ((uint64_t)1) << pos;
    }

    uint64_t val = 0;
    for (uint32_t i = 0; i < VEC_N_SIZE_64; i++) {
        val = 0;
        for (uint32_t j = 0; j < weight; j++) {
            uint32_t tmp = i - index_tab[j];
            int val1 = 1 ^ ((tmp | -tmp) >> 31);
            uint64_t mask = -val1;
            val |= (bit_tab[j] & mask);
        }
        v[i] |= val;
    }
}

/**
 * @brief Generate a random binary vector of fixed Hamming weight.
 *
 * It samples a binary vector with exactly `weight` bits set to 1, where the
 * positions are chosen **uniformly at random** without bias.
 *
 * This sampling procedure is used **exclusively during key generation** to generate
 * the vectors **x** and **y**
 *
 * @param[in,out] ctx     Pointer to a previously initialized SHAKE-256 XOF context.
 * @param[out]    v       Pointer to an array of ⌈PARAM_N/64⌉ 64-bit words.
 *                        On return, **v** is a bitmask with exactly `weight`
 *                        bits set to 1.
 * @param[in]     weight  Desired Hamming weight.
 */
void vect_sample_fixed_weight1(shake256_xof_ctx *ctx, uint64_t *v, uint16_t weight) {
    uint32_t support[PARAM_OMEGA_R] = {0};
    vect_generate_random_support1(ctx, support, weight);
    vect_write_support_to_vector(v, support, weight);
}

/**
 * @brief Generate a random binary vector of fixed Hamming weight.
 *
 * Implementation of Algorithm 5 in https://eprint.iacr.org/2021/1631.pdf
 * This sampling procedure is used **exclusively during encryption** to generate
 * the vectors **r1**, **r2**, and **e**.
 *
 * @param[in,out] ctx     Pointer to a previously initialized SHAKE-256 XOF context.
 * @param[out]    v       Pointer to an array of ⌈PARAM_N/64⌉ 64-bit words.
 *                        On return, **v** is a mask with exactly **weight**
 *                        bits set to 1.
 * @param[in]     weight  Desired Hamming weight.
 */
void vect_sample_fixed_weight2(shake256_xof_ctx *ctx, uint64_t *v, uint16_t weight) {
    uint32_t support[PARAM_OMEGA_R] = {0};
    vect_generate_random_support2(ctx, support, weight);
    vect_write_support_to_vector(v, support, weight);
}

/**
 * @brief Generates a random vector of dimension <b>PARAM_N</b>
 *
 * This function generates a random binary vector of dimension <b>PARAM_N</b>. It generates a random
 * array of bytes using the xof, and drop the extra bits using a mask.
 *
 * @param[in] ctx Pointer to the context of the xof
 * @param[in] v Pointer to an array
 */
void vect_set_random(shake256_xof_ctx *ctx, uint64_t *v) {
    xof_get_bytes(ctx, (uint8_t *)v, VEC_N_SIZE_BYTES);
    v[VEC_N_SIZE_64 - 1] &= BITMASK(PARAM_N, 64);
}

/**
 * @brief Adds two vectors
 *
 * @param[out] o Pointer to an array that is the result
 * @param[in] v1 Pointer to an array that is the first vector
 * @param[in] v2 Pointer to an array that is the second vector
 * @param[in] size Integer that is the size of the vectors
 */
void vect_add(uint64_t *o, const uint64_t *v1, const uint64_t *v2, uint32_t size) {
    for (uint32_t i = 0; i < size; ++i) {
        o[i] = v1[i] ^ v2[i];
    }
}

/**
 * @brief Compares two vectors
 *
 * Function borrowed from liboqs:
 * https://github.com/open-quantum-safe/liboqs/commit/cce1bfde4e52c524b087b9687020d283fbde0f24
 *
 * @param[in] v1 Pointer to an array that is first vector
 * @param[in] v2 Pointer to an array that is second vector
 * @param[in] size Integer that is the size of the vectors
 * @returns 0 if the vectors are equals and 1 otherwise
 */
uint8_t vect_compare(const uint8_t *v1, const uint8_t *v2, uint32_t size) {
    uint16_t r = 0x0100;

    for (size_t i = 0; i < size; i++) {
        r |= v1[i] ^ v2[i];
    }

    return (r - 1) >> 8;
}

/**
 * @brief Prints a given number of bytes
 *
 * @param[in] v Pointer to an array of bytes
 * @param[in] size Integer that is number of bytes to be displayed
 */
void vect_print(const uint64_t *v, const uint32_t size) {
    if (size == VEC_K_SIZE_BYTES) {
        uint8_t tmp[VEC_K_SIZE_BYTES] = {0};
        memcpy(tmp, v, VEC_K_SIZE_BYTES);
        for (uint32_t i = 0; i < VEC_K_SIZE_BYTES; ++i) {
            printf("%02x", tmp[i]);
        }
    } else if (size == VEC_N_SIZE_BYTES) {
        uint8_t tmp[VEC_N_SIZE_BYTES] = {0};
        memcpy(tmp, v, VEC_N_SIZE_BYTES);
        for (uint32_t i = 0; i < VEC_N_SIZE_BYTES; ++i) {
            printf("%02x", tmp[i]);
        }
    } else if (size == VEC_N1N2_SIZE_BYTES) {
        uint8_t tmp[VEC_N1N2_SIZE_BYTES] = {0};
        memcpy(tmp, v, VEC_N1N2_SIZE_BYTES);
        for (uint32_t i = 0; i < VEC_N1N2_SIZE_BYTES; ++i) {
            printf("%02x", tmp[i]);
        }
    } else if (size == VEC_N1_SIZE_BYTES) {
        uint8_t tmp[VEC_N1_SIZE_BYTES] = {0};
        memcpy(tmp, v, VEC_N1_SIZE_BYTES);
        for (uint32_t i = 0; i < VEC_N1_SIZE_BYTES; ++i) {
            printf("%02x", tmp[i]);
        }
    }
}


/**
 * @brief Xor two vectors
 *
 * @param[in,out] dst   the result vector
 * @param[in] src       another vector
 */
static void matrix_row_xor_binary(uint8_t *dst, const uint8_t *src, uint32_t len) {
    for (uint32_t i = 0; i < len; ++i) {
        dst[i] ^= src[i];
    }
}

/**
 * @brief Check if the matrix mat is invertible
 *
 * @param[in] mat       the matrix waiting to be checked
 * @param[in] len       the number of rows of the matrix
 * @returns             1 if the matrix is invertible and 0 otherwise
 */
static uint8_t matrix_is_invertible_binary(uint8_t mat[MSG_BIT][MSG_BIT], uint32_t len) {
    uint8_t work[len][len];

    for (uint32_t r = 0; r < len; ++r) {
        for (uint32_t c = 0; c < len; ++c) {
            work[r][c] = mat[r][c];
        }
    }

    for (uint32_t col = 0; col < len; ++col) {
        uint32_t pivot = col;
        while (pivot < len && work[pivot][col] == 0u) {
            ++pivot;
        }

        if (pivot == len) {
            return 0u;
        }

        if (pivot != col) {
            for (uint32_t c = 0; c < len; ++c) {
                uint8_t tmp = work[col][c];
                work[col][c] = work[pivot][c];
                work[pivot][c] = tmp;
            }
        }

        for (uint32_t row = col + 1; row < len; ++row) {
            if (work[row][col] != 0u) {
                matrix_row_xor_binary(work[row], work[col], len);
            }
        }
    }

    return 1u;
}

/**
 * @brief Generating a random MSG_BIT * MSG_BIT matrix until it is invertible
 *
 * @param[in,out] ctx   Pointer to a previously initialized SHAKE-256 XOF context
 * @param[in] mat       the matrix waiting to be checked
 * @param[in] len       the number of rows of the matrix
 */
void matrix_set_random(shake256_xof_ctx *ctx, uint8_t v[MSG_BIT][MSG_BIT], uint32_t len) {
    const uint32_t row_bytes = len >> 3;
    uint8_t row_random[row_bytes];

    do {
        for (uint32_t row = 0; row < len; ++row) {
            xof_get_bytes(ctx, row_random, row_bytes);

            for (uint32_t col = 0; col < len; ++col) {
                const uint32_t byte_index = col >> 3;
                const uint32_t bit_index = col & 7u;
                v[row][col] = (uint8_t)((row_random[byte_index] >> bit_index) & 1u);
            }
        }
    } while (matrix_is_invertible_binary(v, len) == 0u);
}

/**
 * @brief Generating the inverse matrix of a specific matrix
 *
 * During this process, we assume that the matrix mat is invertible.
 * 
 * @param[out] work       the inverse matrix of mat
 * @param[in] mat         the specific matrix
 * @param[in] len         the number of rows of the matrix
 */
void invertible_matrix(uint8_t work[MSG_BIT][MSG_BIT], uint8_t mat[MSG_BIT][MSG_BIT], uint32_t len) {
    uint8_t tmp[MSG_BIT][MSG_BIT];

    for (uint32_t r = 0; r < len; ++r) {
        for (uint32_t c = 0; c < len; ++c) {
            tmp[r][c] = mat[r][c];
            work[r][c] = (uint8_t)(r == c);
        }
    }

    for (uint32_t col = 0; col < len; ++col) {
        uint32_t pivot = col;

        while (tmp[pivot][col] == 0u) {
            ++pivot;
        }

        if (pivot != col) {
            for (uint32_t c = 0; c < len; ++c) {
                uint8_t t;

                t = tmp[col][c];
                tmp[col][c] = tmp[pivot][c];
                tmp[pivot][c] = t;

                t = work[col][c];
                work[col][c] = work[pivot][c];
                work[pivot][c] = t;
            }
        }

        for (uint32_t row = 0; row < len; ++row) {
            if (row != col && tmp[row][col] != 0u) {
                matrix_row_xor_binary(tmp[row], tmp[col], len);
                matrix_row_xor_binary(work[row], work[col], len);
            }
        }
    }
}
/**
 * @brief Generate a uniform random integer in {0, ..., bound - 1} using bytes from an initialized SHAKE256 XOF context
 *
 * @param[in,out] ctx    Initialized SHAKE256 XOF context.
 * @param[in]     bound  Upper bound, must be > 0.
 *
 * @return A random integer in [0, bound).
 */
static uint32_t permutation_rand_bounded(shake256_xof_ctx *ctx, uint32_t bound) {
    uint32_t x = 0;
    uint64_t limit = UINT64_C(0x100000000);  // 2^32
    limit -= limit % bound;
    do {
        xof_get_bytes(ctx, (uint8_t *)&x, sizeof x);
    } while ((uint64_t)x >= limit);
    return x % bound;
}

/**
 * @brief Generate a random permutation array P
 *
 * P has length VEC_N_SIZE_64_BIT.
 * P[i] = j means: column i of the new matrix is column j of the old matrix.
 *
 * @param[in,out] ctx  Initialized SHAKE256 XOF context, e.g. &dk_xof_ctx
 * @param[out]    p    Output permutation array, length VEC_N_SIZE_64_BIT
 */
void permutation_set_random(shake256_xof_ctx *ctx, uint32_t *p) {
    const uint32_t n = VEC_N_SIZE_64_BIT;

    for (int i = 0; i < (int)n; ++i) {
        p[i] = i;
    }

    for (int i = n - 1; i >= 0; i--) {
        int j = permutation_rand_bounded(ctx, i + 1U);

        int tmp = p[i];
        p[i] = p[j];
        p[j] = tmp;
    }
}


/**
 * @brief Return the bit at a given position in a binary vector.
 *
 * The vector is represented as an array of 64-bit words. The bit of index col
 * is stored in word col / 64 at bit position col mod 64.
 *
 * @param[in] v Binary vector packed into 64-bit words.
 * @param[in] col Index of the bit to read.
 *
 * @return The bit value at position col, either 0 or 1.
 */
uint8_t vect_get_bit(const uint64_t *v, size_t col) {
    return (uint8_t)((v[col >> 6] >> (col & 63u)) & 1ULL);
}


/**
 * @brief Sets the bit at a given position in a binary vector.
 *
 * The vector is represented as an array of 64-bit words. If bit is nonzero,
 * the bit at position col is set to 1. Otherwise, it is set to 0.
 *
 * @param[in,out] v Binary vector packed into 64-bit words.
 * @param[in] col Index of the bit to set.
 * @param[in] bit New bit value. A nonzero value is interpreted as 1.
 */
void vect_set_bit(uint64_t *v, size_t col, uint8_t bit) {
    uint64_t mask = 1ULL << (col & 63u);
    if (bit) {
        v[col >> 6] |= mask;
    } else {
        v[col >> 6] &= ~mask;
    }
}


/**
 * @brief Applies a column permutation to a binary vector.
 *
 * The input and output vectors are represented as arrays of 64-bit words.
 * The permutation array P indicates the new position of each original column: out[P[old_col]] = in[old_col].
 *
 * @param[out] out Permuted output vector packed into 64-bit words.
 * @param[in] in Input vector packed into 64-bit words.
 * @param[in] P Column permutation array.
 */
static inline void vect_permute_columns(uint64_t *out, const uint64_t *in, const uint32_t P[VEC_N_SIZE_64_BIT]) {
    memset(out, 0, VEC_N_SIZE_64 * sizeof(uint64_t));

    for (size_t old_col = 0; old_col < VEC_N_SIZE_64_BIT; ++old_col) {
        uint8_t bit = vect_get_bit(in, old_col);
        vect_set_bit(out, P[old_col], bit);
    }
}


/**
 * @brief Applies a column permutation to a binary vector in place.
 *
 * The permutation array P follows the convention: v_after[P[old_col]] = v_before[old_col].
 *
 * @param[in,out] v     Binary vector to be permuted, packed into 64-bit words.
 * @param[in] P         Column permutation array.
 */
void vect_permute_columns_inplace(uint64_t *v, const uint32_t P[VEC_N_SIZE_64_BIT]) {
    uint64_t tmp[VEC_N_SIZE_64];
    vect_permute_columns(tmp, v, P);
    memcpy(v, tmp, sizeof(tmp));
}


/**
 * @brief Multiplies two binary matrices over F_2.
 *
 * This function computes the product: G_permuted = t * intermediate_matrix over the binary field F_2, where addition is XOR and multiplication is AND.
 * Each row of intermediate_matrix and G_permuted is packed into 64-bit words.
 *
 * @param[in,out] G_permuted        Output matrix packed into 64-bit words.
 * @param[in] t                     Binary matrix of size MSG_BIT x MSG_BIT.
 * @param[in] intermediate_matrix   Input matrix packed into 64-bit words.
 */
void matrix_mul_binary_u8_u64(uint64_t G_permuted[MSG_BIT][VEC_N_SIZE_64], uint8_t t[MSG_BIT][MSG_BIT], uint64_t intermediate_matrix[MSG_BIT][VEC_N_SIZE_64]) {
    for (size_t i = 0; i < MSG_BIT; ++i) {
        for (size_t k = 0; k < MSG_BIT; ++k) {
            if ((t[i][k] & 1u) != 0u) {
                for (size_t w = 0; w < VEC_N_SIZE_64; ++w) {
                    G_permuted[i][w] ^= intermediate_matrix[k][w];
                }
            }
        }
    }
}