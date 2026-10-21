#include "cpucycles.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint64_t cpucycles_overhead(void)
{
    uint64_t t0, t1, overhead = -1LL;
    unsigned int i;

    for (i = 0; i < 100000; i++) {
        t0 = cpucycles();
        __asm__ volatile("");
        t1 = cpucycles();
        if (t1 - t0 < overhead)
            overhead = t1 - t0;
    }

    return overhead;
}

int cpucycles_cmp_uint64(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return x < y ? -1 : x > y;
}

uint64_t cpucycles_median(uint64_t *t, size_t tlen)
{
    if (tlen == 0) {
        fprintf(stderr, "ERROR: Need at least one cycle count!\n");
        return 0;
    }

    qsort(t, tlen, sizeof(uint64_t), cpucycles_cmp_uint64);
    return t[tlen / 2];
}

static uint64_t bench_average(const uint64_t *t, size_t tlen)
{
    size_t i;
    uint64_t acc = 0;

    for (i = 0; i < tlen; i++)
        acc += t[i];

    return acc / tlen;
}

static uint64_t bench_percentile(const uint64_t *sorted, size_t len,
                                 unsigned int pct)
{
    size_t idx;

    if (pct >= 100)
        return sorted[len - 1];

    /* Nearest-rank percentile: ceil(pct * len / 100) - 1. */
    idx = (pct * len + 99) / 100;
    if (idx == 0)
        idx = 1;

    return sorted[idx - 1];
}

static uint64_t bench_virtual_octile_mean(const uint64_t *sorted,
                                          size_t len, unsigned int lo,
                                          unsigned int hi)
{
    size_t i;
    uint64_t acc = 0;
    size_t start = (size_t)lo * len;
    size_t end = (size_t)hi * len;
    size_t count = end - start;

    /* Cycle counts fit in ~40 bits and `count` is bounded by the run
     * count, so the running sum stays far below 2^64; no wide
     * accumulator is needed. */
    for (i = start; i < end; i++)
        acc += sorted[i / 8];

    return acc / count;
}

static uint64_t bench_stddev(const uint64_t *t, size_t tlen, uint64_t avg)
{
    size_t i;
    double sumsq = 0.0;

    /* Variance of cycle timings is a human-facing statistic; accumulate
     * the sum of squared deviations in `double` to avoid 128-bit integer
     * math. */
    for (i = 0; i < tlen; i++) {
        double delta = (double)t[i] - (double)avg;
        sumsq += delta * delta;
    }

    return (uint64_t)sqrt(sumsq / (double)tlen);
}

static size_t bench_outlier_count(const uint64_t *sorted, size_t len,
                                  uint64_t p50)
{
    size_t i, count = 0;
    /* p50 is a cycle count (~40 bits); p50*3 and sorted[i]*2 stay well
     * within 64 bits, so the comparison needs no wider accumulator. */
    uint64_t threshold = p50 * 3;

    for (i = 0; i < len; i++) {
        if (sorted[i] * 2 > threshold)
            count++;
    }

    return count;
}

static uint64_t bench_ratio_x100(uint64_t numerator, uint64_t denominator)
{
    if (denominator == 0)
        return 0;
    /* `numerator` is at most a cycle count scaled by 100, so a further
     * factor of 100 stays below 2^64; plain 64-bit math is exact here. */
    return (numerator * 100 + denominator / 2) / denominator;
}

