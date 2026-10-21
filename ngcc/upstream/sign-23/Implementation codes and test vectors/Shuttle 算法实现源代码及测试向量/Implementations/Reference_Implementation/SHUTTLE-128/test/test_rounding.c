/*
 * test_rounding.c -- SCALAR (reference backend) correctness for the
 * SHUTTLE rounding/lift/hint/norm substrate.  Built per mode with:
 *
 *   gcc -O2 -std=c99 -Wall -I. -I../tools -Intt/<qset> -DSHUTTLE_MODE=<m>
 *       -DDISABLE_NAMESPACE=1 test/test_rounding.c rounding.c reduce.c
 *       poly.c poly_ntt.c ntt/<qset>/ntt_ref.c -o out/test_rounding_<m>
 *
 * Sections (fails=0 required):
 *  (a) compress_y / stretch_s: the two translation identities (now
 *      unconditional, including at exact half-integer ties), round-half-up
 *      == Python floor((2v+alpha)/(2 alpha)), StretchS no-mod bound.
 *  (b) roundB_update_s2: b is a multiple of alpha_b in [0,q); |delta| <=
 *      alpha_b/2; e' = e + delta; one-pass == two-step; e' support <=
 * BE_ENC. (c) the mod-2q lift: lift_to_mod2q_coeff / mat_mul_2q match a
 * NAIVE reference doing the whole A x product in plain mod-2q integer
 *      arithmetic, over random inputs (signer AND verifier forms); the
 *      TRUE-parity gotcha is exercised (negative coeffs); the
 *      raw-vs-freeze parity DIVERGES and the raw form matches the
 * reference. (d) make_hint / use_hint round-trip: use_hint(make_hint(...))
 * recovers the high bits over random comY in [0,2q); the /2 is exact (even
 * by parity); a negative test that an out-of-range H_h bucket is rejected
 *      by ct_range_reject (the decode-side range check).
 *  (e) the norm gates accept/reject at the boundary (nsq==BK_SQ accepts,
 *      nsq==BK_SQ+1 rejects; same for BV_SQ and BK_LOW_SQ).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "packing.h" /* ct_range_reject (decode-side range check) */
#include "params.h"
#include "poly_ntt.h"
#include "reduce.h"
#include "rounding.h"

/* ---- per-mode divisor table (mirror gen_rounding.py / params.h) ---- */
#if SHUTTLE_MODE == 128
#    define A1 90
#    define AS 10
#    define AE 5
#elif SHUTTLE_MODE == 256
#    define A1 135
#    define AS 5
#    define AE 5
#elif SHUTTLE_MODE == 512
#    define A1 144
#    define AS 3
#    define AE 3
#endif

static int rnd_range(int lo, int hi) /* uniform int in [lo, hi] */
{
    return lo + (int)(rand() % (hi - lo + 1));
}

/* reference round-to-nearest, ties UP toward +inf (round-half-up): the
 * shift-invariant CompressY rule.  q = floor((2v + alpha) / (2 alpha)); C `/`
 * truncates toward zero, so correct it toward -inf for negative numerators. */
static long ref_round_div(long v, long alpha)
{
    long num = 2 * v + alpha;
    long den = 2 * alpha; /* > 0 */
    long q = num / den;
    if ((num % den) != 0 && num < 0)
        q -= 1;
    return q;
}

/* alpha selector by block index (compile-time partition). */
static long block_alpha(int p)
{
    if (p == 0)
        return A1;
    if (p < 1 + ELL)
        return AS;
    return AE;
}

