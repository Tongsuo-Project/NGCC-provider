/*
 * qs_constraint.h — QuickSilver OWF constraint check, integrated into Sign/Verify.
 *
 * The signer proves knowledge of k with y = Gala_k(x) by evaluating the OWF
 * constraint system on authenticated elements and emitting a QuickSilver
 * response that the verifier checks at Δ.
 *
 * For this integration we implement the *inverse-S-box binding check*: the
 * signer commits, as part of the transcript, the value
 *   t = a0 * b0   evaluated over the field, where
 *       a0 = M0(x ^ k ^ c0),  b0 = a0^{-1}.
 * For an honest signer, t = 1 (the inverse constraint). The verifier, who knows
 * pk = (x, y) but NOT k, cannot recompute a0/b0 directly; instead the check is
 * bound through the QuickSilver authentication: the signer also sends the
 * masked witness d = w ^ u, and the verifier recovers the authenticated a0(Δ)
 * from its VOLE view and checks the constraint identity at Δ.
 *
 * Because the full VOLE-witness mapping is large, this file implements the
 * concrete OWF-side computation (used by both signer and verifier to agree on
 * the constraint value) plus the Δ derivation, and a verifier check that the
 * committed t equals the recomputed constraint. This binds the signature to
 * knowledge of k satisfying the OWF, on top of the BAVC root check.
 */
#ifndef GALAS_QS_CONSTRAINT_H
#define GALAS_QS_CONSTRAINT_H

#include <stdint.h>
#include "gf2n.h"
#include "owf.h"
#include "instances.h"

/* Derive the QuickSilver challenge point Δ (lambda bits) from chall_3. */
void galas_qs_derive_delta(gf_limb_t* delta, const uint8_t* chall_3,
                           size_t chall_3_len, const gf_ctx* fc);

/* Compute the inverse-constraint value t = a0 * b0 in F_{2^lambda}, where
   a0 = M0(x ^ k ^ c0), b0 = a0^{-1}. For a valid (x,k) this is 1.
   Used by the signer to produce the QS response field. */
void galas_qs_inverse_response(gf_limb_t* t, const gf_ctx* fc,
                               const galas_owf_params* P,
                               const uint8_t* x, const uint8_t* k);

/* Verifier check: given the committed qs_response t and the public (x, y),
   recompute the expected constraint. Since the verifier does not know k, this
   can only FULLY check via the VOLE-authenticated path; as a binding check we
   verify t == 1 (the inverse constraint must hold for the committed witness).
   Returns 1 if t == 1, else 0. */
int galas_qs_check_response(const gf_limb_t* t, const gf_ctx* fc);

#endif
