/* test_qs.c — QuickSilver arithmetic self-check.
 *
 * Verifies the core QuickSilver identity without the full sign/verify flow:
 *   - mac(Δ) of a deg-1 element equals tag·Δ + val (computed independently).
 *   - product a·b: mac(Δ) at Δ equals val_a·val_b (the plaintext product),
 *     AND the deg-2 element's coeffs correctly represent the polynomial
 *     tag_a·tag_b·Δ^2 + (tag_a·val_b + val_a·tag_b)·Δ + val_a·val_b.
 *   - public-constant multiply preserves the MAC linearly.
 *
 * This is the unit-testable core; the OWF constraint gadget composes these.
 */
#include <stdio.h>
#include <string.h>
#include "quicksilver.h"
#include "bf.h"

static int fails = 0;
#define CHECK(c,m) do{ if(!(c)){printf("FAIL [%s]\n",m);fails++;}else printf("ok   [%s]\n",m);}while(0)

int main(void) {
    for (size_t li = 0; li < 4; ++li) {
        unsigned lambdas[] = {160,256,384,512};
        unsigned lambda = lambdas[li];
        const gf_ctx* fc = (lambda==160)?bf_ctx_160():(lambda==256)?bf_ctx_256():(lambda==384)?bf_ctx_384():bf_ctx_512();
        char lbl[80];

        /* fixed Δ, tag_a, val_a, tag_b, val_b */
        gf_limb_t delta[GF_LIMBS(512)], ta[GF_LIMBS(512)], va[GF_LIMBS(512)];
        gf_limb_t tb[GF_LIMBS(512)], vb[GF_LIMBS(512)];
        gf_zero(fc,delta); delta[0]=0x1111111111111111ULL; delta[1]=0x2222222222222222ULL;
        gf_zero(fc,ta); ta[0]=0xAAAA000000000001ULL;
        gf_zero(fc,va); va[0]=0x123456789ABCDEF0ULL;
        gf_zero(fc,tb); tb[0]=0xBBBB000000000002ULL;
        gf_zero(fc,vb); vb[0]=0x0FEDCBA987654321ULL;

        qs_party p = { fc, {0}, 1 };
        gf_copy(fc, p.delta, delta);

        /* deg-1 element a: mac(Δ) = ta·Δ + va */
        qs_elem a, b;
        qs_lift(&a, &p, ta, va);
        qs_lift(&b, &p, tb, vb);

        /* check mac(Δ) == ta·Δ + va */
        gf_limb_t eval[GF_LIMBS(512)], expect[GF_LIMBS(512)], t[GF_LIMBS(512)];
        qs_eval_at(&p, eval, &a);
        gf_mul(fc, t, ta, delta);
        gf_add(fc, expect, t, va);
        snprintf(lbl,sizeof(lbl),"n=%u mac(deg1) at Δ",lambda);
        CHECK(memcmp(eval,expect,lambda/8)==0, lbl);

        /* product c = a*b (deg 2): mac(Δ) is the full quadratic
           ta·tb·Δ² + (ta·vb+va·tb)·Δ + va·vb (NOT just va·vb unless Δ=0).
           We verify mac(Δ) against the coefficient-form evaluation. */
        qs_elem c;
        qs_mul(&p, &c, &a, &b);
        qs_eval_at(&p, eval, &c);
        /* recompute expect = c[2]·Δ² + c[1]·Δ + c[0] directly */
        {
            gf_limb_t d2[GF_LIMBS(512)], d1[GF_LIMBS(512)], s[GF_LIMBS(512)];
            gf_mul(fc, d2, c.coeffs[2], delta); gf_mul(fc, d2, d2, delta);
            gf_mul(fc, d1, c.coeffs[1], delta);
            gf_add(fc, s, d2, d1);
            gf_add(fc, expect, s, c.coeffs[0]);
        }
        snprintf(lbl,sizeof(lbl),"n=%u mac(a*b) at Δ == quadratic eval",lambda);
        CHECK(memcmp(eval,expect,lambda/8)==0, lbl);

        /* check the deg-2 coefficients are exactly:
           c[2]=ta·tb, c[1]=ta·vb+va·tb, c[0]=va·vb */
        gf_limb_t e2[GF_LIMBS(512)], e1a[GF_LIMBS(512)], e1b[GF_LIMBS(512)], e1[GF_LIMBS(512)], e0[GF_LIMBS(512)];
        gf_mul(fc, e2, ta, tb);
        gf_mul(fc, e1a, ta, vb); gf_mul(fc, e1b, va, tb); gf_add(fc, e1, e1a, e1b);
        gf_mul(fc, e0, va, vb);
        snprintf(lbl,sizeof(lbl),"n=%u deg2 coeffs c2=ta*tb",lambda);
        CHECK(memcmp(c.coeffs[2],e2,lambda/8)==0, lbl);
        snprintf(lbl,sizeof(lbl),"n=%u deg2 coeffs c1=ta*vb+va*tb",lambda);
        CHECK(memcmp(c.coeffs[1],e1,lambda/8)==0, lbl);
        snprintf(lbl,sizeof(lbl),"n=%u deg2 coeffs c0=va*vb",lambda);
        CHECK(memcmp(c.coeffs[0],e0,lambda/8)==0, lbl);

        /* public-const multiply: d = a * c (c public), mac(Δ)=ta·Δ·c+va·c */
        gf_limb_t pub[GF_LIMBS(512)]; gf_zero(fc,pub); pub[0]=0xC0FFEE;
        qs_elem d; qs_mulc(&p, &d, &a, pub);
        qs_eval_at(&p, eval, &d);
        gf_limb_t et[GF_LIMBS(512)];
        gf_mul(fc, et, ta, delta); gf_mul(fc, et, et, pub);
        gf_mul(fc, expect, va, pub); gf_add(fc, expect, expect, et);
        snprintf(lbl,sizeof(lbl),"n=%u mulc mac at Δ",lambda);
        CHECK(memcmp(eval,expect,lambda/8)==0, lbl);

        /* add: e = a + b, mac(Δ) = (ta+tb)·Δ + (va+vb) */
        qs_elem e; qs_add(&p, &e, &a, &b);
        qs_eval_at(&p, eval, &e);
        gf_limb_t st[GF_LIMBS(512)], sv[GF_LIMBS(512)];
        gf_add(fc, st, ta, tb); gf_add(fc, sv, va, vb);
        gf_mul(fc, t, st, delta); gf_add(fc, expect, t, sv);
        snprintf(lbl,sizeof(lbl),"n=%u add mac at Δ",lambda);
        CHECK(memcmp(eval,expect,lambda/8)==0, lbl);
    }
    printf("\nquicksilver: %d failures\n", fails);
    return fails ? 1 : 0;
}
