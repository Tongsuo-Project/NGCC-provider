/*
 * qs_gadgets.h — Gala OWF constraint gadgets over QuickSilver elements.
 *
 * Composes the quicksilver.h arithmetic into the GALAS constraint system:
 *   - qs_gadget_inverse: given authenticated a (deg 1), produce authenticated b
 *     (deg 1) such that a*b has mac(Δ) = 1 (the inverse constraint a*b=1).
 *     Prover knows a_val = a's plaintext, sets b_val = a_val^{-1}, and the tag
 *     of b is chosen so the cross-term cancels: this is the standard QuickSilver
 *     inverse gadget. The verifier checks mac(a*b) == 1 at Δ.
 *   - qs_gadget_lin_map: evaluate a linearized polynomial L(x)=Σ a_j x^{2^j}
 *     on an authenticated element (deg stays 1; linear combo of Frobenii).
 *   - constraint check helpers.
 *
 * These compose verified primitives (qs_mul, qs_mulc, gf_frobenius, gf_inv).
 */
#ifndef GALAS_QS_GADGETS_H
#define GALAS_QS_GADGETS_H

#include "quicksilver.h"
#include "owf.h"

/* Inverse gadget: out <- authenticated b with b_val = a_val^{-1} (in the field),
   and mac chosen so that mac(a*b at Δ) == 1.
   Pre: a is deg-1, a_val (plaintext) is nonzero. Returns 0 on success. */
int qs_gadget_inverse(const qs_party* p, qs_elem* b_out, const qs_elem* a);

/* Linearized-map gadget: out <- L(x) = Σ_j coeff_j * x^{2^j}, where coeff_j are
   the n field-element coefficients of the linearized poly (from galas_params).
   x is deg-1; out is deg-1 (linear combination). Uses Frobenius caching. */
void qs_gadget_lin_map(const qs_party* p, qs_elem* out,
                       const qs_elem* x,
                       const uint64_t* coeffs /* n * nlimbs u64 */);

/* Constraint check: returns 1 if mac(e at Δ) == expected (a field element),
   else 0. Used to verify e.g. a*b == 1, or L0(b0)^... == z. */
int qs_check_eq(const qs_party* p, const qs_elem* e, const gf_limb_t* expected);

#endif
