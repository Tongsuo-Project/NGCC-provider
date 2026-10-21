#ifndef CPUCYCLES_H
#define CPUCYCLES_H

#include <stddef.h>
#include <stdint.h>

#define BENCH_WARMUP 100
#ifndef BENCH_SIGN_RUNS
#    define BENCH_SIGN_RUNS 10000
#endif

#ifdef USE_RDPMC /* Needs echo 2 > /sys/devices/cpu/rdpmc */

static inline uint64_t cpucycles(void)
{
    const uint32_t ecx = (1U << 30) + 1;
    uint64_t result;

    __asm__ volatile("rdpmc; shlq $32,%%rdx; orq %%rdx,%%rax"
                     : "=a"(result)
                     : "c"(ecx)
                     : "rdx");

    return result;
}

#else

static inline uint64_t cpucycles(void)
{
    uint64_t result;

    __asm__ volatile("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax"
                     : "=a"(result)
                     :
                     : "%rdx");

    return result;
}

#endif

typedef int (*bench_tail_sign_fn)(void *ctx);

typedef struct tail_sign_result {
    const char *name;
    size_t runs;
    uint64_t p25;
    uint64_t p50;
    uint64_t p75;
    uint64_t p90;
    uint64_t p99;
    uint64_t stq1;
    uint64_t stq2;
    uint64_t stq3;
    uint64_t avg;
    uint64_t stddev;
    uint64_t cv_x100;
    size_t outlier_count;
    uint64_t p90_p50_x100;
    uint64_t p99_p50_x100;
} tail_sign_result;

uint64_t cpucycles_overhead(void);
int cpucycles_cmp_uint64(const void *a, const void *b);
uint64_t cpucycles_median(uint64_t *t, size_t tlen);
int bench_tail_sign(const char *name, tail_sign_result *result,
                    bench_tail_sign_fn sign_once, void *ctx);
void bench_print_tail(const tail_sign_result *result);

#endif
