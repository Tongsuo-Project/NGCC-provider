/*
 * prof.c -- counters + report for the macro-gated SHUTTLE profiler (see
 * prof.h).  Compiled (with real bodies) only when PROF_TIME or PROF_RAND is
 * defined; otherwise this TU is empty and contributes nothing.
 */
#include "prof.h"

#if defined(PROF_TIME) || defined(PROF_RAND)
#    include <stdio.h>
#    include <string.h>

uint64_t prof_cyc[PT_NBUCKETS];
uint64_t prof_cnt[PT_NBUCKETS];
uint64_t prof_sq[PC_NCTX];
uint64_t prof_use[PU_NCONS];
int prof_ctx = PC_OTHER;

void prof_reset(void)
{
    memset(prof_cyc, 0, sizeof prof_cyc);
    memset(prof_cnt, 0, sizeof prof_cnt);
    memset(prof_sq, 0, sizeof prof_sq);
    memset(prof_use, 0, sizeof prof_use);
    prof_ctx = PC_OTHER;
}

#    ifdef PROF_TIME
/* report row: bucket, label, parent bucket (-1 => top-level). */
struct row {
    int b;
    const char *name;
    int parent;
};
static const struct row SIGN_ROWS[] = {
    {PT_SETUP, "setup (skDecode/tr/mu)", -1},
    {PT_EXPAND_A, "expand_A", -1},
    {PT_SAMPLE_Y, "sample_y (Gaussian)", -1},
    {PT_G_SHAKE, "  xof squeeze (SampleY)", PT_SAMPLE_Y},
    {PT_G_BASESAMP, "  BaseSampler (sigma_s)", PT_SAMPLE_Y},
    {PT_G_APPROXEXP, "  ApproxExp (t7d8 Q64)", PT_SAMPLE_Y},
    {PT_G_FINAL, "  fold/sign/finalize", PT_SAMPLE_Y},
    {PT_COMMIT, "commitment (NTT mat-mul)", -1},
    {PT_NTT_FWD, "  ntt forward", PT_COMMIT},
    {PT_NTT_PW, "  ntt pointwise/basemul", PT_COMMIT},
    {PT_NTT_INV, "  ntt inverse", PT_COMMIT},
    {PT_HIGHBITS, "highbits/lsb (CompressY)", -1},
    {PT_CHALLENGE, "challenge (hash+sample)", -1},
    {PT_IRS, "irs (rejection sampler)", -1},
    {PT_SAMPLERU, "  SamplerU (CLZ+mantissa)", PT_IRS},
    {PT_APPROXLOG, "  ApproxLog (g2 d13 Q62)", PT_IRS},
    {PT_NORMCHECK, "normcheck (B_v gate)", -1},
    {PT_MAKEHINT, "makehint", -1},
    {PT_RANS, "rANS encode", -1},
    {PT_PACK, "pack (sigEncode layout)", -1},
};
static const struct row KG_ROWS[] = {
    {PT_KG_EXPAND_A, "expand_A", -1},
    {PT_KG_NOISE, "sample_noise (s,e)", -1},
    {PT_KG_BPRODUCT, "b-product (NTT mat-mul)", -1},
    {PT_KG_ROUNDB, "roundB (b, e')", -1},
    {PT_KG_STRETCH, "stretchS", -1},
    {PT_KG_NORM, "norm window", -1},
    {PT_KG_PACK, "pack pk/sk + tr", -1},
};
static const struct row VF_ROWS[] = {
    {PT_VF_UNPACK, "unpack pk/sig", -1},
    {PT_VF_SETUP, "setup (tr/mu)", -1},
    {PT_VF_A, "expand_A + b-hat NTT", -1},
    {PT_VF_MATMUL, "commitment (1+ELL cols)", -1},
    {PT_VF_HINT, "usehint/lsb", -1},
    {PT_VF_CHALLENGE, "challenge (hash+sample)", -1},
    {PT_VF_NORM, "normcheck (z reconstruct)", -1},
};
struct section {
    const char *prim;
    const struct row *rows;
    int nrows;
};
static const struct section SECTIONS[] = {
    {"keygen", KG_ROWS, (int)(sizeof KG_ROWS / sizeof KG_ROWS[0])},
    {"sign", SIGN_ROWS, (int)(sizeof SIGN_ROWS / sizeof SIGN_ROWS[0])},
    {"verify", VF_ROWS, (int)(sizeof VF_ROWS / sizeof VF_ROWS[0])},
};
#        define NSECT ((int)(sizeof SECTIONS / sizeof SECTIONS[0]))
#    endif

