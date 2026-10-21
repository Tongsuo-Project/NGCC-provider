/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#ifndef PARAMS_H
#define PARAMS_H

#include <stdint.h>

/* Core constants */
#define BIT_SECURITY_BITS 256
#define BIT_LAMBDA 264
#define BIT_N 512
#define BIT_Q 7681
#define BIT_BARRETT_SHIFT 32
#define BIT_BARRETT_MULT 559168

#define BIT_Q_HALF (BIT_Q / 2)
#define BIT_Q_HAT 3841
#define BIT_Q_HAT_MINUS (BIT_Q_HAT - BIT_Q)

#define BIT_K 3
#define BIT_L 3

/* Sampling parameters */
#define BIT_TAU 58
#define BIT_PARABOLA_BLOCKS (BIT_L + BIT_K + 1)
#define BIT_PARABOLA_DIM (BIT_PARABOLA_BLOCKS * BIT_N)
#define BIT_PARABOLA_BETA2 50625ULL
#define BIT_GAMMA1 15400
#define BIT_RESPONSE_COEFF_BOUND (BIT_GAMMA1 - 1)
#define BIT_PARABOLA_AUX_DIM 4
#define BIT_PARABOLA_FRAC_BITS 13
#define BIT_PARABOLA_SCALE (1U << BIT_PARABOLA_FRAC_BITS)
#define BIT_PARABOLA_RADIUS_FIXED ((uint64_t)BIT_GAMMA1 * BIT_PARABOLA_SCALE)
#define BIT_PARABOLA_L2_FIXED (BIT_PARABOLA_RADIUS_FIXED * BIT_PARABOLA_RADIUS_FIXED)
/* ceil(sqrt(BIT_PARABOLA_DIM)/2) = 30 for D=3584. */
#define BIT_PARABOLA_ROUND_MARGIN 30U
#define BIT_PARABOLA_PROPOSAL_RADIUS_FIXED (BIT_PARABOLA_RADIUS_FIXED + BIT_PARABOLA_ROUND_MARGIN)

#define BIT_GAMMA2 470
#define BIT_GAMMA2_BARRETT_SHIFT 32
/* ceil(2^32/gamma2), exact for canonical HighBits inputs. */
#define BIT_GAMMA2_BARRETT_MULT 9138229U

/* High-bits and hint thresholds */
#define BIT_HINT_MODULUS ((((BIT_Q - 1) + (BIT_GAMMA2 >> 1)) / BIT_GAMMA2) + 1)

/* Byte-level sizes */
#define BIT_SEEDBYTES 32
#define BIT_CHALLENGEBYTES 64
#define BIT_MESSAGEBYTES 64
#define BIT_TRBYTES 64
#define BIT_SAMPLE_S1_BUFLEN BIT_N / 2

/* Polynomial and vector encoding */
#define BIT_PUBLIC_COEFF_BITS 13
#define BIT_POLY_PUBLIC_BYTES (BIT_N * BIT_PUBLIC_COEFF_BITS / 8)
#define BIT_POLYVECK_PUBLIC_BYTES (BIT_K * BIT_POLY_PUBLIC_BYTES)

#define BIT_Z0_LOW_BITS 8
#define BIT_POLY_Z0_LOW_BYTES (BIT_N * BIT_Z0_LOW_BITS / 8)
#define BIT_Z1_LOW_BYTES (BIT_L * BIT_N)
#define BIT_Z_LOW_BYTES (BIT_POLY_Z0_LOW_BYTES + BIT_Z1_LOW_BYTES)

#define BIT_W1_BITS 5
#define BIT_POLY_W1_BYTES (BIT_N * BIT_W1_BITS / 8)
#define BIT_POLYVECK_W1_BYTES (BIT_K * BIT_POLY_W1_BYTES)

/* Store centered byte-aligned z0 and byte-aligned z1 low parts directly; encode
 * their high parts and h as independent rANS streams with 16-bit lengths. */
#define BIT_H_INF 3
/* Maximum squared hint norm. */
#define BIT_H_L2_BOUND_SQ 905U

/* Joint signature radius and its squared comparison bound. */
#define BIT_SIGNATURE_L2_BOUND 17735U
#define BIT_SIGNATURE_L2_BOUND_SQ \
    (UINT64_C(1) * BIT_SIGNATURE_L2_BOUND * BIT_SIGNATURE_L2_BOUND)
#define BIT_SIG_RANS_LENGTH_BYTES 4
#define BIT_SIG_RANS_PAYLOAD_BYTES 860

/* Key and signature sizes */
#define BIT_PUBLICKEYBYTES (BIT_SEEDBYTES + BIT_POLYVECK_PUBLIC_BYTES)
#define BIT_SECRETKEYBYTES                                                                         \
    (BIT_SEEDBYTES + BIT_POLYVECK_PUBLIC_BYTES + BIT_SEEDBYTES + BIT_TRBYTES +                         \
     (BIT_L + BIT_K) * BIT_N / 4)
#define BIT_SIGN_MAX_ITERS 65536

#define BIT_SIGNBYTES                                                                              \
    (BIT_CHALLENGEBYTES + BIT_Z_LOW_BYTES + BIT_SIG_RANS_LENGTH_BYTES + BIT_SIG_RANS_PAYLOAD_BYTES)

#if BIT_PUBLICKEYBYTES != 2528 || BIT_SECRETKEYBYTES != 3392 || BIT_SIGNBYTES != 2976
#error "BiT-256 encoded sizes no longer match the parameter set"
#endif

#endif
