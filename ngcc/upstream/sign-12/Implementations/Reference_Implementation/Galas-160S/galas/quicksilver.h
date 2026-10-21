/*
 * quicksilver.h — QuickSilver authenticated-field-element framework for GALAS.
 *
 * Port of the deg<=3 QuickSilver machinery from
 * RefCodes/opt-faestAndgalas/quicksilver.hpp + galas_qs_gadgets.hpp.
 *
 * Model: an authenticated degree-1 element over GF(2^lambda) carries
 *   mac(Δ) = tag·Δ + val
 * where `val` is the plaintext (the actual witness value) and `tag` is the
 * VOLE MAC. The verifier holds the correlated view Q and recovers val at Δ.
 *
 * Multiplying two deg-1 elements yields a deg-2 element; the QuickSilver check
 * evaluates the constraint polynomial at Δ and compares prover/verifier views.
 *
 * GALAS constraints (degree <= 3, d=3):
 *   - inverse S-box:  a·b = 1   (deg 2)
 *   - linearized maps L_i: Frobenius sums (deg 1)
 *   - subfield compression σ: degree-1 embedding via B' basis
 */
#ifndef GALAS_QUICKSILVER_H
#define GALAS_QUICKSILVER_H

#include <stdint.h>
#include "gf2n.h"

/* A QuickSilver authenticated element of degree `deg` (0..3).
   coeffs[deg] = plaintext value; coeffs[0..deg-1] = MAC tags.
   mac(Δ) = sum_{i=0..deg} coeffs[i] * Δ^i. */
typedef struct {
    unsigned deg;                       /* 0..3 */
    gf_limb_t coeffs[4][GF_LIMBS(512)]; /* coeffs[i] = coefficient of Δ^i */
} qs_elem;

/* A QuickSilver party state: carries the field ctx and the global point Δ
   (verifier knows Δ; prover does not, but prover's elements still carry tags). */
typedef struct {
    const gf_ctx* fc;
    gf_limb_t delta[GF_LIMBS(512)];     /* global point Δ (verifier-side) */
    int is_verifier;                    /* 1 = verifier (knows Δ), 0 = prover */
} qs_party;

/* ---- element constructors ---- */
void qs_const(qs_elem* e, const qs_party* p, const gf_limb_t* value);  /* deg-0 constant */
void qs_lift(qs_elem* e, const qs_party* p, const gf_limb_t* tag, const gf_limb_t* val); /* deg-1 */
void qs_zero(qs_elem* e);

/* ---- arithmetic ---- */
/* deg-1 + deg-d -> deg-d+? (we cap at deg 3). out <- a + b. */
void qs_add(const qs_party* p, qs_elem* out, const qs_elem* a, const qs_elem* b);
/* deg-1 * deg-d -> deg-(d+1). out <- a * b. a must be deg<=1. */
void qs_mul(const qs_party* p, qs_elem* out, const qs_elem* a, const qs_elem* b);
/* multiply by a public constant (no degree increase). out <- a * c. */
void qs_mulc(const qs_party* p, qs_elem* out, const qs_elem* a, const gf_limb_t* c);

/* evaluate the MAC polynomial at Δ: out(Δ) = sum coeffs[i]·Δ^i. */
void qs_eval_at(const qs_party* p, gf_limb_t* out, const qs_elem* e);

#endif /* GALAS_QUICKSILVER_H */
