/**
 * @file code.c
 * @brief Implementation of concatenated code
 */

#include "code.h"
#include <stdint.h>
#include "crypto_memset.h"
#include "parameters.h"
#include "reed_muller.h"
#include "reed_solomon.h"
#include "symmetric.h"

#include <string.h>
#include <stdio.h>
#include "vector.h"

/**
 * @brief Encoding the message m to a code word em using the concatenated code
 *
 * First we encode the message using the Reed-Solomon code, then with the duplicated Reed-Muller code we obtain
 * a concatenated code word.
 *
 * @param[out] em Pointer to an array that is a code word
 * @param[in] m Pointer to an array that is the message
 */
static void code_encode(uint64_t *em, const uint64_t *m) {
    uint64_t tmp[VEC_N1_SIZE_64] = {0};

    reed_solomon_encode(tmp, m);
    reed_muller_encode(em, tmp);

    memset_zero(tmp, sizeof tmp);
}

/**
 * @brief Decoding the code word em to a message m using the concatenated code
 *
 * @param[out] m Pointer to an array that is the message
 * @param[in] em Pointer to an array that is the code word
 */
void code_decode(uint64_t *m, const uint64_t *em) {
    uint64_t tmp[VEC_N1_SIZE_64] = {0};

    reed_muller_decode(tmp, em);
    reed_solomon_decode(m, tmp);

#ifdef VERBOSE
    printf("\n\nReed-Muller decoding result (the input for the Reed-Solomon decoding algorithm): ");
    vect_print(tmp, VEC_N1_SIZE_BYTES);
#endif

    // Zeroize sensitive data
    memset_zero(tmp, sizeof tmp);
}


/**
 * @brief Getting the generator matrix of the concatenated-code
 *
 * We use all the encoder to encode all the base vectors. The generator matrix then follows.
 * @param[out] G the generator matrix of the concatenated-code
 */
static uint64_t G[MSG_BIT][VEC_N_SIZE_64];
static uint8_t status = 0;
void code_generator_matrix() {
    if(status == 1)
        return;

    memset(G, 0, sizeof(G));
    status = 1;

    for (size_t byte = 0; byte < PARAM_K; ++byte) {
        for (size_t bit = 0; bit < 8; ++bit) {
            const size_t row = byte * 8 + bit;
            uint64_t msg_words[CEIL_DIVIDE(PARAM_K, 8)] = {0};
            uint8_t *msg_bytes = (uint8_t *)msg_words;

            msg_bytes[byte] = (uint8_t)(1u << bit);

            code_encode(G[row], msg_words);
        }
    }
}

/**
 * @brief Encoding the message m to a code word em using the matrix named as G_mask
 *
 * @param[out] em Pointer to an array that is a code word
 * @param[in] m Pointer to an array that is the message
 * @param[in] G_mask the generator matrix of the code
 */
void code_encode_matrix(uint64_t *em, const uint64_t *m, uint64_t G_mask[MSG_BIT][VEC_N_SIZE_64]) {
    memset(em, 0, VEC_N_SIZE_64 * sizeof(uint64_t));

    const uint8_t *msg_bytes = (const uint8_t *)m;

    for (size_t byte = 0; byte < PARAM_K; ++byte) {
        uint8_t x = msg_bytes[byte];

        while (x) {
            unsigned bit = __builtin_ctz((unsigned)x);
            size_t row = byte * 8 + bit;

            for (size_t w = 0; w < VEC_N_SIZE_64; ++w) {
                em[w] ^= G_mask[row][w];
            }

            x &= (uint8_t)(x - 1);
        }
    }

#ifdef VERBOSE
    uint64_t tmp[VEC_N1_SIZE_64] = {0};
    reed_solomon_encode(tmp, m);
    printf("\n\nReed-Solomon code word: ");
    vect_print(tmp, VEC_N1_SIZE_BYTES);
    printf("\n\nConcatenated code word: ");
    vect_print(em, VEC_N1N2_SIZE_BYTES);
    memset_zero(tmp, sizeof tmp);
#endif
}


/**
 * @brief Filling the matrix G with random bit from colomn PARAM_N1N2 to VEC_N_SIZE_64_BIT - 1
 * 
 * @param[in,out] xof_ctx              Pointer to a previously initialized SHAKE-256 XOF context.
 * @param[in] intermediate_matrix      the filled generator matrix
 */
