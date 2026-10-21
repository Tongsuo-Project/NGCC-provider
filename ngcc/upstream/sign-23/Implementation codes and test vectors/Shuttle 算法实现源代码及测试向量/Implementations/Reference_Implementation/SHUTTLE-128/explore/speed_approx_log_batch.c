/* speed_approx_log_batch.c -- N-way batched ApproxLog: does multi-way batching
 * (x2/x3/x4/x8) amortize the constant-time table scan and hide the Horner
 * multiply latency, and does it shift the optimal segmentation g?
 *
 * For each (g, N) it measures median cycles PER ApproxLog output in a throughput
 * loop (N independent inputs per call).  N=1 is the scalar evaluator; N>=2 are
 * the generated log2_frac_g{g}_x{N} (shared table load, N interleaved Horner
 * chains).  A correctness gate first checks every x{N} is bit-identical to the
 * scalar evaluator.
 *
 * Build: gcc -O3 -march=native -std=gnu11 -I. speed_approx_log_batch.c \
 *        cpucycles.c -lquadmath -o speed_log_batch
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "approx_log_schemes.h"
#include "cpucycles.h"

#define NTESTS 2048
#define KBATCH 256          /* batch-calls per timed sample */
#define POOLBITS 12
#define POOL (1u << POOLBITS)

static uint32_t psel[POOL];
static uint64_t px[POOL];
static uint64_t t[NTESTS];
static volatile uint64_t sink = 0;

/* time one scalar evaluator (N=1) */
#define TIME1(FN) do {                                                       \
    for (size_t i=0;i<NTESTS;i++){ uint64_t p=i*KBATCH; uint64_t acc=0;       \
        uint64_t t0=cpucycles();                                             \
        for (int b=0;b<KBATCH;b++){ uint32_t q=(uint32_t)(p+b)&(POOL-1);     \
            acc += (uint64_t)FN(psel[q], px[q]); }                          \
        t[i]=cpucycles()-t0; sink ^= acc; }                                  \
    qsort(t,NTESTS,sizeof(uint64_t),cpucycles_cmp_uint64);                    \
    percall = (double)cpucycles_median(t,NTESTS)/KBATCH;                      \
} while(0)

/* time one N-way batched evaluator */
#define TIMEN(FN, N) do {                                                     \
    for (size_t i=0;i<NTESTS;i++){ uint64_t p=i*KBATCH*(N); uint64_t acc=0;   \
        uint64_t t0=cpucycles();                                             \
        for (int b=0;b<KBATCH;b++){                                          \
            uint32_t s[N]; uint64_t xv[N]; int64_t o[N];                     \
            for (int n=0;n<(N);n++){ uint32_t q=(uint32_t)(p+(uint64_t)b*(N)+n)&(POOL-1); s[n]=psel[q]; xv[n]=px[q]; } \
            FN(s,xv,o); for (int n=0;n<(N);n++) acc += (uint64_t)o[n]; }      \
        t[i]=cpucycles()-t0; sink ^= acc; }                                  \
    qsort(t,NTESTS,sizeof(uint64_t),cpucycles_cmp_uint64);                    \
    percall = (double)cpucycles_median(t,NTESTS)/(KBATCH*(N));               \
} while(0)

/* correctness: x{N} bit-identical to scalar */
#define GATE(SCAL, FNN, N) do {                                               \
    for (int it=0; it<4096; it++){                                           \
        uint32_t s[N]; uint64_t xv[N]; int64_t o[N];                         \
        for (int n=0;n<(N);n++){ uint32_t q=(uint32_t)(it*(N)+n)&(POOL-1); s[n]=psel[q]; xv[n]=px[q]; } \
        FNN(s,xv,o);                                                         \
        for (int n=0;n<(N);n++) if (o[n] != SCAL(s[n],xv[n])) { gate_fail++; } \
    } } while(0)

int main(void)
{
    uint64_t sd = 0xdeadbeefcafef00dULL;
    for (uint32_t i=0;i<POOL;i++){ sd^=sd<<13; sd^=sd>>7; sd^=sd<<17; psel[i]=(uint32_t)(sd&0x7f); px[i]=sd*0x2545F4914F6CDD1DULL; }

    int gate_fail = 0;
    GATE(log2_frac_g1, log2_frac_g1_x2, 2); GATE(log2_frac_g1, log2_frac_g1_x4, 4); GATE(log2_frac_g1, log2_frac_g1_x8, 8);
    GATE(log2_frac_g2, log2_frac_g2_x2, 2); GATE(log2_frac_g2, log2_frac_g2_x4, 4); GATE(log2_frac_g2, log2_frac_g2_x8, 8);
    GATE(log2_frac_g3, log2_frac_g3_x2, 2); GATE(log2_frac_g3, log2_frac_g3_x4, 4); GATE(log2_frac_g3, log2_frac_g3_x8, 8);
    GATE(log2_frac_g4, log2_frac_g4_x2, 2); GATE(log2_frac_g4, log2_frac_g4_x4, 4); GATE(log2_frac_g4, log2_frac_g4_x8, 8);
    GATE(log2_frac_g5, log2_frac_g5_x2, 2); GATE(log2_frac_g5, log2_frac_g5_x4, 4); GATE(log2_frac_g5, log2_frac_g5_x8, 8);
    printf("batched-vs-scalar bit-identity gate: %s (%d mismatches)\n\n", gate_fail?"FAIL":"PASS", gate_fail);

    printf("=== cycles per ApproxLog output, by (g, batch width N) ===\n");
    printf("  %-4s %8s %8s %8s %8s %8s\n", "g", "x1", "x2", "x3", "x4", "x8");
    double percall;
#define ROW(G, F1,F2,F3,F4,F8) do {                                          \
    double a1,a2,a3,a4,a8;                                                    \
    TIME1(F1); a1=percall; TIMEN(F2,2); a2=percall; TIMEN(F3,3); a3=percall;  \
    TIMEN(F4,4); a4=percall; TIMEN(F8,8); a8=percall;                         \
    printf("  g=%-2d %8.2f %8.2f %8.2f %8.2f %8.2f\n", G, a1,a2,a3,a4,a8);    \
} while(0)
    ROW(1, log2_frac_g1, log2_frac_g1_x2, log2_frac_g1_x3, log2_frac_g1_x4, log2_frac_g1_x8);
    ROW(2, log2_frac_g2, log2_frac_g2_x2, log2_frac_g2_x3, log2_frac_g2_x4, log2_frac_g2_x8);
    ROW(3, log2_frac_g3, log2_frac_g3_x2, log2_frac_g3_x3, log2_frac_g3_x4, log2_frac_g3_x8);
    ROW(4, log2_frac_g4, log2_frac_g4_x2, log2_frac_g4_x3, log2_frac_g4_x4, log2_frac_g4_x8);
    ROW(5, log2_frac_g5, log2_frac_g5_x2, log2_frac_g5_x3, log2_frac_g5_x4, log2_frac_g5_x8);

    printf("\nsink=%llu\n", (unsigned long long)sink);
    return gate_fail ? 1 : 0;
}
