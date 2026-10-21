/*
 * sampler_oracle.c -- C-oracle dumper for the SHUTTLE sampler byte-schedule
 * cross-check (P12, deliverable 3).
 *
 * Dumps, on FIXED inputs:
 *   - cdt_scan96 / sampler_sigma2 / noise_magnitude_batch outputs on a fixed
 *     random byte buffer (byte-exact-verifiable: pure functions of bytes).
 *   - sampler_u (a, frac_q62) for several calls off a 0x09||seed ctx, PLUS
 *     the raw 18-byte (rho_a||rho_b) stream each call consumes (dumped from a
 *     SECOND, identically-seeded ctx) so the Python ref can re-derive (a,
 *     frac) from the exact bytes and assert byte-exact interpretation.
 *
 * Build (per mode, DISABLE_NAMESPACE):
 *   gcc -O2 -std=c99 -I../../ref -I../../tools -I../../ref/ntt/<qset> \
 *       -DDISABLE_NAMESPACE=1 -DSHUTTLE_MODE=<m> sampler_oracle.c \
 *       ../../ref/{sampler,sampler_u,approx_log,approx_exp,reduce}.c \
 *       <xof srcs> -o sampler_oracle_<m>
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "params.h"
#include "sampler.h"
#include "sampler_u.h"
#include "xof.h"

static uint64_t XS;
static uint64_t xs(void)
{
    uint64_t x = XS;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    XS = x;
    return x;
}

int main(void)
{
    XS = 0x5A3C ^ (uint64_t)SHUTTLE_MODE;
    printf("MODE %d\n", (int)SHUTTLE_MODE);

    /* ---- cdt_scan96 family on a fixed random buffer ---- */
    {
        /* SIGMA2_RAND_BYTES = 384; NOISE_CDT_BYTES = 384. */
        uint8_t buf[512];
        for (int i = 0; i < 512; ++i)
            buf[i] = (uint8_t)xs();
        int32_t z[GAUSS_BATCH];
        sampler_sigma2(z, buf); /* uses the internal SHUTTLE_RCDT_Z table */
        printf("SIGMA2");
        for (int i = 0; i < GAUSS_BATCH; ++i)
            printf(" %d", (int)z[i]);
        printf("\n");
        /* dump the input buffer (first 384 B used) so Python re-runs
         * cdt_scan96 on the SAME bytes with the SAME RCDT_Z table. */
        printf("CDTBUF ");
        for (int i = 0; i < 384; ++i)
            printf("%02x", buf[i]);
        printf("\n");
    }

    /* ---- sampler_u: (a, frac) for NCALL calls + the raw 18-byte stream ---- */
    {
        const int NCALL = 16;
        uint8_t seed[SEEDBYTES];
        for (int i = 0; i < SEEDBYTES; ++i)
            seed[i] = (uint8_t)(0x40 + i);
        uint8_t absorb[1 + SEEDBYTES];
        absorb[0] = DS_IRS; /* 0x09 */
        memcpy(absorb + 1, seed, SEEDBYTES);

        /* ctx A: call sampler_u, dump (a, frac) */
        xof_ctx ca;
        xof256_init(&ca, absorb, sizeof absorb);
        for (int k = 0; k < NCALL; ++k) {
            sampler_u_res r = sampler_u(&ca);
            printf("SU %u %lld\n", (unsigned)r.a, (long long)r.frac_q62);
        }

        /* ctx B: identical seed, squeeze the raw 18-byte chunks each call
         * consumes (10 rho_a then 8 rho_b) so Python re-derives (a, frac). */
        xof_ctx cb;
        xof256_init(&cb, absorb, sizeof absorb);
        for (int k = 0; k < NCALL; ++k) {
            uint8_t rho_a[10], rho_b[8];
            xof256_squeeze(&cb, rho_a, 10);
            xof256_squeeze(&cb, rho_b, 8);
            printf("SURAW ");
            for (int i = 0; i < 10; ++i)
                printf("%02x", rho_a[i]);
            printf(" ");
            for (int i = 0; i < 8; ++i)
                printf("%02x", rho_b[i]);
            printf("\n");
        }
    }

    return 0;
}
