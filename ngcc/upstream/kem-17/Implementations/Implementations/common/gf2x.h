/**
 * @file gf2x.h
 * @brief Header file for gf2x.c
 */

#ifndef HEP_QC_GF2X_H
#define HEP_QC_GF2X_H

#include <stdint.h>

void vect_mul(uint64_t *o, const uint64_t *v1, const uint64_t *v2);

#endif  // HEP_QC_GF2X_H
