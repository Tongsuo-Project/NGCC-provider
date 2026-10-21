/**
 * @file code.h
 * @brief Header file of code.c
 */

#ifndef HEP_QC_CODE_H
#define HEP_QC_CODE_H

#include <stddef.h>
#include <stdint.h>
#include "parameters.h"
#include "symmetric.h"

void code_decode(uint64_t *m, const uint64_t *em);

void code_generator_matrix(void);

void code_generator_matrix_randomized_permuted(shake256_xof_ctx *xof_ctx, uint64_t intermediate_matrix[MSG_BIT][VEC_N_SIZE_64], const uint32_t p[VEC_N_SIZE_64_BIT]);
void code_encode_permuted_matrix(uint64_t *em, const uint64_t *m, const uint64_t intermediate_matrix[MSG_BIT][VEC_N_SIZE_64]);
void code_encode_matrix(uint64_t *em, const uint64_t *m, uint64_t G_mask[MSG_BIT][VEC_N_SIZE_64]);

void msg_matrix_mul(uint64_t *em, const uint64_t *m, uint8_t t_inverse[MSG_BIT][MSG_BIT]);

#endif  // HEP_QC_CODE_H=