/*
 * quicksilver.c — QuickSilver authenticated-element arithmetic. See quicksilver.h.
 *
 * The MAC polynomial model: an element of degree d carries d+1 field-element
 * coefficients; mac(Δ) = Σ coeffs[i]·Δ^i. The plaintext is coeffs[deg].
 *   - add: degree stays max(deg_a, deg_b); coefficients add position-wise.
 *   - mul (a deg<=1, b deg<=d): result degree d+1 (capped at 3); coefficient
 *     of Δ^k in (a·b) = Σ_{i+j=k} a_i·b_j. Since a is deg<=1, k in {j, j+1}.
 *   - mulc (public c): coefficients all scale by c, degree unchanged.
 */
#include "quicksilver.h"
#include <string.h>

void qs_const(qs_elem* e, const qs_party* p, const gf_limb_t* value) {
    (void)p;
    memset(e, 0, sizeof(*e));
    e->deg = 0;
    gf_copy(p->fc, e->coeffs[0], value);
}

void qs_lift(qs_elem* e, const qs_party* p, const gf_limb_t* tag, const gf_limb_t* val) {
    /* mac(Δ) = val + tag·Δ  =>  coeffs[0] = val (Δ^0), coeffs[1] = tag (Δ^1). */
    memset(e, 0, sizeof(*e));
    e->deg = 1;
    gf_copy(p->fc, e->coeffs[0], val);   /* constant term = plaintext */
    gf_copy(p->fc, e->coeffs[1], tag);   /* Δ coefficient = MAC tag */
}

void qs_zero(qs_elem* e) {
    memset(e, 0, sizeof(*e));
    e->deg = 0;
}

void qs_add(const qs_party* p, qs_elem* out, const qs_elem* a, const qs_elem* b) {
    unsigned d = a->deg > b->deg ? a->deg : b->deg;
    qs_elem r;
    memset(&r, 0, sizeof(r));
    r.deg = d;
    for (unsigned i = 0; i <= d; ++i) {
        gf_limb_t ai[GF_LIMBS(512)], bi[GF_LIMBS(512)];
        gf_zero(p->fc, ai); gf_zero(p->fc, bi);
        if (i <= a->deg) gf_copy(p->fc, ai, a->coeffs[i]);
        if (i <= b->deg) gf_copy(p->fc, bi, b->coeffs[i]);
        gf_add(p->fc, r.coeffs[i], ai, bi);
    }
    *out = r;
}

void qs_mul(const qs_party* p, qs_elem* out, const qs_elem* a, const qs_elem* b) {
    /* a must be deg <= 1; result deg = a->deg + b->deg (capped 3). */
    unsigned da = a->deg, db = b->deg;
    unsigned dr = da + db;
    if (dr > 3) dr = 3;
    qs_elem r;
    memset(&r, 0, sizeof(r));
    r.deg = dr;
    /* convolution: r_k = Σ_{i+j=k, i<=da, j<=db} a_i·b_j */
    for (unsigned i = 0; i <= da; ++i) {
        for (unsigned j = 0; j <= db; ++j) {
            unsigned k = i + j;
            if (k > 3) continue;
            gf_limb_t prod[GF_LIMBS(512)];
            gf_mul(p->fc, prod, a->coeffs[i], b->coeffs[j]);
            gf_limb_t tmp[GF_LIMBS(512)];
            gf_add(p->fc, tmp, r.coeffs[k], prod);
            gf_copy(p->fc, r.coeffs[k], tmp);
        }
    }
    *out = r;
}

void qs_mulc(const qs_party* p, qs_elem* out, const qs_elem* a, const gf_limb_t* c) {
    qs_elem r = *a;
    for (unsigned i = 0; i <= a->deg; ++i) {
        gf_limb_t prod[GF_LIMBS(512)];
        gf_mul(p->fc, prod, a->coeffs[i], c);
        gf_copy(p->fc, r.coeffs[i], prod);
    }
    *out = r;
}

void qs_eval_at(const qs_party* p, gf_limb_t* out, const qs_elem* e) {
    /* Horner from the top: out = e->coeffs[deg]; then out = out·Δ + coeffs[i]. */
    gf_limb_t acc[GF_LIMBS(512)];
    gf_copy(p->fc, acc, e->coeffs[e->deg]);
    for (int i = (int)e->deg - 1; i >= 0; --i) {
        gf_limb_t t[GF_LIMBS(512)];
        gf_mul(p->fc, t, acc, p->delta);
        gf_add(p->fc, acc, t, e->coeffs[i]);
    }
    gf_copy(p->fc, out, acc);
}
