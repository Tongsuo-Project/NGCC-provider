/**
 * @file parsing.h
 * @brief Header file for parsing.c
 */

#ifndef HEP_QC_PARSING_H
#define HEP_QC_PARSING_H

#include <immintrin.h>
#include <stdint.h>
#include "data_structures.h"
#include "parameters.h"

void hep_qc_c_kem_to_string(uint8_t *ct, const ciphertext_kem_t *c_kem);
void hep_qc_c_kem_from_string(ciphertext_pke_t *c_pke, uint8_t *salt, const uint8_t *ct);

void hep_qc_dk_pke_from_string_all(uint64_t *y, uint8_t t[MSG_BIT][MSG_BIT], uint32_t *p, const uint8_t *dk_pke);
void hep_qc_ek_pke_from_string_all(uint64_t *h, uint64_t *s, uint64_t G_mask[MSG_BIT][VEC_N_SIZE_64], const uint8_t *ek_pke);

#endif  // HEP_QC_PARSING_H
