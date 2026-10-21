/* Independent __float128 audit of the generated segmented base-2
 * logarithm.
 *
 * Gate (spec eta_log): max absolute error of log2(b), b in [1,2), must
 * stay below 2^-57.  We also re-check, in C, every safety/correctness
 * invariant the Python generator asserts:
 *   - bit-exact ApproxLog(0,1)=0   (segment 0, x=0 -> frac == 0);
 *   - global strict monotonicity of the represented value in b;
 *   - the constant-time full-table row scan equals direct indexing.
 *
 * Build:  gcc -O2 -I. -I.. verify_approx_log_poly.c -lquadmath -o
 * verify_log
 */
#include <quadmath.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Exercise the IN-TREE wrapper (ref/approx_log.h), which #include's the
 * frozen tools/approx_log_poly.h and re-exports the same entry points --
 * so the authoritative verifier gate covers the actual integration
 * surface. */
#include "approx_log.h"

#define SAMPLES_PER_SEG (1u << 18)
#define TARGET_BITS 57.0Q
#define QSCALE ldexpq(1.0Q, SHUTTLE_LOG_POLY_QBITS) /* 2^62 */

/* Mantissa precision: b = 1 + m/2^KAPPA_B.  Valid inputs are the dyadic b
 * at this resolution, so monotonicity is required only at KAPPA_B
 * granularity, not at the full 2^64 sub-mantissa resolution.  The reduced
 * argument carries KAPPA_B - g bits.  Any KAPPA_B <= 58 keeps the
 * cross-segment rounding wobble
 * (~d ULP ~ 2^-59) below one mantissa step's log2 increase. */
#ifndef KAPPA_B
#    define KAPPA_B 57 /* spec value (Description.tex parameter table) */
#endif
#define RBITS (KAPPA_B - SHUTTLE_LOG_POLY_G) /* reduced-argument bits */
/* snap a Q64 fraction down to the KAPPA_B-representable mantissa grid */
static inline uint64_t snap_kb(uint64_t x_q64)
{
    return (x_q64 >> (64 - RBITS)) << (64 - RBITS);
}

/* reference: log2(beta_j + (x_q64/2^64) * 2^-g) */
static __float128 reference_log2(uint32_t j, uint64_t x_q64)
{
    __float128 beta =
        1.0Q + (__float128)j / (__float128)SHUTTLE_LOG_POLY_SEGMENTS;
    __float128 x = (__float128)x_q64 / ldexpq(1.0Q, 64);
    __float128 step = 1.0Q / (__float128)SHUTTLE_LOG_POLY_SEGMENTS;
    __float128 b = beta + x * step;
    return logq(b) / logq(2.0Q);
}

/* direct-indexed Horner (cross-check against the constant-time scan path)
 */
static int64_t direct_frac_q62(uint32_t j, uint64_t x_q64)
{
    __int128 acc = kShuttleLogPoly[j][SHUTTLE_LOG_POLY_DEGREE];
    for (int k = SHUTTLE_LOG_POLY_DEGREE - 1; k >= 0; k--)
        acc = (__int128)kShuttleLogPoly[j][k] +
              (__int128)shuttle_log_mulhi(acc, x_q64);
    return (int64_t)acc;
}

