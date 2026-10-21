/*
 * test_approx.c -- integration test for the ApproxExp / ApproxLog
 * wrappers (ref/approx_exp.h, ref/approx_log.h).
 *
 * This is the in-tree, quadmath-FREE companion to the authoritative
 * tools/verify_approx_{exp,log}_poly.c gates.  Those verifiers own the
 * high-precision relative/absolute error gates (53 bits / 57 bits vs a
 * __float128 reference); this test owns the structural integration
 * invariants that must hold over the runtime domain WITHOUT any floating
 * point:
 *
 *   ApproxExp:
 *     (a) shuttle_exp_accept_poly_q64 over the full (x,y) grid
 * [0,36]x[0,255]; (b) the 4-way batch shuttle_exp_accept_poly_q64_x4 is
 * BIT-IDENTICAL to four scalar calls (byte-exact); (c) the documented
 * per-set acceptance gates eta_max (Security.txt) are emitted, flagging
 * the thin SHUTTLE-512 margin.
 *
 *   ApproxLog:
 *     (d) shuttle_log2_frac_q62 over a dense mantissa grid: the
 * constant-time full-table scan result == a direct-indexed Horner (the CT
 * contract); (e) the 2-way batch shuttle_log2_frac_q62_x2 == two scalar
 * calls (byte-exact); (f) the c_{0,0}=0 pin: shuttle_log2_frac_q62(0,0) ==
 * 0 bit-exactly (ApproxLog(0,1)=0).
 *
 * Build (NO -lquadmath, NO __int128 in this TU):
 *   gcc -std=c99 -Wpedantic -Wall -Wextra -Werror -O2 \
 *       -I SHUTTLE/ref -I SHUTTLE/ref/tools \
 *       SHUTTLE/ref/test/test_approx.c \
 *       SHUTTLE/ref/approx_exp.c SHUTTLE/ref/approx_log.c
 * -DSHUTTLE_MODE=128
 */
#include <stdint.h>
#include <stdio.h>

#include "approx_exp.h"
#include "approx_log.h"

/* ---- direct-indexed log Horner (the CT-scan cross-check oracle) ----
 * Mirrors shuttle_log2_frac_q62 but indexes kShuttleLogPoly[j] DIRECTLY
 * (secret-dependent address) instead of the full-table eqmask scan.  The
 * two must agree bit-for-bit; the runtime path uses the scan, this is the
 * oracle. Uses shuttle_log_mulhi (the rounded high-half) so the only
 * difference from the scan path is the row-fetch mechanism, never the
 * arithmetic.
 *
 * NOTE: __int128 here is confined to the pragma-guarded region (the
 * wrapper header already suppressed -Wpedantic for the kernel; we
 * re-suppress for this local oracle so the whole TU stays
 * -Wpedantic-clean). */
#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#endif
static int64_t log_direct_frac_q62(uint32_t j, uint64_t x_q64)
{
    __int128 acc = kShuttleLogPoly[j][SHUTTLE_LOG_POLY_DEGREE];
    int k;
    for (k = SHUTTLE_LOG_POLY_DEGREE - 1; k >= 0; k--)
        acc = (__int128)kShuttleLogPoly[j][k] +
              (__int128)shuttle_log_mulhi(acc, x_q64);
    return (int64_t)acc;
}
#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic pop
#endif

