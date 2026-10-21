/**
 * @file vector.h
 * @brief Header file for vector.c
 */

#ifndef HEP_QC_VECTOR_H
#define HEP_QC_VECTOR_H

#include <immintrin.h>
#include <stdint.h>
#include "symmetric.h"
#include "parameters.h"

void vect_generate_random_support1(shake256_xof_ctx *ctx, uint32_t *support, uint16_t weight);
void vect_generate_random_support2(shake256_xof_ctx *ctx, uint32_t *support, uint16_t weight);
void vect_write_support_to_vector(uint64_t *v, uint32_t *support, uint16_t weight);
void vect_sample_fixed_weight1(shake256_xof_ctx *ctx, uint64_t *v, uint16_t weight);
void vect_sample_fixed_weight2(shake256_xof_ctx *ctx, uint64_t *v, uint16_t weight);
void vect_set_random(shake256_xof_ctx *ctx, uint64_t *v);

void vect_add(uint64_t *o, const uint64_t *v1, const uint64_t *v2, uint32_t size);
uint8_t vect_compare(const uint8_t *v1, const uint8_t *v2, uint32_t size);

void vect_print(const uint64_t *v, const uint32_t size);
uint8_t vect_get_bit(const uint64_t *v, size_t col);
void vect_set_bit(uint64_t *v, size_t col, uint8_t bit);

void invertible_matrix(uint8_t work[MSG_BIT][MSG_BIT], uint8_t mat[MSG_BIT][MSG_BIT], uint32_t len);
void matrix_set_random(shake256_xof_ctx *ctx, uint8_t v[MSG_BIT][MSG_BIT], uint32_t len);

void permutation_set_random(shake256_xof_ctx *ctx, uint32_t *p);

void vect_permute_columns_inplace(uint64_t *v, const uint32_t P[VEC_N_SIZE_64_BIT]);

void matrix_mul_binary_u8_u64(uint64_t G_permuted[MSG_BIT][VEC_N_SIZE_64], uint8_t t[MSG_BIT][MSG_BIT], uint64_t intermediate_matrix[MSG_BIT][VEC_N_SIZE_64]);



#endif  // HEP_QC_VECTOR_H
