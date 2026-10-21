/*
 * dudect.h -- minimal dudect-style timing-leakage t-test harness for SHUTTLE.
 *
 * Self-contained port of the dudect methodology (Reparaz, Balasch, Verbauwhede,
 * "Dude, is my code constant time?", DATE 2017). The caller provides a
 * `do_one_computation(uint8_t *data)` that runs the function under test on one
 * input, and a `prepare_inputs()` that fills `inputs` and `classes` with two
 * input classes (class 0 = fixed, class 1 = random) -- the leakage hypothesis
 * is "execution time depends on the input class".
 *
 * Methodology / thresholds (documented in SECRET_PUBLIC_AUDIT / TIMECOP):
 *   - We collect one cycle measurement per (class, input) using rdtsc (the
 *     harness defines its own cycle counter; no external cpucycles.c needed).
 *   - We apply the standard dudect post-processing: drop the top
 *     PERCENTILE_CROP tail (cache/scheduler outliers), then run Welch's t-test
 *     between the two classes (the "first-order" leakage detector). A
 *     cropped-percentile family of t-tests is computed; max |t| over the family
 *     is the reported statistic.
 *   - VERDICT: |t| < T_THRESHOLD_BORDER (5)  -> "no leakage detected"
 *              |t| < T_THRESHOLD_FAIL  (10)  -> "borderline / WARN"
 *              |t| >= T_THRESHOLD_FAIL (10)  -> "potential leak / FAIL"
 *     (dudect's own |t|>10 rule of thumb; these are NOISY on a shared machine /
 *     under WSL, so a single run is a SMOKE, not proof -- see component-vs-sign
 *     gating policy.)
 *
 * This header is deliberately small and dependency-free (libm only for sqrt in
 * the t-statistic, which is on PUBLIC measurement data, never secret).
 */
#ifndef SHUTTLE_DUDECT_H
#define SHUTTLE_DUDECT_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- cycle counter (x86 rdtsc; serialized with rdtscp where available) ---- */
static inline uint64_t dudect_cpucycles(void)
{
#if defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi)::"%rcx");
    return ((uint64_t)hi << 32) | lo;
#else
    /* portable fallback: not a real cycle counter, but keeps the harness
     * buildable. */
    return (uint64_t)clock();
#endif
}

/* ---- t-test thresholds ---- */
#define DUDECT_T_BORDER 5.0  /* |t| below this: clearly no first-order leak    */
#define DUDECT_T_FAIL 10.0   /* |t| at/above this: dudect "definitely leaking" */

/* Number of cropped-percentile t-tests in the family (dudect default). */
#define DUDECT_NUMBER_PERCENTILES 100

/* Welch's t online accumulator for one (sub)test. */
typedef struct {
    double mean[2];
    double m2[2];
    double n[2];
} dudect_ttest_t;

static void dudect_ttest_init(dudect_ttest_t *t)
{
    memset(t, 0, sizeof *t);
}

static void dudect_ttest_push(dudect_ttest_t *t, double x, int cls)
{
    t->n[cls] += 1.0;
    double delta = x - t->mean[cls];
    t->mean[cls] += delta / t->n[cls];
    t->m2[cls] += delta * (x - t->mean[cls]);
}

static double dudect_ttest_compute(const dudect_ttest_t *t)
{
    if (t->n[0] < 2.0 || t->n[1] < 2.0)
        return 0.0;
    double var0 = t->m2[0] / (t->n[0] - 1.0);
    double var1 = t->m2[1] / (t->n[1] - 1.0);
    double num = t->mean[0] - t->mean[1];
    double den = sqrt(var0 / t->n[0] + var1 / t->n[1]);
    if (den == 0.0)
        return 0.0;
    return num / den;
}

/*
 * dudect_run_ttests: given parallel arrays of `n` cycle measurements and class
 * labels, compute the cropped-percentile family of Welch t-tests and return the
 * maximum |t| over the family. `report_name` is printed with the verdict.
 *
 * Returns max |t|.  Caller decides PASS/WARN/FAIL from the thresholds.
 */
static double dudect_run_ttests(const char *report_name, const int64_t *cycles,
                                const uint8_t *classes, size_t n)
{
    /* The plain (uncropped) t-test plus a family cropped at increasing
     * percentiles of the per-sample max, to suppress positive-tail outliers. */
    int64_t maxc = 0;
    for (size_t i = 0; i < n; i++)
        if (cycles[i] > maxc)
            maxc = cycles[i];

    double best = 0.0;
    /* test 0: no crop */
    {
        dudect_ttest_t tt;
        dudect_ttest_init(&tt);
        for (size_t i = 0; i < n; i++)
            dudect_ttest_push(&tt, (double)cycles[i], classes[i] ? 1 : 0);
        double t = dudect_ttest_compute(&tt);
        if (fabs(t) > best)
            best = fabs(t);
    }
    /* family: crop at thresholds spaced log-ish across [0, maxc]. */
    for (int p = 1; p < DUDECT_NUMBER_PERCENTILES; p++) {
        /* dudect's percentile schedule: 1 - 2^(-10 * p/N). */
        double frac = 1.0 - pow(2.0, -10.0 * (double)p / DUDECT_NUMBER_PERCENTILES);
        int64_t crop = (int64_t)(frac * (double)maxc);
        dudect_ttest_t tt;
        dudect_ttest_init(&tt);
        for (size_t i = 0; i < n; i++) {
            if (cycles[i] < crop)
                dudect_ttest_push(&tt, (double)cycles[i], classes[i] ? 1 : 0);
        }
        double t = dudect_ttest_compute(&tt);
        if (fabs(t) > best)
            best = fabs(t);
    }

    const char *verdict =
        (best < DUDECT_T_BORDER)
            ? "PASS (no first-order leak)"
            : (best < DUDECT_T_FAIL ? "WARN (borderline)" : "FAIL (potential leak)");
    printf("dudect %s: max|t|=%.2f n=%zu -> %s\n", report_name, best, n, verdict);
    return best;
}

#endif /* SHUTTLE_DUDECT_H */