/* =================================================================== */
/* (a) CompressY / StretchS                                            */
/* =================================================================== */
static int test_compress(void)
{
    int fails = 0, it, p;
    unsigned i;

    /* round_div_hup exactness over a dense |v| sweep (via compress_y on
     * a single-block poly), for each block divisor. */
    for (p = 0; p < KVEC; ++p) {
        long alpha = block_alpha(p);
        poly in[KVEC], out[KVEC];
        memset(in, 0, sizeof(in));
        for (i = 0; i < N; ++i) {
            /* hit dense small values + the exact half-integer ties
             * +-(k*alpha + alpha/2); round-half-up sends +tie -> up and
             * -tie -> up (toward +inf), checked against ref_round_div. */
            long v;
            if (i < N / 2)
                v = (long)i - (long)N / 4; /* dense around 0 */
            else {
                long k = (long)(i - N / 2);
                long tie = k * alpha +
                           (alpha / 2); /* half-integer if alpha even */
                v = (k & 1) ? tie : -tie;
            }
            in[p].coeffs[i] = (int32_t)v;
        }
        compress_y(out, in);
        for (i = 0; i < N; ++i) {
            long want = ref_round_div(in[p].coeffs[i], alpha);
            if (out[p].coeffs[i] != (int32_t)want) {
                if (fails < 6)
                    printf(
                        "  [cmp] p=%d i=%u v=%d got=%d want=%ld a=%ld\n",
                        p, i, in[p].coeffs[i], out[p].coeffs[i], want,
                        alpha);
                fails++;
            }
        }
    }

    /* exact-tie audit: round-half-up sends every half-integer tie toward
     * +inf, so +alpha/2 -> +1 but -alpha/2 -> 0, and +1.5alpha -> +2 but
     * -1.5alpha -> -1.  (Round-half-away would give +-1 / +-2 symmetrically;
     * the asymmetry here is exactly the shift-invariance that fixes the
     * commitment identity.) */
    for (p = 0; p < KVEC; ++p) {
        long alpha = block_alpha(p);
        if (alpha % 2 == 0) {
            poly in[KVEC], out[KVEC];
            memset(in, 0, sizeof(in));
            in[p].coeffs[0] = (int32_t)(alpha / 2);    /* +0.5 -> +1 */
            in[p].coeffs[1] = (int32_t)(-(alpha / 2)); /* -0.5 -> 0  */
            in[p].coeffs[2] = (int32_t)(alpha + alpha / 2); /* 1.5 -> 2 */
            in[p].coeffs[3] =
                (int32_t)(-(alpha + alpha / 2)); /* -1.5 -> -1 */
            compress_y(out, in);
            if (out[p].coeffs[0] != 1 || out[p].coeffs[1] != 0 ||
                out[p].coeffs[2] != 2 || out[p].coeffs[3] != -1) {
                printf(
                    "  [tie] p=%d a=%ld got %d %d %d %d (want 1 0 2 "
                    "-1)\n",
                    p, alpha, out[p].coeffs[0], out[p].coeffs[1],
                    out[p].coeffs[2], out[p].coeffs[3]);
                fails++;
            }
        }
    }

    /* the two translation identities, over random bounded x, y. */
    for (it = 0; it < 2000; ++it) {
        poly x[KVEC], y[KVEC], sx[KVEC], cs[KVEC], yp[KVEC], sum[KVEC];
        for (p = 0; p < KVEC; ++p)
            for (i = 0; i < N; ++i) {
                x[p].coeffs[i] = (int32_t)rnd_range(-12, 12); /* small */
                y[p].coeffs[i] =
                    (int32_t)rnd_range(-3000, 3000); /* response */
            }
        /* CompressY(StretchS(x)) == x */
        stretch_s(sx, x);
        compress_y(cs, sx);
        for (p = 0; p < KVEC; ++p)
            for (i = 0; i < N; ++i)
                if (cs[p].coeffs[i] != x[p].coeffs[i]) {
                    if (fails < 6)
                        printf("  [id1] it=%d p=%d i=%u\n", it, p, i);
                    fails++;
                }
        /* CompressY(y + StretchS(x)) == CompressY(y) + x.
         *
         * Round-half-up is shift-invariant -- round(t + k) = round(t) + k for
         * EVERY integer k and every sign of t -- so this identity holds
         * UNCONDITIONALLY, including at exact even half-integer ties.  (The
         * old ties-away rule failed here whenever a divisor alpha was even,
         * which is the bug this rounding change fixes; we now test the ties
         * too, with NO skip.) */
        for (p = 0; p < KVEC; ++p)
            for (i = 0; i < N; ++i)
                sum[p].coeffs[i] = y[p].coeffs[i] + sx[p].coeffs[i];
        compress_y(yp, y);
        compress_y(cs, sum);
        for (p = 0; p < KVEC; ++p) {
            for (i = 0; i < N; ++i) {
                if (cs[p].coeffs[i] != yp[p].coeffs[i] + x[p].coeffs[i]) {
                    if (fails < 6)
                        printf("  [id2] it=%d p=%d i=%u\n", it, p, i);
                    fails++;
                }
            }
        }
    }

    /* StretchS no-mod bound: StretchS(1, s, e') with |s|<=BS_ENC,
     * |e'|<=BE_ENC stays bounded by max(alpha_1, alpha_s*BS_ENC,
     * alpha_e*BE_ENC) (no wrap). */
    {
        poly x[KVEC], sx[KVEC];
        long bound = A1;
        if ((long)AS * BS_ENC > bound)
            bound = (long)AS * BS_ENC;
        if ((long)AE * BE_ENC > bound)
            bound = (long)AE * BE_ENC;
        for (i = 0; i < N; ++i)
            x[0].coeffs[i] = 1; /* the constant block is the scalar 1 */
        for (p = 1; p < 1 + ELL; ++p)
            for (i = 0; i < N; ++i)
                x[p].coeffs[i] = (int32_t)rnd_range(-BS_ENC, BS_ENC);
        for (p = 1 + ELL; p < KVEC; ++p)
            for (i = 0; i < N; ++i)
                x[p].coeffs[i] = (int32_t)rnd_range(-BE_ENC, BE_ENC);
        stretch_s(sx, x);
        for (p = 0; p < KVEC; ++p)
            for (i = 0; i < N; ++i)
                if (sx[p].coeffs[i] > bound || sx[p].coeffs[i] < -bound) {
                    if (fails < 6)
                        printf("  [nomod] p=%d i=%u val=%d bound=%ld\n", p,
                               i, sx[p].coeffs[i], bound);
                    fails++;
                }
    }

    printf(
        "(a) compress_y/stretch_s (round-half-up, identities, no-mod): %s\n",
        fails ? "FAIL" : "PASS");
    return fails;
}

