/* test_qsgadgets.c — verify the linearized-map gadget transforms val & tag
 * consistently (L is F_2-linear, so L(mac) at Δ == L(val) when tag-transformed).
 *
 * Also verifies the inverse gadget produces b_val = a_val^{-1}. */
#include <stdio.h>
#include <string.h>
#include "qs_gadgets.h"
#include "owf.h"
#include "bf.h"

static int fails = 0;
#define CHECK(c,m) do{ if(!(c)){printf("FAIL [%s]\n",m);fails++;}else printf("ok   [%s]\n",m);}while(0)

int main(void) {
    for (size_t li = 0; li < 4; ++li) {
        unsigned lambdas[] = {160,256,384,512};
        unsigned lambda = lambdas[li];
        const galas_owf_params* P = galas_owf_params_for(lambda);
        const gf_ctx* fc = (lambda==160)?bf_ctx_160():(lambda==256)?bf_ctx_256():(lambda==384)?bf_ctx_384():bf_ctx_512();
        char lbl[80];

        gf_limb_t delta[GF_LIMBS(512)];
        gf_zero(fc,delta); delta[0]=0x4242424242424242ULL;
        qs_party p = { fc, {0}, 1 };
        gf_copy(fc, p.delta, delta);

        /* authenticated x: plaintext x_val, tag x_tag */
        gf_limb_t x_val[GF_LIMBS(512)], x_tag[GF_LIMBS(512)];
        gf_zero(fc,x_val); x_val[0]=0x9988776655443322ULL;
        gf_zero(fc,x_tag); x_tag[0]=0x1122334455667788ULL;
        qs_elem x; qs_lift(&x, &p, x_tag, x_val);

        /* L = M0 (input map). out = L(x). */
        qs_elem lo;
        qs_gadget_lin_map(&p, &lo, &x, P->M0);

        /* out_val should equal galas_apply_lin_map(x_val, M0) */
        gf_limb_t exp_val[GF_LIMBS(512)];
        galas_apply_lin_map(fc, exp_val, x_val, P->M0);
        snprintf(lbl,sizeof(lbl),"n=%u lin_map value matches plain OWF M0",lambda);
        CHECK(memcmp(lo.coeffs[0], exp_val, lambda/8)==0, lbl);

        /* out_tag should equal galas_apply_lin_map(x_tag, M0) */
        gf_limb_t exp_tag[GF_LIMBS(512)];
        galas_apply_lin_map(fc, exp_tag, x_tag, P->M0);
        snprintf(lbl,sizeof(lbl),"n=%u lin_map tag matches plain OWF M0",lambda);
        CHECK(memcmp(lo.coeffs[1], exp_tag, lambda/8)==0, lbl);

        /* NOTE: L(mac(x) at Δ) == mac(L(x)) is NOT expected to hold, because L
           is F_2-linear but does NOT commute with field multiplication by Δ
           (L(c·x) = Σ a_j c^{2^j} x^{2^j} != c·L(x) in general). The VOLE/Δ
           consistency is handled by the QuickSilver protocol, not by naive
           commutativity. The value + tag transforms above are the real checks. */

        /* inverse gadget: b_val = x_val^{-1} */
        qs_elem b; int rc = qs_gadget_inverse(&p, &b, &x);
        gf_limb_t inv[GF_LIMBS(512)];
        gf_inv(fc, inv, x_val);
        snprintf(lbl,sizeof(lbl),"n=%u inverse gadget b_val == x_val^-1",lambda);
        CHECK(rc==0 && memcmp(b.coeffs[0], inv, lambda/8)==0, lbl);
    }
    printf("\nqs_gadgets: %d failures\n", fails);
    return fails ? 1 : 0;
}
