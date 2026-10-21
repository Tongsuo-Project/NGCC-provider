/*
 * owf.h — the Gala one-way function (reference, C99).
 *
 * Structure (mirrors RefCodes/opt-faestAndgalas/galas_qs_gadgets.hpp):
 *
 *   a0 = M0(x ^ k ^ c0)          (M0 = input linearized poly)
 *   a1 = M1(k ^ c1)
 *   a2 = M2(k ^ c2)
 *   b0 = a0^{-1},  b1 = a1^{-1},  b2 = a2^{-1}
 *   z  = L0(b0) ^ L1(b1) ^ L2(b2)
 *   out = z^{-1} ^ L3(k)
 *
 * Each M_i / L_i is a linearized polynomial  L(X) = sum_{j} coeff_j X^{2^j}.
 *
 * The constants and map coefficients are supplied per n by the instance via
 * the galas_owf_params struct (filled from galas_params_<n>.h by owf_params.c).
 */
#ifndef GALAS_OWF_H
#define GALAS_OWF_H

#include "gf2n.h"

/* A linearized-polynomial map: n field-element coefficients. */
typedef struct {
    const uint64_t (*coeffs)[8];  /* n rows, each (n+63)/64 LE u64 limbs */
    unsigned n;
    unsigned nlimbs;
} galas_lin_map;

/* Full OWF parameter set for one n. References the static tables in
   galas_params_<n>.h; assembled by galas_owf_params_for(n). */
typedef struct {
    unsigned n;
    unsigned nbytes;
    const uint8_t* c0;       /* length nbytes */
    const uint8_t* c1;
    const uint8_t* c2;
    const uint64_t* M0;      /* n * nlimbs u64 */
    const uint64_t* M1;
    const uint64_t* M2;
    const uint64_t* L0;
    const uint64_t* L1;
    const uint64_t* L2;
    const uint64_t* L3;
} galas_owf_params;

/* Return a pointer to the static OWF parameter set for n, or NULL if n is
   unsupported. */
const galas_owf_params* galas_owf_params_for(unsigned n);

/* Apply linearized polynomial map L to x: out <- sum_j coeff_j * x^{2^j}. */
void galas_apply_lin_map(const gf_ctx* ctx, gf_limb_t* out,
                         const gf_limb_t* x,
                         const uint64_t* coeffs /* n * nlimbs u64 */);

/* Evaluate the Gala OWF: out <- Gala_k(x). Both x and k are nbytes-long
   little-endian byte arrays (in), out is nbytes (out).
   Returns 0 normally. Returns -1 if any of the wide-S-box inputs a0,a1,a2,z
   is zero (the rejection case used by KeyGen). */
int galas_owf_eval(const galas_owf_params* P, const gf_ctx* ctx,
                   uint8_t* out, const uint8_t* x, const uint8_t* k);

#endif /* GALAS_OWF_H */
