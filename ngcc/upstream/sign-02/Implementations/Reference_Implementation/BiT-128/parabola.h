/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#ifndef PARABOLA_H
#define PARABOLA_H

#include <stdint.h>
#include "polyvec.h"

typedef struct {
    /* Joint Q13 coordinates: y0 || ys || ye. */
    int32_t coeffs[BIT_PARABOLA_BLOCKS][BIT_N];
} bit_parabola_fixed;

/* Sample the joint paraboloid on the Q13 grid; return -1 on nonce exhaustion. */
int bit_parabola_sample_fixed(bit_parabola_fixed *y, const uint8_t *seed, uint16_t *nonce);
uint64_t bit_parabola_norm2_fixed(const bit_parabola_fixed *y);
/* Joint response rejection with integer shifts c0, c_s and c_e.
 * mask holds the Q13 proposal. Return 1 to reject, 0 to accept,
 * or -1 on nonce exhaustion. */
int bit_parabola_reject_response(const poly *c0, const polyvecl *c_s, const polyveck *c_e,
                                 const bit_parabola_fixed *mask, const uint8_t *seed,
                                 uint16_t *nonce);

#endif