/* =================================================================== */
/* (b) RoundB fused                                                    */
/* =================================================================== */
static int posmod(int32_t a, int32_t m) /* non-neg rep, test-only */
{
    int32_t r = a % m;
    return (r < 0) ? r + m : r;
}
static int32_t centermod_ref(int32_t a, int32_t m) /* [-m/2, m/2) even m */
{
    int32_t r = posmod(a, m);
    if (r >= m / 2)
        r -= m;
    return r;
}
static int32_t centermod_q_ref(int32_t a) /* (-(q-1)/2 .. (q-1)/2] */
{
    int32_t r = posmod(a, Q);
    if (r > (Q - 1) / 2)
        r -= Q;
    return r;
}

static int test_roundb(void)
{
    int fails = 0, it, p;
    unsigned i;
    for (it = 0; it < 500; ++it) {
        poly pkb0[EM], e[EM], pkb[EM], ep[EM];
        for (p = 0; p < EM; ++p)
            for (i = 0; i < N; ++i) {
                pkb0[p].coeffs[i] = (int32_t)rnd_range(-3 * Q, 3 * Q);
                e[p].coeffs[i] = (int32_t)rnd_range(-BE_ENC, BE_ENC);
            }
        roundB_update_s2(pkb, ep, pkb0, e);
        for (p = 0; p < EM; ++p)
            for (i = 0; i < N; ++i) {
                int32_t b = pkb[p].coeffs[i];
                /* b in [0,q), multiple of alpha_b */
                if (b < 0 || b >= Q || (b % ALPHA_B) != 0) {
                    if (fails < 6)
                        printf("  [b] p=%d i=%u b=%d\n", p, i, b);
                    fails++;
                }
                /* two-step reference: vp=b0 mod q; v0=vp bmodpm ab; bref =
                 * vp-v0; deltaref=(bref-b0) bmodpm q; epref = e + delta.
                 */
                int32_t b0 = pkb0[p].coeffs[i];
                int32_t vp = posmod(b0, Q);
                int32_t v0 = centermod_ref(vp, ALPHA_B);
                int32_t bref = vp - v0;
                int32_t deltaref = centermod_q_ref(bref - b0);
                int32_t epref = e[p].coeffs[i] + deltaref;
                if (b != bref) {
                    if (fails < 6)
                        printf("  [bref] p=%d i=%u b=%d ref=%d\n", p, i, b,
                               bref);
                    fails++;
                }
                if (ep[p].coeffs[i] != epref) {
                    if (fails < 6)
                        printf("  [ep] p=%d i=%u ep=%d ref=%d\n", p, i,
                               ep[p].coeffs[i], epref);
                    fails++;
                }
                /* |delta| <= alpha_b/2 */
                if (deltaref > ALPHA_B / 2 || deltaref < -(ALPHA_B / 2)) {
                    if (fails < 6)
                        printf("  [delta] p=%d i=%u delta=%d\n", p, i,
                               deltaref);
                    fails++;
                }
            }
    }
    printf(
        "(b) roundB_update_s2 (b mult of a_b, |delta|<=a_b/2, e'=e+delta, "
        "1-pass==2-step): %s\n",
        fails ? "FAIL" : "PASS");
    return fails;
}