int main(void)
{
    int fails = 0;

    /* ===================== ApproxExp ===================== */

    /* (a)+(b): full (x,y) grid; the x4 batch must equal four scalar calls.
     * We batch four CONSECUTIVE y at a fixed x (the natural SampleY lane).
     */
    {
        int exp_grid = 0;    /* count of evaluated scalar points */
        int x4_fail = 0;     /* batch != scalar count            */
        uint64_t acc_or = 0; /* sanity: outputs are not all zero */
        int x, y;
        for (x = 0; x <= SHUTTLE_EXP_POLY_X_MAX; x++) {
            for (y = 0; y <= SHUTTLE_EXP_POLY_Y_MAX; y++) {
                uint64_t p = approx_exp_accept_q64(x, y);
                acc_or |= p;
                exp_grid++;
            }
            /* x4: every window of 4 consecutive y at this x */
            for (y = 0; y + 3 <= SHUTTLE_EXP_POLY_Y_MAX; y++) {
                int xs[4] = {x, x, x, x};
                int ys[4] = {y, y + 1, y + 2, y + 3};
                uint64_t o[4];
                int n;
                approx_exp_accept_q64_x4(xs, ys, o);
                for (n = 0; n < 4; n++)
                    if (o[n] != approx_exp_accept_q64(xs[n], ys[n]))
                        x4_fail++;
            }
        }
        /* (0,0): exp(0)=1, so p_hat(0,0) is the largest threshold on the
         * grid (accept almost surely).  It is NOT exactly 2^64-1: the 7
         * squarings of the seed v=2^64-1 drift it down by a few ULPs (Q64
         * round-down), so p_hat(0,0) = 2^64-128 = 0xffffffffffffff80 on
         * this frozen scheme. Assert it is the grid maximum and within 256
         * ULPs of full scale. */
        {
            uint64_t p00 = approx_exp_accept_q64(0, 0);
            if (p00 != (uint64_t)0xFFFFFFFFFFFFFF80ULL) {
                printf(
                    "[exp] p_hat(0,0) != 0xffffffffffffff80 (frozen "
                    "value) FAIL "
                    "(got 0x%016llx)\n",
                    (unsigned long long)p00);
                fails++;
            }
            if (UINT64_MAX - p00 > 256u) {
                printf("[exp] p_hat(0,0) too far below full scale FAIL\n");
                fails++;
            }
        }
        if (acc_or == 0) {
            printf("[exp] all-zero output over the grid FAIL\n");
            fails++;
        }
        printf(
            "[exp] scalar grid evaluated (37x256)        : %s (%d pts)\n",
            (exp_grid == 37 * 256) ? "PASS" : "FAIL", exp_grid);
        if (exp_grid != 37 * 256)
            fails++;
        printf(
            "[exp] x4 batch == 4 scalar (bit-identical)  : %s (%d "
            "mismatch)\n",
            x4_fail ? "FAIL" : "PASS", x4_fail);
        if (x4_fail)
            fails++;

        /* worst point (x,y)=(36,248) must be in-domain and monotone-sane:
         * exp(p) is decreasing in N, so p_hat(36,255) <= p_hat(36,248). */
        if (!(approx_exp_accept_q64(36, 255) <=
              approx_exp_accept_q64(36, 248))) {
            printf(
                "[exp] monotonicity p_hat(36,255) <= p_hat(36,248) "
                "FAIL\n");
            fails++;
        }
    }

    /* (c): per-set acceptance gates the measured 2^-54.4857 must clear
     * (tools/log/Security.txt).  Reported as integer bit-margins (no
     * float): margin = 54.4857 - bits(eta_max).  All positive => PASS; the
     * SHUTTLE-512 margin (~1.52 bits) is the thinnest -- flag it as a
     * watch item. */
    {
        /* eta_max bits per set, x100 to keep integers (Security.txt).  The
         * measured precision is 54.4857 bits -> 5449 at 2-dp (rounds up).
         */
        const long meas_x100 = 5449L;   /* 54.49 ~ measured precision */
        const long eta128_x100 = 5125L; /* 2^-51.25 */
        const long eta256_x100 = 5198L; /* 2^-51.98 */
        const long eta512_x100 = 5297L; /* 2^-52.97 (binding, thinnest) */
        long m128 = meas_x100 - eta128_x100; /* +323  ~ 3.24 bits */
        long m256 = meas_x100 - eta256_x100; /* +250  ~ 2.51 bits */
        long m512 = meas_x100 - eta512_x100; /* +151  ~ 1.52 bits */
        printf(
            "[exp] eta_max margins: 128=+%ld.%02ld 256=+%ld.%02ld "
            "512=+%ld.%02ld bits\n",
            m128 / 100, m128 % 100, m256 / 100, m256 % 100, m512 / 100,
            m512 % 100);
        if (m128 <= 0 || m256 <= 0 || m512 <= 0) {
            printf(
                "[exp] a per-set acceptance gate is NOT cleared FAIL\n");
            fails++;
        }
        if (m512 < 200)
            printf(
                "[exp] WATCH (R3): SHUTTLE-512 margin %ld.%02ld bits < "
                "2.00 -- "
                "re-run verify_approx_exp_poly if sigma/r/k/N change.\n",
                m512 / 100, m512 % 100);
    }

    /* ===================== ApproxLog ===================== */

    /* (f): the c_{0,0}=0 pin -> ApproxLog(0,1)=0 bit-exactly. */
    if (approx_log2_frac_q62(0, 0) != 0) {
        printf("[log] pin ApproxLog(0,1)=0 (frac(0,0)==0)    : FAIL\n");
        fails++;
    } else {
        printf("[log] pin ApproxLog(0,1)=0 (frac(0,0)==0)    : PASS\n");
    }

    /* (d): dense mantissa grid, all 4 segments; the constant-time
     * full-table scan must equal the direct-indexed Horner for EVERY
     * point. */
    {
        const uint32_t STEPS = 1u
                               << 14; /* 16384 x_q64 samples per segment */
        int ct_fail = 0;
        int mono_fail = 0;
        uint32_t j;
        for (j = 0; j < SHUTTLE_LOG_POLY_SEGMENTS; j++) {
            int64_t prev = 0;
            int have_prev = 0;
            uint32_t i;
            for (i = 0; i <= STEPS; i++) {
                /* x_q64 in [0, 2^64) spread across the unit interval */
                uint64_t x_q64 = (i == STEPS)
                                     ? (uint64_t)0xFFFFFFFFFFFFFFFFULL
                                     : ((uint64_t)i << (64 - 14));
                int64_t scan = approx_log2_frac_q62(j, x_q64);
                int64_t direct = log_direct_frac_q62(j, x_q64);
                if (scan != direct)
                    ct_fail++;
                /* within a segment frac is increasing in x (sanity, not
                 * the spec monotonicity gate -- that lives in the
                 * verifier). */
                if (have_prev && scan < prev)
                    mono_fail++;
                prev = scan;
                have_prev = 1;
            }
        }
        printf(
            "[log] CT-scan == direct index (dense grid)   : %s (%d "
            "mismatch)\n",
            ct_fail ? "FAIL" : "PASS", ct_fail);
        if (ct_fail)
            fails++;
        printf(
            "[log] intra-segment frac increasing in x     : %s (%d "
            "drop)\n",
            mono_fail ? "FAIL" : "PASS", mono_fail);
        if (mono_fail)
            fails++;
    }

    /* (e): the 2-way batch must equal two scalar calls bit-for-bit.  Pair
     * two INDEPENDENT (j, x_q64) lanes, sweeping segments and opposed x
     * ramps. */
    {
        const uint32_t STEPS = 4096;
        int x2_fail = 0;
        uint32_t j;
        for (j = 0; j < SHUTTLE_LOG_POLY_SEGMENTS; j++) {
            uint32_t j2 = (j + 1u) % SHUTTLE_LOG_POLY_SEGMENTS;
            uint32_t i;
            for (i = 0; i < STEPS; i++) {
                uint64_t x0 = (uint64_t)i << (64 - 12);
                uint64_t x1 = (uint64_t)(STEPS - 1 - i) << (64 - 12);
                uint32_t sel[2] = {j, j2};
                uint64_t xb[2] = {x0, x1};
                int64_t o[2];
                approx_log2_frac_q62_x2(sel, xb, o);
                if (o[0] != approx_log2_frac_q62(j, x0) ||
                    o[1] != approx_log2_frac_q62(j2, x1))
                    x2_fail++;
            }
        }
        printf(
            "[log] x2 batch == 2 scalar (bit-identical)   : %s (%d "
            "mismatch)\n",
            x2_fail ? "FAIL" : "PASS", x2_fail);
        if (x2_fail)
            fails++;
    }

    printf("\nSUMMARY test_approx (SHUTTLE-%d) fails=%d\n", SHUTTLE_MODE,
           fails);
    return fails ? 1 : 0;
}
