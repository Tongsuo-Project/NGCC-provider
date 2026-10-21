/* Copyright (c) 2026 Hang Zhang. */
#ifndef BIT_SPECTRAL_H
#define BIT_SPECTRAL_H

#include <stdint.h>
#include "polyvec.h"

#ifndef BIT_KEYGEN_MAX_ITERS
#define BIT_KEYGEN_MAX_ITERS 1024U
#endif

/* Upper bound for ||(c, c*s_0, c*e)||^2 over every weight-tau challenge.
 * The input coefficients must belong to {-1, 0, 1}. */
uint64_t bit_keygen_spectral_bound_sq(const polyvecl *s_0, const polyveck *e);

#endif
