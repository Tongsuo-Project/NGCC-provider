#include <math.h>
#include <quadmath.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Exercise the IN-TREE wrapper (ref/approx_exp.h), which #include's the
 * frozen tools/approx_exp_poly.h and re-exports the same entry points.
 * This makes the authoritative verifier gate the actual integration
 * surface, not just the raw autogen header.  Build with: -I. -I.. (so both
 * "approx_exp.h" and the "tools/approx_exp_poly.h" it includes resolve).
 */
#include "approx_exp.h"

#define BLISS_R 825
#define BLISS_K 256
#define TARGET_BITS 53.0Q

static __float128 reference_exp(int x, int y)
{
    uint64_t n = (uint64_t)y * (uint64_t)(y + 2 * BLISS_K * x);
    return expq(-((__float128)n) /
                ((__float128)(2ULL * BLISS_R * BLISS_R)));
}

static __float128 poly_probability(int x, int y)
{
    uint64_t q = shuttle_exp_accept_poly_q64(x, y);
    return ((__float128)q) / ldexpq(1.0Q, 64);
}

int main(void)
{
    __float128 max_rel = 0.0Q;
    int worst_x = 0;
    int worst_y = 0;
    for (int x = 0; x <= SHUTTLE_EXP_POLY_X_MAX; x++) {
        for (int y = 0; y <= SHUTTLE_EXP_POLY_Y_MAX; y++) {
            __float128 ref = reference_exp(x, y);
            __float128 got = poly_probability(x, y);
            __float128 rel = fabsq(got - ref) / ref;
            if (rel > max_rel) {
                max_rel = rel;
                worst_x = x;
                worst_y = y;
            }
        }
    }
    /* performance-optimal 4-way batched variant must be bit-identical to
     * scalar */
    int x4_ok = 1;
    for (int x = 0; x <= SHUTTLE_EXP_POLY_X_MAX && x4_ok; x++)
        for (int y = 0; y <= SHUTTLE_EXP_POLY_Y_MAX - 3; y++) {
            int xs[4] = {x, x, x, x};
            int ys[4] = {y, y + 1, y + 2, y + 3};
            uint64_t o[4];
            shuttle_exp_accept_poly_q64_x4(xs, ys, o);
            for (int n = 0; n < 4; n++)
                if (o[n] != shuttle_exp_accept_poly_q64(xs[n], ys[n])) {
                    x4_ok = 0;
                    break;
                }
        }

    __float128 bits = -logq(max_rel) / logq(2.0Q);
    char err_s[128];
    char bits_s[128];
    quadmath_snprintf(err_s, sizeof(err_s), "%.40Qe", max_rel);
    quadmath_snprintf(bits_s, sizeof(bits_s), "%.20Qf", bits);
    printf("max accept relative error = %s\n", err_s);
    printf("accept precision          = %s bits\n", bits_s);
    printf("worst point               = x=%d y=%d\n", worst_x, worst_y);
    printf("x4 batched == scalar      = %s\n", x4_ok ? "PASS" : "FAIL");
    return (bits >= TARGET_BITS && x4_ok) ? EXIT_SUCCESS : EXIT_FAILURE;
}
