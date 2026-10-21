/*
 * challenge.h — Fiat-Shamir challenge encoding/decoding for GALAS.
 *
 * chall_3 encodes tau leaf-index challenges (i_delta[0..tau)) plus the global
 * point Delta (with w_grind zero bits). decode_all_chall_3 reconstructs
 * i_delta[] from chall_3 and checks the grinding constraint.
 *
 * Port of ref-FAEST challenge.c logic (decode_all_chall_3, check_challenge_3).
 */
#ifndef GALAS_CHALLENGE_H
#define GALAS_CHALLENGE_H

#include <stdint.h>
#include <stdbool.h>
#include "instances.h"

/* check that chall_3 has at least (lambda - w_grind) distinct values among the
   first lambda values, AND that the top w_grind bits are zero (grinding). */
bool galas_check_chall_3(const uint8_t* chall_3, unsigned lambda, unsigned w_grind);

/* decode chall_3 into i_delta[0..tau) (the tau distinct leaf indices). Returns
   true if exactly tau distinct values are present in the right range. */
bool galas_decode_all_chall_3(uint16_t* i_delta, const uint8_t* chall_3,
                              const galas_paramset_t* ps);

#endif