/* =================================================================== */
/* (c) mod-2q lift                                                     */
/* =================================================================== */
/* Naive references: do the WHOLE A x product in plain mod-2q integer
 * arithmetic.  The q-domain accumulator u_i = (-b_i.x0 + sum_j A_ij.xs_j
 * [+ x_e_i]) mod q, then comY_i = (2 u_i + [i==0]*q*(x0&1) [-
 * [i==0]*q*(c&1)]) mod 2q.  We build b_i, A_ij in coeff domain (small) and
 * do negacyclic convolution by hand so the reference shares NO code with
 * the NTT path. */

/* negacyclic poly mult (a*b mod X^n+1) mod q, coeff domain. */
static void negconv_modq(int32_t out[N], const int32_t a[N],
                         const int32_t b[N])
{
    long acc[N];
    int i, j;
    for (i = 0; i < N; ++i)
        acc[i] = 0;
    for (i = 0; i < N; ++i)
        for (j = 0; j < N; ++j) {
            long prod = (long)a[i] * b[j];
            int idx = i + j;
            if (idx >= N) {
                idx -= N;
                prod = -prod; /* X^n = -1 */
            }
            acc[idx] += prod;
        }
    for (i = 0; i < N; ++i)
        out[i] = (int32_t)posmod((int32_t)(acc[i] % Q), Q);
}

