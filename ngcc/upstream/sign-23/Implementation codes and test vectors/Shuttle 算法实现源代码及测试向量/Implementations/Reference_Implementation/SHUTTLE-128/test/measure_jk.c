/* measure_jk.c -- off-path harness: empirical per-lane XOF block-count (J)
 * distribution for ExpandS / SampleY, used to pin the AVX N-way bulk
 * preset K* (= smallest K with the per-lane overflow P(J>K) <= 1/16).
 * Built with -DMEASURE_JK and -DMEASURE_SAMPLER={0:expand_s,1:sample_y} by
 * tools/gen_bulk_k.sh, which post-processes the histogram into the
 * BULK_K_* macros (reproducible-constants principle).
 *
 * The number of blocks a lane consumes (J) is governed by the PUBLIC lane
 * width plus the sampler's rejection rate, which is identical in
 * distribution across the XOF backends (NGCC vs SHA3 produce different
 * bytes but the same accept statistics), so one NGCC measurement pins K
 * for both xof modes.  Seeds are varied by a cheap LCG (only the spread of
 * J matters, not the exact bytes). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "params.h"
#include "poly.h"
#include "polyvec.h"

extern unsigned long shuttle_jk_hist[8192];
extern unsigned long shuttle_jk_blocks;

static uint32_t s_lcg = 0x12345678u;
static uint8_t nextb(void)
{
    s_lcg = s_lcg * 1664525u + 1013904223u;
    return (uint8_t)(s_lcg >> 24);
}

int main(int argc, char **argv)
{
    long M = (argc > 1) ? atol(argv[1]) : 8000;
    long i;
    size_t k;
    unsigned long total = 0, jmax = 0;
#if MEASURE_SAMPLER == 0
    static poly s1s2[ELL + EM];
    uint8_t seed[CHALLENGESEEDBYTES];
    const char *name = "expand_s";
    for (i = 0; i < M; i++) {
        for (k = 0; k < sizeof seed; k++)
            seed[k] = nextb();
        expand_s(s1s2, seed);
    }
#else
    static poly y[KVEC];
    uint8_t seed[SEEDBYTES];
    const char *name = "sample_y";
    for (i = 0; i < M; i++) {
        for (k = 0; k < sizeof seed; k++)
            seed[k] = nextb();
        sample_y(y, seed);
    }
#endif
    for (k = 0; k < 8192; k++) {
        total += shuttle_jk_hist[k];
        if (shuttle_jk_hist[k])
            jmax = k;
    }
    printf(
        "# sampler=%s mode=%d calls=%ld lanes=%d samples=%lu jmax=%lu\n",
        name, SHUTTLE_MODE, M, XOF_STREAMS, total, jmax);
    for (k = 0; k <= jmax; k++)
        printf("%zu %lu\n", k, shuttle_jk_hist[k]);
    return 0;
}