int main(void)
{
    __float128 max_abs = 0.0Q;
    uint32_t worst_j = 0;
    uint64_t worst_x = 0;
    int ct_scan_ok = 1;
    int pin_zero_ok;
    int64_t prev = 0;
    int have_prev = 0;
    int64_t worst_drop =
        0; /* most negative (frac - prev) at KAPPA_B res */
    uint64_t x_kbmax = (((uint64_t)1 << RBITS) - 1) << (64 - RBITS);

    /* ApproxLog(0,1)=0 : segment 0, x=0 must read exactly 0 */
    pin_zero_ok = (shuttle_log2_frac_q62(0, 0) == 0);

    for (uint32_t j = 0; j < SHUTTLE_LOG_POLY_SEGMENTS; j++) {
        for (uint32_t i = 0; i <= SAMPLES_PER_SEG; i++) {
            /* precision: full sub-mantissa resolution (worst case for
             * error) */
            uint64_t x_q64 =
                (i == SAMPLES_PER_SEG)
                    ? x_kbmax
                    : ((__uint128_t)i << 64) / SAMPLES_PER_SEG;

            int64_t frac = shuttle_log2_frac_q62(j, x_q64);
            if (frac != direct_frac_q62(j, x_q64))
                ct_scan_ok = 0;

            __float128 got = (__float128)frac / QSCALE;
            __float128 ref = reference_log2(j, x_q64);
            __float128 e = fabsq(got - ref);
            if (e > max_abs) {
                max_abs = e;
                worst_j = j;
                worst_x = x_q64;
            }

            /* monotonicity at the VALID (KAPPA_B-resolution) inputs: snap
             * x to the mantissa grid; the (j,x) scan order equals
             * increasing u. */
            int64_t fk = shuttle_log2_frac_q62(j, snap_kb(x_q64));
            if (have_prev) {
                int64_t d = fk - prev;
                if (d < worst_drop)
                    worst_drop = d;
            }
            prev = fk;
            have_prev = 1;
        }
    }
    int mono_ok = (worst_drop >= 0);
    __float128 drop_bits =
        (worst_drop < 0)
            ? -logq((__float128)(-worst_drop) / QSCALE) / logq(2.0Q)
            : 999.0Q;

    /* performance-optimal 2-way batched variant must be bit-identical to
     * scalar */
    int x2_ok = 1;
    for (uint32_t j = 0; j < SHUTTLE_LOG_POLY_SEGMENTS && x2_ok; j++)
        for (uint32_t i = 0; i < 4096; i++) {
            uint32_t j2 = (j + 1u) % SHUTTLE_LOG_POLY_SEGMENTS;
            uint64_t x0 = ((__uint128_t)i << 64) / 4096;
            uint64_t x1 = ((__uint128_t)(4095 - i) << 64) / 4096;
            uint32_t s[2] = {j, j2};
            uint64_t xb[2] = {x0, x1};
            int64_t o[2];
            shuttle_log2_frac_q62_x2(s, xb, o);
            if (o[0] != shuttle_log2_frac_q62(j, x0) ||
                o[1] != shuttle_log2_frac_q62(j2, x1)) {
                x2_ok = 0;
                break;
            }
        }

    __float128 bits = -logq(max_abs) / logq(2.0Q);
    char es[128], bs[128];
    quadmath_snprintf(es, sizeof es, "%.40Qe", max_abs);
    quadmath_snprintf(bs, sizeof bs, "%.20Qf", bits);
    printf("segments=%d degree=%d Q%d\n", SHUTTLE_LOG_POLY_SEGMENTS,
           SHUTTLE_LOG_POLY_DEGREE, SHUTTLE_LOG_POLY_QBITS);
    printf("max abs error = %s\n", es);
    printf("precision      = %s bits  (worst j=%u x=0x%016llx)\n", bs,
           worst_j, (unsigned long long)worst_x);
    printf("ApproxLog(0,1)=0    : %s\n", pin_zero_ok ? "PASS" : "FAIL");
    if (mono_ok) {
        printf("monotone @ kappa_b=%d: PASS (no drop)\n", KAPPA_B);
    } else {
        char ds[128];
        quadmath_snprintf(ds, sizeof ds, "%.2Qf", drop_bits);
        printf("monotone @ kappa_b=%d: FAIL (worst drop 2^-%s)\n", KAPPA_B,
               ds);
    }
    printf("CT-scan == direct   : %s\n", ct_scan_ok ? "PASS" : "FAIL");
    printf("x2 batched == scalar: %s\n", x2_ok ? "PASS" : "FAIL");
    int ok = (bits >= TARGET_BITS) && pin_zero_ok && mono_ok &&
             ct_scan_ok && x2_ok;
    printf("RESULT             : %s (need >= %.1f bits)\n",
           ok ? "PASS" : "FAIL", (double)TARGET_BITS);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
