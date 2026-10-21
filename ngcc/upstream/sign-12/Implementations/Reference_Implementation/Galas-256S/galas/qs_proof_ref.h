/*
 * qs_proof_ref.h -- portable degree-3 QuickSilver proof for Galas.
 *
 * This module is intentionally independent from the optimized implementation:
 * it mirrors the transcript semantics used by FAEST/Galas QuickSilver, but is
 * written as straightforward C99 reference code.
 */
#ifndef GALAS_QS_PROOF_REF_H
#define GALAS_QS_PROOF_REF_H

#include <stdint.h>
#include "instances.h"

int galas_qs_prove(uint8_t* qs_proof, uint8_t* qs_check,
                   uint8_t* u_with_witness, uint8_t* const* v,
                   const uint8_t* challenge,
                   const uint8_t* pk_x, const uint8_t* pk_y,
                   const galas_paramset_t* ps);

int galas_qs_verify(uint8_t* qs_check, const uint8_t* qs_proof,
                    uint8_t* const* q,
                    const uint8_t* correction,
                    const uint8_t* delta,
                    const uint8_t* challenge,
                    const uint8_t* pk_x, const uint8_t* pk_y,
                    const galas_paramset_t* ps);

#endif /* GALAS_QS_PROOF_REF_H */