static void matrix_fill_random_tail(shake256_xof_ctx *xof_ctx, uint64_t intermediate_matrix[MSG_BIT][VEC_N_SIZE_64]) {
    size_t tail_bits = VEC_N_SIZE_64_BIT - PARAM_N1N2;
    size_t random_bytes = CEIL_DIVIDE(MSG_BIT * tail_bits, 8);

    uint8_t rnd[random_bytes];
    memset(rnd, 0, sizeof(rnd));

    xof_get_bytes(xof_ctx, rnd, random_bytes);

    size_t k = 0;
    for (size_t row = 0; row < MSG_BIT; ++row) {
        for (size_t col = PARAM_N1N2; col < VEC_N_SIZE_64_BIT; ++col, ++k) {
            uint8_t bit = (uint8_t)((rnd[k >> 3] >> (k & 7u)) & 1u);
            vect_set_bit(intermediate_matrix[row], col, bit);
        }
    }
}

/**
 * @brief Using a permutation vector to restruct the generator matrix
 * 
 * @param[in,out] intermediate_matrix  the generator matrix
 * @param[in] p                        the permutation vector
 */

static void matrix_permute_columns(uint64_t intermediate_matrix[MSG_BIT][VEC_N_SIZE_64], const uint32_t p[VEC_N_SIZE_64_BIT]) {
    uint64_t (*tmp)[VEC_N_SIZE_64] = calloc(MSG_BIT, sizeof(*tmp));
    for (size_t new_col = 0; new_col < VEC_N_SIZE_64_BIT; ++new_col) {
        size_t old_col = p[new_col];
        for (size_t row = 0; row < MSG_BIT; ++row) {
            vect_set_bit(tmp[row], new_col, vect_get_bit(intermediate_matrix[row], old_col));
        }
    }

    for (size_t i = 0; i < MSG_BIT; i++) {
        for (size_t j = 0; j < VEC_N_SIZE_64; j++) {
            intermediate_matrix[i][j] = tmp[i][j];
        }
    }

    free(tmp);
}

/**
 * @brief Filling the rest colomn with random bits and using a permutation vector to restruct the generator matrix
 * 
 * During the generating process, we use the same ctx to get random bits.
 * 
 * @param[in,out] xof_ctx                  Pointer to a previously initialized SHAKE-256 XOF context.
 * @param[in,out] intermediate_matrix      the restructed generator matrix
 * @param[in] p                            the permutation vector
 */
void code_generator_matrix_randomized_permuted(shake256_xof_ctx *xof_ctx, uint64_t intermediate_matrix[MSG_BIT][VEC_N_SIZE_64], const uint32_t p[VEC_N_SIZE_64_BIT]) {
    for(size_t i = 0; i < MSG_BIT; i++)
        for(size_t j = 0; j < VEC_N_SIZE_64; j++)
            intermediate_matrix[i][j] = G[i][j];
    
    matrix_fill_random_tail(xof_ctx, intermediate_matrix);
    matrix_permute_columns(intermediate_matrix, p);
}

/**
 * @brief Filling the rest colomn with random bits and using a permutation vector to restruct the generator matrix
 * 
 * After permutating and decoding, we get mT as the result. Multiplying T^{-1} in secret key, the message m then follows.
 * 
 * @param[out] em            Pointer to an array that is the real msg
 * @param[in] m              Pointer to an array that is a code word of the concatenated code
 * @param[in] t_inverse      the inverse matrix of t
 */
void msg_matrix_mul(uint64_t *em, const uint64_t *m, uint8_t t_inverse[MSG_BIT][MSG_BIT]) {
    const size_t msg_words = CEIL_DIVIDE(PARAM_K, 8);

    memset(em, 0, msg_words * sizeof(uint64_t));

    for (size_t row = 0; row < MSG_BIT; ++row) {
        if (vect_get_bit(m, row)) {
            for (size_t col = 0; col < MSG_BIT; ++col) {
                if (t_inverse[row][col] & 1u) {
                    em[col >> 6] ^= (1ULL << (col & 63u));
                }
            }
        }
    }
}