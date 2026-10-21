/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "params.h"
#include "align.h"
#include "reduce.h"

static inline int32_t ct_abs(int32_t x)
{
    int32_t mask = x >> 31;
    return (x ^ mask) - mask;
}

typedef struct {
    int16_t coeffs[BIT_N];
} BIT_ALIGN poly;

typedef struct {
    int16_t coeffs[BIT_N];
} BIT_ALIGN poly_ntt;

typedef struct {
    uint8_t pos[BIT_TAU];
    int8_t sign[BIT_TAU];
    uint8_t bflip;
} sparse_challenge;

int poly_sample_S1(poly *a, const uint8_t *seed, uint16_t *nonce);
int poly_challenge(poly *a, const unsigned char challenge[BIT_CHALLENGEBYTES]);
void poly_challenge_to_sparse(sparse_challenge *s, const poly *c);
void poly_cneg(poly *a, uint8_t b);
void poly_add(poly *result, const poly *a, const poly *b);
void poly_add_no_reduce(poly *result, const poly *a, const poly *b);
void poly_add_small(poly *result, const poly *a, const poly *small);
void poly_to_ntt(poly_ntt *result, const poly *a);
/* Writes centered residues to a; result and a must not overlap. */
void poly_to_ntt_centered(poly_ntt *result, poly *a);
void poly_from_ntt(poly *result, const poly_ntt *a);
void poly_from_ntt_inplace(poly *result, poly_ntt *a);
void poly_ntt_basemul_raw(poly_ntt *result, const poly_ntt *a, const poly_ntt *b);
void poly_ntt_dot3_raw(poly_ntt *result, const poly_ntt a[BIT_L], const poly_ntt b[BIT_L]);
void poly_ntt_montgomery_lift(poly_ntt *result);
void poly_mul_challenge_no_reduce(poly *result, const poly *a, const sparse_challenge *c);
void poly_highbits(poly *w1, const poly *w);
void poly_canonicalize(poly *b);
void decompose_hint(int16_t *highbits, int16_t r);
void poly_pack_w1(unsigned char *r, const poly *a);
int poly_reject_highbits_bimodal(const poly *w1, const poly *w0, const sparse_challenge *c);

#endif