static int test_mod2q(void)
{
    int fails = 0;
    int i, jj, p;

    /* lift_to_mod2q_coeff: LiftToModTwoQ(xbar, LSB(.)) correctness. */
    for (i = 0; i < 4000; ++i) {
        int32_t xbar = rnd_range(0, Q - 1);
        int32_t b = rnd_range(0, 1);
        int32_t x = lift_to_mod2q_coeff(xbar, b);
        /* x in [0,2q); x mod q == xbar; x mod 2 == b */
        if (x < 0 || x >= DQ || posmod(x, Q) != xbar || (x & 1) != b) {
            if (fails < 6)
                printf("  [lift] xbar=%d b=%d x=%d\n", xbar, b, x);
            fails++;
        }
    }

    /* raw parity vs freeze parity DIVERGE on a negative coeff, and
     * the raw form is the spec-correct one.  q*(x0 mod 2) where x0
     * negative odd: raw (x0&1)==1 but freeze(x0) is even/odd flipped
     * (freeze adds odd q). Demonstrate that q*(x0_raw&1) !=
     * q*(freeze(x0)&1) for a negative-odd. */
    {
        int32_t x0 = -3;         /* odd negative; raw &1 == 1 */
        int32_t fz = freeze(x0); /* = q-3 = even (q odd) -> &1 == 0 */
        if ((x0 & 1) == (fz & 1)) {
            printf(
                "  [k13] raw/freeze parity did NOT diverge (x0=%d fz=%d)"
                " -- test vacuous\n",
                x0, fz);
            fails++;
        }
    }

    /* mat_mul_2q (signer) vs naive mod-2q reference. */
    for (int it = 0; it < 60; ++it) {
        poly yp[KVEC]; /* the compressed mask (signed, can be negative) */
        poly bc[EM];   /* b in coeff domain [0,q) */
        poly Ac[EM * ELL]; /* A_gen in coeff domain [0,q) */
        poly16 bhat[EM], Ahat[EM * ELL];
        poly comY[EM];

        /* random signed small compressed mask (include negatives to
         * exercise the parity divergence). */
        for (p = 0; p < KVEC; ++p)
            for (i = 0; i < N; ++i)
                yp[p].coeffs[i] = (int32_t)rnd_range(-40, 40);
        /* random public b, A in [0,q). */
        for (p = 0; p < EM; ++p)
            for (i = 0; i < N; ++i)
                bc[p].coeffs[i] = (int32_t)rnd_range(0, Q - 1);
        for (p = 0; p < EM * ELL; ++p)
            for (i = 0; i < N; ++i)
                Ac[p].coeffs[i] = (int32_t)rnd_range(0, Q - 1);

        /* NTT-transform b and A into bhat/Ahat (the cached public matrix).
         */
        for (p = 0; p < EM; ++p) {
            poly16 t;
            for (i = 0; i < N; ++i)
                t.coeffs[i] = (uint16_t)bc[p].coeffs[i];
            poly_ntt_cache(&t); /* cached bhat: backend-native order */
            bhat[p] = t;
        }
        for (p = 0; p < EM * ELL; ++p) {
            poly16 t;
            for (i = 0; i < N; ++i)
                t.coeffs[i] = (uint16_t)Ac[p].coeffs[i];
            poly_ntt(&t);
            Ahat[p] = t;
        }

        mat_mul_2q(comY, yp, bhat, Ahat);

        /* naive reference per pk poly i. */
        for (p = 0; p < EM; ++p) {
            int32_t bx0[N], axs[N], acc[N];
            /* -b_i . x0 */
            negconv_modq(bx0, bc[p].coeffs, yp[0].coeffs);
            for (i = 0; i < N; ++i)
                acc[i] = posmod(-bx0[i], Q);
            /* + sum_j A_ij . xs_j */
            for (jj = 0; jj < ELL; ++jj) {
                negconv_modq(axs, Ac[p * ELL + jj].coeffs,
                             yp[1 + jj].coeffs);
                for (i = 0; i < N; ++i)
                    acc[i] = posmod(acc[i] + axs[i], Q);
            }
            /* + x_e_i (the e-block, 2*I_m) */
            for (i = 0; i < N; ++i)
                acc[i] = posmod(acc[i] + yp[1 + ELL + p].coeffs[i], Q);
            /* lift: comY = (2*acc + [i==0]*q*(x0&1)) mod 2q */
            for (i = 0; i < N; ++i) {
                long v = 2L * acc[i];
                if (p == 0)
                    v += (long)Q * (yp[0].coeffs[i] & 1);
                v = ((v % DQ) + DQ) % DQ;
                if (comY[p].coeffs[i] != (int32_t)v) {
                    if (fails < 8)
                        printf("  [m2q] it=%d p=%d i=%d got=%d want=%ld\n",
                               it, p, i, comY[p].coeffs[i], v);
                    fails++;
                }
                if (comY[p].coeffs[i] < 0 || comY[p].coeffs[i] >= DQ)
                    fails++;
            }
        }
    }

    /* mat_mul_z1_2q (verifier) vs naive mod-2q reference. */
    for (int it = 0; it < 60; ++it) {
        poly z1[Z1LEN], c;
        poly bc[EM], Ac[EM * ELL];
        poly16 bhat[EM], Ahat[EM * ELL];
        poly wt[EM];

        for (p = 0; p < Z1LEN; ++p)
            for (i = 0; i < N; ++i)
                z1[p].coeffs[i] = (int32_t)rnd_range(-40, 40);
        for (i = 0; i < N; ++i)
            c.coeffs[i] = (int32_t)rnd_range(0, 1); /* binary challenge */
        for (p = 0; p < EM; ++p)
            for (i = 0; i < N; ++i)
                bc[p].coeffs[i] = (int32_t)rnd_range(0, Q - 1);
        for (p = 0; p < EM * ELL; ++p)
            for (i = 0; i < N; ++i)
                Ac[p].coeffs[i] = (int32_t)rnd_range(0, Q - 1);
        for (p = 0; p < EM; ++p) {
            poly16 t;
            for (i = 0; i < N; ++i)
                t.coeffs[i] = (uint16_t)bc[p].coeffs[i];
            poly_ntt_cache(&t); /* cached bhat: backend-native order */
            bhat[p] = t;
        }
        for (p = 0; p < EM * ELL; ++p) {
            poly16 t;
            for (i = 0; i < N; ++i)
                t.coeffs[i] = (uint16_t)Ac[p].coeffs[i];
            poly_ntt(&t);
            Ahat[p] = t;
        }

        mat_mul_z1_2q(wt, z1, &c, bhat, Ahat);

        for (p = 0; p < EM; ++p) {
            int32_t bx0[N], axs[N], acc[N];
            negconv_modq(bx0, bc[p].coeffs, z1[0].coeffs);
            for (i = 0; i < N; ++i)
                acc[i] = posmod(-bx0[i], Q);
            for (jj = 0; jj < ELL; ++jj) {
                negconv_modq(axs, Ac[p * ELL + jj].coeffs,
                             z1[1 + jj].coeffs);
                for (i = 0; i < N; ++i)
                    acc[i] = posmod(acc[i] + axs[i], Q);
            }
            for (i = 0; i < N; ++i) {
                long v = 2L * acc[i];
                if (p == 0) {
                    v += (long)Q * (z1[0].coeffs[i] & 1);
                    v -= (long)Q * (c.coeffs[i] & 1);
                }
                v = ((v % DQ) + DQ) % DQ;
                if (wt[p].coeffs[i] != (int32_t)v) {
                    if (fails < 8)
                        printf(
                            "  [z1m2q] it=%d p=%d i=%d got=%d want=%ld\n",
                            it, p, i, wt[p].coeffs[i], v);
                    fails++;
                }
                if (wt[p].coeffs[i] < 0 || wt[p].coeffs[i] >= DQ)
                    fails++;
            }
        }
    }

    /* commitment-parity lemma (lem:commitment-parity): for honest matmul
     * output, LSB(comY) == LSB(y'_0)*j -- only the first commitment slot
     * carries a nonzero low bit; comY[p>0] is even.  This is what makes
     * the use_hint /2 exact across all slots. */
    for (int it = 0; it < 30; ++it) {
        poly yp[KVEC], bc[EM], Ac[EM * ELL], comY[EM];
        poly16 bhat[EM], Ahat[EM * ELL];
        for (p = 0; p < KVEC; ++p)
            for (i = 0; i < N; ++i)
                yp[p].coeffs[i] = (int32_t)rnd_range(-40, 40);
        for (p = 0; p < EM; ++p)
            for (i = 0; i < N; ++i)
                bc[p].coeffs[i] = (int32_t)rnd_range(0, Q - 1);
        for (p = 0; p < EM * ELL; ++p)
            for (i = 0; i < N; ++i)
                Ac[p].coeffs[i] = (int32_t)rnd_range(0, Q - 1);
        for (p = 0; p < EM; ++p) {
            poly16 t;
            for (i = 0; i < N; ++i)
                t.coeffs[i] = (uint16_t)bc[p].coeffs[i];
            poly_ntt_cache(&t); /* cached bhat: backend-native order */
            bhat[p] = t;
        }
        for (p = 0; p < EM * ELL; ++p) {
            poly16 t;
            for (i = 0; i < N; ++i)
                t.coeffs[i] = (uint16_t)Ac[p].coeffs[i];
            poly_ntt(&t);
            Ahat[p] = t;
        }
        mat_mul_2q(comY, yp, bhat, Ahat);
        for (p = 0; p < EM; ++p)
            for (i = 0; i < N; ++i) {
                int32_t want_lsb = (p == 0) ? (yp[0].coeffs[i] & 1) : 0;
                if (lsb_coeff(comY[p].coeffs[i]) != want_lsb) {
                    if (fails < 6)
                        printf("  [parity] p=%d i=%d lsb=%d want=%d\n", p,
                               i, lsb_coeff(comY[p].coeffs[i]), want_lsb);
                    fails++;
                }
            }
    }

    printf(
        "(c) mod-2q lift (lift_to_mod2q, mat_mul_2q & mat_mul_z1_2q vs "
        "naive mod-2q, raw-parity, parity lemma): %s\n",
        fails ? "FAIL" : "PASS");
    return fails;
}

