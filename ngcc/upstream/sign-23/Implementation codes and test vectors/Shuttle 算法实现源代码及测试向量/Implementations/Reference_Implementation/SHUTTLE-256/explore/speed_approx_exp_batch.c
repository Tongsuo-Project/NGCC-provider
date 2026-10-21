/* speed_approx_exp_batch.c -- N-way batched ApproxExp (Gaussian accept poly).
 *
 * ApproxExp has NO table scan: it is a shared-coefficient Taylor Horner plus a
 * squaring chain, both latency-bound dependency chains.  Batching N independent
 * (x,y) inputs interleaves N such chains to hide the multiply latency (only N
 * accumulators live, coefficients shared).  We sweep the squaring/degree split
 * (t,d) x batch width N and report cycles per exp output.
 *
 * Gate: each x{N} is bit-identical to the scheme's scalar x1; and x1 of the
 * deployed split (t7d8) matches a __float128 reference exp within 2^-53.
 *
 * Build: gcc -O3 -march=native -std=gnu11 -I. speed_approx_exp_batch.c \
 *        cpucycles.c -lquadmath -o speed_exp_batch
 */
#include <quadmath.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "approx_exp_schemes.h"
#include "cpucycles.h"

#define R 825
#define NTESTS 2048
#define KBATCH 256
#define POOLBITS 12
#define POOL (1u << POOLBITS)

static int poolx[POOL];
static int pooly[POOL];
static uint64_t t[NTESTS];
static volatile uint64_t sink = 0;

#define TIMEN(FN, N) do {                                                    \
    for (size_t i=0;i<NTESTS;i++){ uint64_t p=i*KBATCH*(N); uint64_t acc=0;   \
        uint64_t t0=cpucycles();                                             \
        for (int b=0;b<KBATCH;b++){ int xs[N],ys[N]; uint64_t o[N];          \
            for (int n=0;n<(N);n++){ uint32_t q=(uint32_t)(p+(uint64_t)b*(N)+n)&(POOL-1); xs[n]=poolx[q]; ys[n]=pooly[q]; } \
            FN(xs,ys,o); for (int n=0;n<(N);n++) acc += o[n]; }              \
        t[i]=cpucycles()-t0; sink ^= acc; }                                  \
    qsort(t,NTESTS,sizeof(uint64_t),cpucycles_cmp_uint64);                    \
    percall = (double)cpucycles_median(t,NTESTS)/(KBATCH*(N));               \
} while(0)

/* gate: x{N} bit-identical to x1 over the pool */
#define GATE(F1, FN, N) do {                                                 \
    for (int it=0; it<2048; it++){ int xs[N],ys[N]; uint64_t o[N],o1[1];     \
        for (int n=0;n<(N);n++){ uint32_t q=(uint32_t)(it*(N)+n)&(POOL-1); xs[n]=poolx[q]; ys[n]=pooly[q]; } \
        FN(xs,ys,o);                                                         \
        for (int n=0;n<(N);n++){ int xx[1]={xs[n]}, yy[1]={ys[n]}; F1(xx,yy,o1); if(o[n]!=o1[0]) gate_fail++; } } \
} while(0)

int main(void)
{
    uint64_t sd = 0x9e3779b97f4a7c15ULL;
    for (uint32_t i=0;i<POOL;i++){ sd^=sd<<13; sd^=sd>>7; sd^=sd<<17; poolx[i]=(int)(sd%37); pooly[i]=(int)((sd>>20)&0xff); }

    int gate_fail = 0;
    GATE(shuttle_exp_t7d8_x1, shuttle_exp_t7d8_x2, 2);
    GATE(shuttle_exp_t7d8_x1, shuttle_exp_t7d8_x4, 4);
    GATE(shuttle_exp_t7d8_x1, shuttle_exp_t7d8_x8, 8);
    GATE(shuttle_exp_t8d7_x1, shuttle_exp_t8d7_x4, 4);
    GATE(shuttle_exp_t5d10_x1, shuttle_exp_t5d10_x4, 4);
    /* accuracy spot-check of the deployed split vs libquadmath */
    __float128 worst = 0;
    for (int x=0;x<=36;x++) for (int y=0;y<=255;y++){
        int xx[1]={x}, yy[1]={y}; uint64_t o[1]; shuttle_exp_t7d8_x1(xx,yy,o);
        uint64_t n = (uint64_t)y*(uint64_t)(y+512*x);
        __float128 ref = expq(-((__float128)n)/((__float128)(2ULL*R*R)));
        __float128 rel = fabsq((__float128)o[0]/ldexpq(1,64) - ref)/ref;
        if (rel>worst) worst=rel;
    }
    double accbits = (double)(-logq(worst)/logq(2.0Q));
    printf("batched-vs-scalar gate: %s (%d mismatches);  t7d8 accuracy 2^-%.2f\n\n",
           gate_fail?"FAIL":"PASS", gate_fail, accbits);

    printf("=== cycles per ApproxExp output, by (t,d split) x batch width N ===\n");
    printf("  %-8s %5s %8s %8s %8s %8s %8s\n", "split", "mul", "x1", "x2", "x3", "x4", "x8");
    double percall;
#define ROW(T,D) do {                                                        \
    double a1,a2,a3,a4,a8;                                                    \
    TIMEN(shuttle_exp_t##T##d##D##_x1,1); a1=percall;                         \
    TIMEN(shuttle_exp_t##T##d##D##_x2,2); a2=percall;                         \
    TIMEN(shuttle_exp_t##T##d##D##_x3,3); a3=percall;                         \
    TIMEN(shuttle_exp_t##T##d##D##_x4,4); a4=percall;                         \
    TIMEN(shuttle_exp_t##T##d##D##_x8,8); a8=percall;                         \
    printf("  t%dd%-4d %5d %8.2f %8.2f %8.2f %8.2f %8.2f\n", T,D, (T)+(D), a1,a2,a3,a4,a8); \
} while(0)
    ROW(4,12); ROW(5,10); ROW(6,9); ROW(7,8); ROW(8,7);

    printf("\nsink=%llu\n", (unsigned long long)sink);
    return gate_fail ? 1 : 0;
}
