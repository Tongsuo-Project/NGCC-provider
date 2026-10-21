/*
 * test_ntt.c -- SCALAR (reference backend) correctness for the SHUTTLE NTT
 * shim, adapted from each NTT module's test_ntt.c.  Exercises the
 * per-mode poly_ntt / poly_invntt_tomont / poly_pointwise_montgomery /
 * poly_ntt_canonical / poly_ntt_import shim, plus the raw s<n>_*_ref
 * oracle.
 *
 * Built per mode with: gcc ... -DSHUTTLE_MODE=<m> test/test_ntt.c reduce.c
 *   poly_ntt.c ntt/<qset>/ntt_ref.c
 *
 * Asserts (fails=0 required):
 *   [1] round-trip: poly_invntt_tomont(poly_ntt(a)) == a*R mod q  (5000
 * polys) [2] negacyclic product via ntt->pointwise->invntt_tomont ==
 * schoolbook (polymul_schoolbook), 2000 pairs [3]
 * poly_ntt_import(poly_ntt_canonical(p)) == poly_ntt(p) (mod q): the
 *       canonical->backend-native bridge is order-consistent (no-op for
 * ref, so this also checks the ref native order IS the canonical order),
 * 2000 [4] per-config readback: the [0,q) output convention LiftToModTwoQ
 * needs. The shim canonicalizes to [0,q) for every config, so we read
 * uint16 here; the SIGNED-int16 (smod) vs UNSIGNED ((uint16_t)x) raw-lane
 *       distinction is exercised against the asm in test_freeze_avx.c.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "params.h"
#include "poly_ntt.h"

#define MONT16 ((uint16_t)((1u << 16) % Q))

/* canonical [0,q) of a value already reduced to [0,q) (uniform readback
 * after the shim canonicalizes). */
static uint16_t U(uint16_t x)
{
    return (uint16_t)(x % Q);
}

int main(void)
{
    srand(20260624);
    ntt_ref_init();

    /* [1] round-trip poly_invntt_tomont(poly_ntt(a)) == a*R mod q */
    int f1 = 0;
    for (int it = 0; it < 5000; it++) {
        poly16 a, ref;
        for (int i = 0; i < N; i++) {
            uint16_t v = (uint16_t)(rand() % Q);
            a.coeffs[i] = v;
            ref.coeffs[i] = (uint16_t)((uint32_t)v * MONT16 % Q);
        }
        poly_ntt(&a);
        poly_invntt_tomont(&a);
        for (int i = 0; i < N; i++)
            if (U(a.coeffs[i]) != ref.coeffs[i]) {
                if (f1 < 4)
                    printf("  [rt] it=%d i=%d got=%u exp=%u\n", it, i,
                           U(a.coeffs[i]), ref.coeffs[i]);
                f1++;
                break;
            }
    }
    printf(
        "[1] scalar round-trip invntt_tomont(ntt(a)) == a*R, 5000: %s "
        "(%d)\n",
        f1 ? "FAIL" : "PASS", f1);

    /* [2] negacyclic product == schoolbook */
    int f2 = 0;
    for (int it = 0; it < 2000; it++) {
        poly16 a, b, na, nb, c;
        uint16_t cref[N];
        for (int i = 0; i < N; i++) {
            a.coeffs[i] = (uint16_t)(rand() % Q);
            b.coeffs[i] = (uint16_t)(rand() % Q);
            na.coeffs[i] = a.coeffs[i];
            nb.coeffs[i] = b.coeffs[i];
        }
        poly_ntt(&na);
        poly_ntt(&nb);
        poly_pointwise_montgomery(&c, &na, &nb);
        poly_invntt_tomont(&c);
        polymul_schoolbook(a.coeffs, b.coeffs, cref);
        for (int i = 0; i < N; i++)
            if (U(c.coeffs[i]) != cref[i]) {
                if (f2 < 4)
                    printf("  [mul] it=%d i=%d got=%u exp=%u\n", it, i,
                           U(c.coeffs[i]), cref[i]);
                f2++;
                break;
            }
    }
    printf(
        "[2] negacyclic product (ntt+pointwise+invntt) == schoolbook, "
        "2000: "
        "%s (%d)\n",
        f2 ? "FAIL" : "PASS", f2);

    /* [3] poly_ntt_import(poly_ntt_canonical(p)) == poly_ntt(p) (mod q).
     * For the ref backend poly_ntt_import is a no-op and
     * poly_ntt_canonical == poly_ntt, so this confirms the native order IS
     * the canonical order. */
    int f3 = 0;
    for (int it = 0; it < 2000; it++) {
        poly16 p, want;
        for (int i = 0; i < N; i++) {
            uint16_t v = (uint16_t)(rand() % Q);
            p.coeffs[i] = v;
            want.coeffs[i] = v;
        }
        poly_ntt_canonical(&p);
        poly_ntt_import(&p);
        poly_ntt(&want);
        for (int i = 0; i < N; i++)
            if (U(p.coeffs[i]) != U(want.coeffs[i])) {
                if (f3 < 4)
                    printf("  [imp] it=%d i=%d got=%u exp=%u\n", it, i,
                           U(p.coeffs[i]), U(want.coeffs[i]));
                f3++;
                break;
            }
    }
    printf("[3] import(canonical(p)) == ntt(p) (mod q), 2000: %s (%d)\n",
           f3 ? "FAIL" : "PASS", f3);

    /* [4] readback / [0,q) output convention: every coeff of an
     * invntt_tomont output lands in [0,q) (the LiftToModTwoQ input
     * convention). */
    int f4 = 0;
    for (int it = 0; it < 1000; it++) {
        poly16 a;
        for (int i = 0; i < N; i++)
            a.coeffs[i] = (uint16_t)(rand() % Q);
        poly_ntt(&a);
        poly_invntt_tomont(&a);
        for (int i = 0; i < N; i++)
            if (a.coeffs[i] >= (uint16_t)Q) {
                f4++;
                break;
            }
    }
    printf(
        "[4] invntt output in [0,q) (LiftToModTwoQ-ready), 1000: %s "
        "(%d)\n",
        f4 ? "FAIL" : "PASS", f4);

    int fails = f1 + f2 + f3 + f4;
    printf("\nSUMMARY test_ntt (SHUTTLE-%d, scalar) fails=%d\n",
           SHUTTLE_MODE, fails);
    return fails ? 1 : 0;
}