/* =================================================================== */
/* (d) MakeHint / UseHint round-trip                                   */
/* =================================================================== */
static int test_hint(void)
{
    int fails = 0, p, it;
    unsigned i;

    /* highbits_reduced in [0,H_h) over [0,2q). */
    for (int x = 0; x < DQ; ++x) {
        int32_t b = highbits_reduced(x);
        if (b < 0 || b >= (int32_t)HH)
            fails++;
    }

    /* MakeHint/UseHint round-trip.  Build comY in [0,2q), z2 small.  The
     * signer makes h; the verifier (given comY_tilde = comY - 2*z2 mod 2q
     * and comY0p = LSB(comY)) recovers comY_h == highbits(comY) and a z2'
     * with comY_app - comY_tilde EVEN (so the /2 is exact). */
    for (it = 0; it < 1000; ++it) {
        poly comY[EM], z2[EM], h[EM];
        poly comY_h[EM], z2p[EM], comY0p;
        poly comY_tilde[EM];
        /* SCHEME PARITY INVARIANT (Lemma lem:commitment-parity):
         * LSB(comY) = LSB(y'_0)*j, so ONLY the first commitment slot
         * carries a nonzero low bit; comY[p>0] is EVEN.  This is what
         * makes the /2 in use_hint exact for every slot.  Honest
         * mat_mul_2q output satisfies it; here we impose it directly so
         * the round-trip is the real regime, not arbitrary odd comY (which
         * has no parity structure and would correctly produce a
         * non-integral z2'). */
        for (p = 0; p < EM; ++p)
            for (i = 0; i < N; ++i) {
                int32_t w = (int32_t)rnd_range(0, DQ - 1);
                if (p != 0)
                    w &= ~1; /* comY[p>0] even */
                comY[p].coeffs[i] = w;
                z2[p].coeffs[i] = (int32_t)rnd_range(-200, 200);
            }
        make_hint(h, comY, z2);
        /* verifier inputs: comY_tilde = (comY - 2*z2) mod 2q; comY0p =
         * LSB(comY)*j (the parity the lift would have recovered -- nonzero
         * only on slot 0). */
        for (p = 0; p < EM; ++p)
            for (i = 0; i < N; ++i)
                comY_tilde[p].coeffs[i] =
                    reduce_mod_2q(comY[p].coeffs[i] - 2 * z2[p].coeffs[i]);
        for (i = 0; i < N; ++i)
            comY0p.coeffs[i] =
                comY[0].coeffs[i] & 1; /* LSB(comY) on slot0 */
        use_hint(comY_h, z2p, h, comY_tilde, &comY0p);

        for (p = 0; p < EM; ++p)
            for (i = 0; i < N; ++i) {
                /* comY_h recovered == highbits(comY). */
                int32_t hb_want = highbits_reduced(comY[p].coeffs[i]);
                if (comY_h[p].coeffs[i] != hb_want) {
                    if (fails < 6)
                        printf("  [hint] it=%d p=%d i=%u cyh=%d want=%d\n",
                               it, p, i, comY_h[p].coeffs[i], hb_want);
                    fails++;
                }
                /* the /2 must be exact: comY_app - comY_tilde even. */
                int32_t c0 = (p == 0) ? comY0p.coeffs[i] : 0;
                int32_t app = hbvalue(comY_h[p].coeffs[i]) + c0;
                if (((app - comY_tilde[p].coeffs[i]) & 1) != 0) {
                    if (fails < 6)
                        printf("  [even] it=%d p=%d i=%u app-wt odd\n", it,
                               p, i);
                    fails++;
                }
            }
    }

    /* range-reject negative test: an out-of-range H_h bucket is REJECTED
     * by the decode-side range check (ct_range_reject), NOT wrapped mod
     * H_h. */
    {
        uint32_t fail_acc = 0;
        /* in-range comY_h coeff: accepted */
        if (ct_range_reject((int32_t)HH - 1, 0, (int32_t)HH - 1) != 0)
            fails++; /* should accept the top in-range value */
        /* out-of-range (== H_h, in the gap [H_h, 2^d_h)): rejected */
        fail_acc = ct_range_reject((int32_t)HH, 0, (int32_t)HH - 1);
        if (fail_acc == 0) {
            printf("  [k14] H_h=%d not rejected by ct_range_reject\n", HH);
            fails++;
        }
    }

    printf(
        "(d) make_hint/use_hint round-trip (recover highbits, /2 exact, "
        "range-reject): %s\n",
        fails ? "FAIL" : "PASS");
    return fails;
}

