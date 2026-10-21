/*
 * qs_full_constraints.h — full degree-3 Gala OWF constraint system.
 *
 * Port of RefCodes/opt-faestAndgalas/owf_proof.inc::galas_owf_constraints.
 *
 * The constraint system (all evaluated on authenticated field elements,
 * checked at Δ in the QuickSilver protocol):
 *
 *   1. k[0]·k[1] = 1                    (shrunk-keyspace, degree 2)
 *   2. σ_i · a_i^{2^m} · a_i = 1       (subfield compression, degree 3)
 *      for i ∈ {0,1,2}, where m = λ/2
 *   3. z · t = 1                         (final inverse, degree 2)
 *      where z = L0(b0) ⊕ L1(b1) ⊕ L2(b2),
 *            t = y ⊕ L3(k),
 *            b_i = σ_i · a_i^{2^m}     (the "S(σ_i, a_i)" linearized combo)
 *
 * This file provides:
 *   - galas_constraints_eval: evaluate all constraints on PLAINTEXT values
 *     (Δ=0), returning 1 if all hold (valid witness), 0 otherwise.
 *   - galas_constraints_response: compute the batched QS response for the
 *     constraint system (the value the prover commits and the verifier checks).
 *
 * The full QuickSilver integration (authenticated elements, zk_hash batching)
 * composes these with the qs_elem framework.
 */
#ifndef GALAS_QS_FULL_CONSTRAINTS_H
#define GALAS_QS_FULL_CONSTRAINTS_H

#include <stdint.h>
#include "gf2n.h"
#include "owf.h"

/* Evaluate the full constraint system on plaintext (k, sigma_i, x, y).
   sigma_i are the λ/2-bit subfield witnesses (each lambda/2 bytes, LE).
   Returns 1 if all constraints hold, 0 otherwise.
   This is the Δ=0 check (plaintext correctness); the QS proof checks the
   same polynomials at Δ over authenticated elements. */
int galas_constraints_eval(const gf_ctx* fc, const galas_owf_params* P,
                           const uint8_t* k,  /* lambda bytes */
                           const uint8_t* sigma0, const uint8_t* sigma1,
                           const uint8_t* sigma2, /* lambda/2 bytes each */
                           const uint8_t* x,  /* lambda bytes (public) */
                           const uint8_t* y); /* lambda bytes (public) */

/* Compute the subfield compression witness sigma from a wide-S-box input a.
   sigma = invnorm(a) = P-permute( (a^{2^{λ/2}+1})^{-1} ), λ/2 bits out.
   This is the full operation chain from galas_qs_gadgets.hpp::gala_invnorm. */
void galas_compute_sigma(uint8_t* sigma_out /* lambda/2 bytes */,
                         const uint8_t* a_in /* lambda bytes */,
                         const gf_ctx* fc, const galas_owf_params* P);

#endif
