/*
 * vole_check_ref.h -- portable GALAS VOLE consistency check.
 *
 * This is an independent C reference implementation of the VOLE-check used by
 * the current optimized Galas/FAEST code.  It keeps the FAEST reference style:
 * all transcript bytes are explicit and the caller owns the H_2 context.
 */
#ifndef GALAS_VOLE_CHECK_REF_H
#define GALAS_VOLE_CHECK_REF_H

#include <stdint.h>
#include "instances.h"
#include "random_oracle.h"

void galas_vole_check_sender(uint8_t* proof, galas_H2_ctx* h2,
                             const uint8_t* challenge,
                             const uint8_t* u, uint8_t* const* v,
                             const galas_paramset_t* ps);

void galas_vole_check_receiver(galas_H2_ctx* h2, const uint8_t* challenge,
                               const uint8_t* proof, uint8_t* const* q,
                               const uint8_t* delta,
                               const galas_paramset_t* ps);

#endif