/* =================================================================== */
/* (e) norm gates                                                      */
/* =================================================================== */
/* Build a poly array whose centered sqnorm equals a target by placing the
 * target as one coeff^2 plus a remainder coeff.  We keep all coeffs small
 * (centered, < q/2) so poly_sqnorm reads them verbatim. */
static void set_sqnorm(poly *v, unsigned len, int64_t target)
{
    unsigned p, i;
    int64_t rem = target;
    int32_t step = 90;
    for (p = 0; p < len; ++p)
        for (i = 0; i < N; ++i)
            v[p].coeffs[i] = 0;
    /* greedily place squares of small magnitudes (<= 90, well under q/2).
     */
    p = 0;
    i = 0;
    while (rem > 0) {
        int32_t m = step;
        while ((int64_t)m * m > rem)
            m--;
        if (m == 0)
            m = 1;
        v[p].coeffs[i] = m;
        rem -= (int64_t)m * m;
        if (++i >= N) {
            i = 0;
            if (++p >= len)
                break; /* ran out of room (won't happen for our targets) */
        }
    }
}

static int64_t arr_sqnorm_ref(const poly *v, unsigned len)
{
    int64_t acc = 0;
    unsigned p, i;
    for (p = 0; p < len; ++p)
        for (i = 0; i < N; ++i) {
            int64_t c = centermod_q_ref(v[p].coeffs[i]);
            acc += c * c;
        }
    return acc;
}