int bench_tail_sign(const char *name, tail_sign_result *result,
                    bench_tail_sign_fn sign_once, void *ctx)
{
    uint64_t *cycles;
    uint64_t overhead;
    int ret;
    size_t i;

    if (result == NULL || sign_once == NULL) {
        fprintf(stderr, "ERROR: bench_tail_sign got NULL argument!\n");
        return -1;
    }

    cycles = malloc(BENCH_SIGN_RUNS * sizeof(uint64_t));
    if (cycles == NULL) {
        fprintf(stderr, "ERROR: bench_tail_sign malloc failed!\n");
        return -1;
    }

    for (i = 0; i < BENCH_WARMUP; i++) {
        ret = sign_once(ctx);
        if (ret != 0) {
            free(cycles);
            return ret;
        }
    }

    overhead = cpucycles_overhead();
    for (i = 0; i < BENCH_SIGN_RUNS; i++) {
        uint64_t t0 = cpucycles();
        ret = sign_once(ctx);
        cycles[i] = cpucycles() - t0 - overhead;
        if (ret != 0) {
            free(cycles);
            return ret;
        }
    }

    qsort(cycles, BENCH_SIGN_RUNS, sizeof(uint64_t), cpucycles_cmp_uint64);

    result->name = name;
    result->runs = BENCH_SIGN_RUNS;
    result->p25 = bench_percentile(cycles, BENCH_SIGN_RUNS, 25);
    result->p50 = bench_percentile(cycles, BENCH_SIGN_RUNS, 50);
    result->p75 = bench_percentile(cycles, BENCH_SIGN_RUNS, 75);
    result->p90 = bench_percentile(cycles, BENCH_SIGN_RUNS, 90);
    result->p99 = bench_percentile(cycles, BENCH_SIGN_RUNS, 99);
    result->stq1 =
        bench_virtual_octile_mean(cycles, BENCH_SIGN_RUNS, 1, 3);
    result->stq2 =
        bench_virtual_octile_mean(cycles, BENCH_SIGN_RUNS, 3, 5);
    result->stq3 =
        bench_virtual_octile_mean(cycles, BENCH_SIGN_RUNS, 5, 7);
    result->avg = bench_average(cycles, BENCH_SIGN_RUNS);
    result->stddev = bench_stddev(cycles, BENCH_SIGN_RUNS, result->avg);
    result->cv_x100 = bench_ratio_x100(result->stddev * 100, result->avg);
    result->outlier_count =
        bench_outlier_count(cycles, BENCH_SIGN_RUNS, result->p50);
    result->p90_p50_x100 = bench_ratio_x100(result->p90, result->p50);
    result->p99_p50_x100 = bench_ratio_x100(result->p99, result->p50);

    free(cycles);
    return 0;
}

static void bench_print_ratio(uint64_t ratio_x100)
{
    printf("%llu.%02llux", (unsigned long long)(ratio_x100 / 100),
           (unsigned long long)(ratio_x100 % 100));
}

void bench_print_tail(const tail_sign_result *result)
{
    const char *name;

    if (result == NULL) {
        fprintf(stderr, "ERROR: bench_print_tail got NULL result!\n");
        return;
    }

    name = result->name == NULL ? "sign" : result->name;
    printf(
        "%s (%zu runs) in cycles: p25: %llu, p50: %llu, p75: %llu, "
        "p90: %llu, p99: %llu, StQ1: %llu, StQ2: %llu, StQ3: %llu, "
        "avg: %llu\n",
        name, result->runs, (unsigned long long)result->p25,
        (unsigned long long)result->p50, (unsigned long long)result->p75,
        (unsigned long long)result->p90, (unsigned long long)result->p99,
        (unsigned long long)result->stq1, (unsigned long long)result->stq2,
        (unsigned long long)result->stq3, (unsigned long long)result->avg);
    printf(
        "%s stability: stddev: %llu, cv: %llu.%02llu%%, "
        "outlier_count(>p50*1.5): %zu/%zu, p90/p50: ",
        name, (unsigned long long)result->stddev,
        (unsigned long long)(result->cv_x100 / 100),
        (unsigned long long)(result->cv_x100 % 100), result->outlier_count,
        result->runs);
    bench_print_ratio(result->p90_p50_x100);
    printf(", p99/p50: ");
    bench_print_ratio(result->p99_p50_x100);
    printf("\n");
}
