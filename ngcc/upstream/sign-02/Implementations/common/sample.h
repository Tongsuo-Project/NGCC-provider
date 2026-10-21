/* Copyright (c) 2026 Hang Zhang. */

#ifndef BIT_SAMPLE_H
#define BIT_SAMPLE_H
#include "polyvec.h"
#include "parabola.h"

/* Round Q13 coordinates toward +infinity at half-grid points. */
#if BIT_N == 1024
int32_t bit_round_q13_to_i32(int32_t x);
#else
int16_t bit_round_q13_to_i16(int32_t x);
#endif

/* Return -1 on nonce exhaustion, otherwise sample and round the joint mask. */
int polyvecy_sample_mask_q13(polyvecy *y, bit_parabola_fixed *y_q13, const uint8_t *seed,
                             uint16_t *nonce);
/* Return nonzero if coefficient, hint, joint L2 or nonzero-residue checks fail.
 * The joint response includes z0; equality at the squared radius is accepted. */
int check_signature_bounds(const polyvecm1 *z1, const polyveck *h);
/* Verification requires the original response to be centered modulo q. */
int check_signature_verification_bounds(const polyvecm1 *z1, const polyveck *h);
#endif
