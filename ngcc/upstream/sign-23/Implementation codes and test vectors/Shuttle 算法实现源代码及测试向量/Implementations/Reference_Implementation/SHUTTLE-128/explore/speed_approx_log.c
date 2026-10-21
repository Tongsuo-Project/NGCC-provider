/* speed_approx_log.c -- correctness gate + microbenchmark for the candidate
 * ApproxLog schemes (deployed single-segment baseline vs segmented g=1..7).
 *
 * Correctness gate (return code): for every scheme, the integer Q62 output of
 * the EXACT mantissa->inputs mapping must stay within 2^-57 absolute of
 * log2(b) over a dense mantissa grid (__float128 reference).
 *
 * Timing (informational, cycles/call):
 *   - latency-bound : a serial dependency chain (each call's input is mixed
 *                     from the previous output) -- this is the cost on the
 *                     rejection-loop critical path, where ApproxLog is called
 *                     once per transition and nothing hides its latency;
 *   - throughput    : independent calls over an L1-resident input pool.
 * The decision metric is latency-bound p50.
 *
 * Build (native, stable cycles -- turbo/HT off):
 *   gcc -O3 -march=native -std=gnu11 -I. \
 *       speed_approx_log.c cpucycles.c -lquadmath -o speed_approx_log
 */
#include <quadmath.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "approx_log_schemes.h"
#include "cpucycles.h"

#define PGRID 22            /* mantissa grid: b = 1 + m/2^PGRID, m in [0,2^22) */
#define NTESTS 2048
#define KBATCH 512          /* work units per timed sample (amortize rdtsc) */
#define POOLBITS 12
#define POOL (1u << POOLBITS)
#define Q62 ldexpq(1.0Q, 62)

/* ---- exact mantissa -> (sel, x_q64) for a segmented scheme with g bits ---- */
static inline void map_seg(uint32_t m, int g, uint32_t *sel, uint64_t *x)
{
    int rb = PGRID - g;                 /* reduced-argument bits */
    *sel = m >> rb;
    *x = (uint64_t)(m & ((1u << rb) - 1)) << (64 - rb);
}

/* uniform wrappers so the timing macro is identical across schemes */
static inline int64_t call_baseline(uint32_t sel, uint64_t x){ (void)sel; return log2_frac_baseline(x >> 2); }
static inline int64_t call_g1(uint32_t s, uint64_t x){ return log2_frac_g1(s, x); }
static inline int64_t call_g2(uint32_t s, uint64_t x){ return log2_frac_g2(s, x); }
static inline int64_t call_g3(uint32_t s, uint64_t x){ return log2_frac_g3(s, x); }
static inline int64_t call_g4(uint32_t s, uint64_t x){ return log2_frac_g4(s, x); }
static inline int64_t call_g5(uint32_t s, uint64_t x){ return log2_frac_g5(s, x); }
static inline int64_t call_g6(uint32_t s, uint64_t x){ return log2_frac_g6(s, x); }
static inline int64_t call_g7(uint32_t s, uint64_t x){ return log2_frac_g7(s, x); }

static __float128 ref_log2(uint32_t m){ return logq(1.0Q + (__float128)m/ldexpq(1.0Q,PGRID))/logq(2.0Q); }

/* ---------- correctness gate: dense mantissa scan, returns worst bits ---------- */
static double gate_bits(int g)
{
    __float128 worst = 0;
    uint32_t M = 1u << PGRID;
    for (uint32_t m = 0; m < M; m++) {
        int64_t out;
        if (g == 0) {                       /* baseline */
            uint64_t u = (uint64_t)((__uint128_t)(m & ((1u<<PGRID)-1)) << (62 - PGRID));
            out = log2_frac_baseline(u);
        } else {
            uint32_t sel; uint64_t x; map_seg(m, g, &sel, &x);
            switch (g) {
                case 1: out = log2_frac_g1(sel,x); break;
                case 2: out = log2_frac_g2(sel,x); break;
                case 3: out = log2_frac_g3(sel,x); break;
                case 4: out = log2_frac_g4(sel,x); break;
                case 5: out = log2_frac_g5(sel,x); break;
                case 6: out = log2_frac_g6(sel,x); break;
                case 7: out = log2_frac_g7(sel,x); break;
                default: out = 0;
            }
        }
        __float128 e = fabsq((__float128)out/Q62 - ref_log2(m));
        if (e > worst) worst = e;
    }
    return (double)(-logq(worst)/logq(2.0Q));
}

/* timing helpers */
static uint64_t timed_median(uint64_t *t, size_t n){ return cpucycles_median(t, n); }

