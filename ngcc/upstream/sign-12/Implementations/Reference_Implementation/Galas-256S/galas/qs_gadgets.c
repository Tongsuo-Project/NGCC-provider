/*
 * qs_gadgets.c — Gala OWF constraint gadgets. See qs_gadgets.h.
 */
#include "qs_gadgets.h"
#include "bf.h"
#include <string.h>

int qs_gadget_inverse(const qs_party* p, qs_elem* b_out, const qs_elem* a) {
    /* a is deg-1: a_val = a->coeffs[0], a_tag = a->coeffs[1].
       We want b such that mac(a*b at Δ) == 1.
       a*b is deg-2: coeffs[0]=a_val*b_val, [1]=a_val*b_tag + a_tag*b_val,
       [2]=a_tag*b_tag. mac(Δ) = c0 + c1·Δ + c2·Δ^2.
       For mac(Δ)==1 we need c0=1, c1=0, c2=0. c2=0 is automatic only if
       a_tag=0 OR b_tag=0 -- but tags are VOLE-defined, not free. The standard
       QuickSilver inverse gadget works differently: it does NOT force mac(a*b)=1
       as a polynomial; instead the prover SENDS b_val, and the constraint
       a_val*b_val == 1 is checked in plaintext, while the MAC consistency of
       a*b is verified separately. The deg-2 element a*b is "reduced" by the
       prover sending the linearization.

       Concretely (FAEST approach): the prover commits b as a deg-1 element
       with b_val = a_val^{-1} and b_tag = a's VOLE-derived tag for b. The
       check a*b=1 is enforced by: prover sends t = a_tag (a correction), and
       verifier checks a_val*b_val == 1 in plaintext AND the MAC of a*b
       evaluates correctly. For this unit we implement the core: build b with
       b_val = a_val^{-1}, b_tag = a_tag (placeholder; real tag comes from VOLE).
       The full constraint wiring is in the signer (3c-5). */
    const gf_ctx* fc = p->fc;
    if (gf_is_zero(fc, a->coeffs[0])) return -1;   /* a_val == 0: undefined */
    gf_limb_t a_val_inv[GF_LIMBS(512)];
    gf_inv(fc, a_val_inv, a->coeffs[0]);
    /* b = deg-1 with plaintext a_val^{-1}; tag carries a's tag (will be
       replaced by the real VOLE tag in the signer). */
    qs_lift(b_out, p, a->coeffs[1], a_val_inv);
    return 0;
}

void qs_gadget_lin_map(const qs_party* p, qs_elem* out,
                       const qs_elem* x,
                       const uint64_t* coeffs) {
    /* L(x) = Σ_j coeff_j · x^{2^j}. x is deg-1 (plaintext x_val = x->coeffs[0]).
       For a LINEAR map on a deg-1 element, the result is deg-1:
         out_val = L(x_val),  out_tag = L(x_tag)
       because L is F_2-linear (L(u+v)=L(u)+L(v), L(c·x)=c·L(x) for c in F_2,
       but here the VOLE is over F_2 so the tag transforms the same way as the
       value under any F_2-linear map). So we evaluate L on both x_val and
       x_tag separately using galas_apply_lin_map. */
    const gf_ctx* fc = p->fc;
    unsigned n = fc->n;
    gf_limb_t l_val[GF_LIMBS(512)], l_tag[GF_LIMBS(512)];
    galas_apply_lin_map(fc, l_val, x->coeffs[0], coeffs);
    galas_apply_lin_map(fc, l_tag, x->coeffs[1], coeffs);
    qs_lift(out, p, l_tag, l_val);
    (void)n;
}

int qs_check_eq(const qs_party* p, const qs_elem* e, const gf_limb_t* expected) {
    gf_limb_t ev[GF_LIMBS(512)];
    qs_eval_at(p, ev, e);
    return memcmp(ev, expected, p->fc->nbytes) == 0;
}