void prof_report(const char *title, uint64_t nsig)
{
    if (nsig == 0)
        nsig = 1;
    printf(
        "\n=== SHUTTLE profile: %s  (per-op, averaged over %llu) ===\n",
        title, (unsigned long long)nsig);

#    ifdef PROF_TIME
    /* Print whichever section has data: a per-primitive profiled run
     * (keygen | sign | verify) leaves only its own buckets non-zero. */
    {
        int s;
        for (s = 0; s < NSECT; s++) {
            const struct row *rows = SECTIONS[s].rows;
            int nrows = SECTIONS[s].nrows;
            uint64_t total = 0;
            int i;
            for (i = 0; i < nrows; i++)
                if (rows[i].parent < 0)
                    total += prof_cyc[rows[i].b];
            if (total == 0)
                continue;
            printf("-- %s TIMING (cycles/op, %% of total) --\n",
                   SECTIONS[s].prim);
            for (i = 0; i < nrows; i++) {
                uint64_t c = prof_cyc[rows[i].b];
                double pct;
                if (rows[i].parent < 0) {
                    pct = 100.0 * (double)c / (double)total;
                    printf("  %-28s %12llu  %6.2f%%   (calls %llu)\n",
                           rows[i].name, (unsigned long long)(c / nsig),
                           pct, (unsigned long long)prof_cnt[rows[i].b]);
                } else {
                    uint64_t pc = prof_cyc[rows[i].parent];
                    pct = pc ? 100.0 * (double)c / (double)pc : 0.0;
                    printf("  %-28s %12llu  %6.2f%% of parent\n",
                           rows[i].name, (unsigned long long)(c / nsig),
                           pct);
                }
            }
            printf("  %-28s %12llu  100.00%%\n", "TOTAL (sum top-level)",
                   (unsigned long long)(total / nsig));
        }
    }
#    endif

#    ifdef PROF_RAND
    {
        static const char *cn[PC_NCTX] = {
            "setup",    "A (ExpandA)", "gauss (SampleY)", "challenge",
            "irs",      "noise",       "other"};
        static const char *un[PU_NCONS] = {
            "signs",         "sigma_s(RCDT)",  "y", "rej_tail",
            "noise", "SamplerU(exp+mant)"};
        uint64_t tsq = 0;
        uint64_t tuse = 0;
        int i;
        for (i = 0; i < PC_NCTX; i++)
            tsq += prof_sq[i];
        if (tsq == 0)
            tsq = 1;
        printf(
            "-- RANDOMNESS: XOF squeezed (bytes/sig, %% of total) --\n");
        for (i = 0; i < PC_NCTX; i++)
            if (prof_sq[i])
                printf("  %-28s %12llu  %6.2f%%\n", cn[i],
                       (unsigned long long)(prof_sq[i] / nsig),
                       100.0 * (double)prof_sq[i] / (double)tsq);
        printf("  %-28s %12llu\n", "TOTAL squeezed",
               (unsigned long long)(tsq / nsig));
        for (i = 0; i < PU_NCONS; i++)
            tuse += prof_use[i];
        if (tuse) {
            printf(
                "-- RANDOMNESS: sampler consumption (bytes/sig) --\n");
            for (i = 0; i < PU_NCONS; i++)
                if (prof_use[i])
                    printf("  %-28s %12llu  %6.2f%%\n", un[i],
                           (unsigned long long)(prof_use[i] / nsig),
                           100.0 * (double)prof_use[i] / (double)tuse);
            printf("  %-28s %12llu\n", "TOTAL consumed",
                   (unsigned long long)(tuse / nsig));
        }
    }
#    endif
    printf("\n");
}
#endif /* PROF_TIME || PROF_RAND */