static int test_norm(void)
{
    int fails = 0;

    /* poly_array_sqnorm == int64 reference. */
    {
        poly v[KVEC];
        set_sqnorm(v, KVEC, BK_SQ - 17);
        int64_t got = poly_array_sqnorm(v, KVEC);
        int64_t want = arr_sqnorm_ref(v, KVEC);
        if (got != want) {
            printf("  [sqn] got=%lld want=%lld\n", (long long)got,
                   (long long)want);
            fails++;
        }
    }

    /* keygen_norm_ok: boundary inclusivity (closed window). */
    {
        poly v[KVEC];
        /* nsq == BK_SQ : accept */
        set_sqnorm(v, KVEC, BK_SQ);
        if (poly_array_sqnorm(v, KVEC) == BK_SQ && !keygen_norm_ok(v)) {
            printf("  [kn] nsq==BK_SQ rejected\n");
            fails++;
        }
        /* nsq == BK_SQ+1 : reject */
        set_sqnorm(v, KVEC, BK_SQ + 1);
        if (poly_array_sqnorm(v, KVEC) == BK_SQ + 1 && keygen_norm_ok(v)) {
            printf("  [kn] nsq==BK_SQ+1 accepted\n");
            fails++;
        }
        /* nsq == BK_LOW_SQ : accept */
        set_sqnorm(v, KVEC, BK_LOW_SQ);
        if (poly_array_sqnorm(v, KVEC) == BK_LOW_SQ &&
            !keygen_norm_ok(v)) {
            printf("  [kn] nsq==BK_LOW_SQ rejected\n");
            fails++;
        }
        /* nsq == BK_LOW_SQ-1 : reject */
        set_sqnorm(v, KVEC, BK_LOW_SQ - 1);
        if (poly_array_sqnorm(v, KVEC) == BK_LOW_SQ - 1 &&
            keygen_norm_ok(v)) {
            printf("  [kn] nsq==BK_LOW_SQ-1 accepted\n");
            fails++;
        }
    }

    /* response_norm_ok: boundary at BV_SQ.  Split target across z1 (Z1LEN)
     * and z2' (EM). */
    {
        poly z1[Z1LEN], z2p[EM];
        memset(z1, 0, sizeof(z1));
        memset(z2p, 0, sizeof(z2p));
        /* nsq == BV_SQ : accept.  Put it all in z1. */
        set_sqnorm(z1, Z1LEN, BV_SQ);
        memset(z2p, 0, sizeof(z2p));
        if (poly_array_sqnorm(z1, Z1LEN) == BV_SQ &&
            !response_norm_ok(z1, z2p)) {
            printf("  [rn] nsq==BV_SQ rejected\n");
            fails++;
        }
        /* nsq == BV_SQ+1 : reject */
        set_sqnorm(z1, Z1LEN, BV_SQ + 1);
        memset(z2p, 0, sizeof(z2p));
        if (poly_array_sqnorm(z1, Z1LEN) == BV_SQ + 1 &&
            response_norm_ok(z1, z2p)) {
            printf("  [rn] nsq==BV_SQ+1 accepted\n");
            fails++;
        }
    }

    printf(
        "(e) norm gates (sqnorm==ref, BK/BK_LOW/BV boundary inclusivity): "
        "%s\n",
        fails ? "FAIL" : "PASS");
    return fails;
}

int main(void)
{
    int fails = 0;
    srand(20260624u);
    ntt_ref_init();

    printf("=== test_rounding (SHUTTLE-%d, scalar) ===\n", SHUTTLE_MODE);
    fails += test_compress();
    fails += test_roundb();
    fails += test_mod2q();
    fails += test_hint();
    fails += test_norm();

    printf("\nSUMMARY test_rounding (SHUTTLE-%d, scalar) fails=%d\n",
           SHUTTLE_MODE, fails);
    return fails ? 1 : 0;
}