#define BENCH_LAT(IDX, CALL, G) do {                                        \
    uint64_t x = 0x123456789abcdef0ULL ^ (uint64_t)(IDX*0x9e3779b97f4a7c15ULL); \
    uint32_t sel = (uint32_t)(x >> (64 - (G))) & ((1u<<(G))-1);             \
    for (size_t i=0;i<NTESTS;i++){                                          \
        uint64_t t0=cpucycles();                                           \
        for (int b=0;b<KBATCH;b++){ int64_t r=CALL(sel,x); x ^= (uint64_t)r*0x2545F4914F6CDD1DULL; sel=(uint32_t)(x>>(64-(G)))&((1u<<(G))-1);} \
        t[i]=cpucycles()-t0; sink ^= (uint64_t)x;                           \
    }                                                                       \
    qsort(t,NTESTS,sizeof(uint64_t),cpucycles_cmp_uint64);                  \
    lat = (double)cpucycles_median(t,NTESTS)/KBATCH;                        \
} while(0)

#define BENCH_TPUT(CALL, G) do {                                            \
    for (size_t i=0;i<NTESTS;i++){                                          \
        uint64_t acc=0; uint64_t t0=cpucycles();                           \
        for (int b=0;b<KBATCH;b++){ acc += (uint64_t)CALL(psel[(i*KBATCH+b)&(POOL-1)], px[(i*KBATCH+b)&(POOL-1)]); } \
        t[i]=cpucycles()-t0; sink ^= acc;                                  \
    }                                                                       \
    qsort(t,NTESTS,sizeof(uint64_t),cpucycles_cmp_uint64);                  \
    tput = (double)cpucycles_median(t,NTESTS)/KBATCH;                       \
} while(0)

int main(void)
{
    static uint64_t t[NTESTS];
    static uint32_t psel[POOL]; static uint64_t px[POOL];
    volatile uint64_t sink = 0;
    int fail = 0;

    /* input pool from a deterministic PRNG */
    uint64_t s = 0xdeadbeefcafef00dULL;
    for (uint32_t i=0;i<POOL;i++){ s ^= s<<13; s ^= s>>7; s ^= s<<17; psel[i]=(uint32_t)(s & 0x7f); px[i]=s*0x2545F4914F6CDD1DULL; }

    printf("=== correctness gate (b=1+m/2^%d, %u points), threshold 2^-57 ===\n", PGRID, 1u<<PGRID);
    for (int sidx=0; sidx<AL_NUM_SCHEMES; sidx++){
        int g = AL_SCHEMES[sidx].g;
        double bits = gate_bits(g);
        int ok = bits >= 57.0;
        printf("  %-9s g=%d deg=%2d : abs err = 2^-%.2f  %s\n",
               AL_SCHEMES[sidx].name, g, AL_SCHEMES[sidx].degree, bits, ok?"PASS":"FAIL");
        if (!ok) fail = 1;
    }

    printf("\n=== timing (median cycles per ApproxLog call) ===\n");
    printf("  %-9s %6s %6s %8s %8s\n", "scheme", "deg", "scan", "latency", "tput");
    double lat, tput;
    /* baseline */
    BENCH_LAT(0, call_baseline, 4); BENCH_TPUT(call_baseline, 4);
    printf("  %-9s %6d %6d %8.2f %8.2f\n", "baseline", AL_SCHEMES[0].degree, 0, lat, tput);
    BENCH_LAT(1, call_g1, 1); BENCH_TPUT(call_g1, 1);
    printf("  %-9s %6d %6d %8.2f %8.2f\n", "g1", AL_SCHEMES[1].degree, AL_SCHEMES[1].scan_ops, lat, tput);
    BENCH_LAT(2, call_g2, 2); BENCH_TPUT(call_g2, 2);
    printf("  %-9s %6d %6d %8.2f %8.2f\n", "g2", AL_SCHEMES[2].degree, AL_SCHEMES[2].scan_ops, lat, tput);
    BENCH_LAT(3, call_g3, 3); BENCH_TPUT(call_g3, 3);
    printf("  %-9s %6d %6d %8.2f %8.2f\n", "g3", AL_SCHEMES[3].degree, AL_SCHEMES[3].scan_ops, lat, tput);
    BENCH_LAT(4, call_g4, 4); BENCH_TPUT(call_g4, 4);
    printf("  %-9s %6d %6d %8.2f %8.2f\n", "g4", AL_SCHEMES[4].degree, AL_SCHEMES[4].scan_ops, lat, tput);
    BENCH_LAT(5, call_g5, 5); BENCH_TPUT(call_g5, 5);
    printf("  %-9s %6d %6d %8.2f %8.2f\n", "g5", AL_SCHEMES[5].degree, AL_SCHEMES[5].scan_ops, lat, tput);
    BENCH_LAT(6, call_g6, 6); BENCH_TPUT(call_g6, 6);
    printf("  %-9s %6d %6d %8.2f %8.2f\n", "g6", AL_SCHEMES[6].degree, AL_SCHEMES[6].scan_ops, lat, tput);
    BENCH_LAT(7, call_g7, 7); BENCH_TPUT(call_g7, 7);
    printf("  %-9s %6d %6d %8.2f %8.2f\n", "g7", AL_SCHEMES[7].degree, AL_SCHEMES[7].scan_ops, lat, tput);

    printf("\nsink=%llu  (gate %s)\n", (unsigned long long)sink, fail?"FAILED":"passed");
    return fail;
}
