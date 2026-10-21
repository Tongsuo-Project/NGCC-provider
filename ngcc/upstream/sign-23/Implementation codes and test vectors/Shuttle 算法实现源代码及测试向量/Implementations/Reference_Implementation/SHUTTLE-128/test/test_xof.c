/*
 * test_xof.c -- build-verify for the unified XOF layer (scalar ref).
 *
 * NGCC_MODE (default):
 *   - xof128_init/squeeze and xof256_init/squeeze are byte-exact aliases
 * of a direct init_random_number + get_random_number(..., K*8) call,
 * proving the wrapper is a faithful alias INCLUDING the bytes->bits
 * conversion.
 *   - xof128 == xof256 (the 128/256 collapse).
 *
 * SHA3_MODE (-DSHA3_MODE):
 *   - xof128_* (SHAKE128) and xof256_* (SHAKE256) round-trip against the
 *     vendored fips202 one-shot shake128()/shake256() over a fixed input,
 *     proving init+absorb_once+squeeze chains correctly.
 *   - xof128 != xof256 (two genuinely distinct primitives).
 *
 * Build (NGCC): gcc -std=c99 -Wpedantic -Wall -Wextra -Werror -O2 -I
 * SHUTTLE/ref \
 *   SHUTTLE/ref/test/test_xof.c SHUTTLE/ref/auxfunc.c SHUTTLE/ref/drng.c \
 *   SHUTTLE/ref/symmetric.c -DSHUTTLE_MODE=128
 * Build (SHA3): add -DSHA3_MODE and replace drng.c/auxfunc.c with
 * fips202.c.
 *
 * Exit 0 + "test_xof: PASS" on success; nonzero + a FAIL line otherwise.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "xof.h"

#if defined(SHA3_MODE)
#    include "fips202.h"
#else
#    include "drng.h"
#endif

#define K                                                               \
    200 /* squeeze length in bytes; spans >1 SM3 (32B) / >1 SHAKE block \
         */

static int fail = 0;

static void report(const char *what, int ok)
{
    printf("  [%s] %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) {
        fail = 1;
    }
}

static void hexpfx(const char *label, const uint8_t *b, size_t n)
{
    size_t i;
    printf("    %s ", label);
    for (i = 0; i < n; i++) {
        printf("%02X", b[i]);
    }
    printf("\n");
}

int main(void)
{
    /* A fixed seed buffer (caller-pre-concatenated tag||seed||nonce
     * shape). */
    uint8_t seed[40];
    size_t i;
    for (i = 0; i < sizeof(seed); i++) {
        seed[i] = (uint8_t)(0x10 + i);
    }

    printf("=== test_xof (%s) ===\n",
#if defined(SHA3_MODE)
           "SHA3_MODE"
#else
           "NGCC_MODE"
#endif
    );

#if !defined(SHA3_MODE)
    /* ---------------- NGCC_MODE: byte-exact alias of the DRBG
     * ------------- */
    {
        xof_ctx ctx;
        DRNG_ctx ref;
        uint8_t w_xof256[K], w_ref[K];
        uint8_t w_xof128[K];

        /* xof256_squeeze(K bytes) == get_random_number(K*8 bits). */
        xof256_init(&ctx, seed, sizeof(seed));
        xof256_squeeze(&ctx, w_xof256, K);

        init_random_number(&ref, seed, (unsigned long long)sizeof(seed));
        get_random_number(&ref, w_ref, (unsigned long long)K * 8u);

        report("xof256 == init_random_number + get_random_number(K*8)",
               memcmp(w_xof256, w_ref, K) == 0);

        /* xof128 collapses to the same SM3 DRBG -> identical to xof256. */
        xof128_init(&ctx, seed, sizeof(seed));
        xof128_squeeze(&ctx, w_xof128, K);
        report("xof128 == xof256 (NGCC 128/256 collapse)",
               memcmp(w_xof128, w_ref, K) == 0);

        /* NGCC DRBG semantics (NOT a rate-buffer): each get_random_number
         * is a full SM3_DRNG_Generate that re-derives blocks from V and
         * then advances state ONCE per call.  So two separate squeezes do
         * NOT equal one concatenated squeeze (unlike a SHAKE rate cursor).
         * We assert exactly that, and that a re-seeded second squeeze
         * reproduces the first block (determinism).  Consumed-bytes
         * consequence: under NGCC_MODE a producer must draw a per-stream
         * chunk in ONE squeeze; the 16-stream flow's fixed,
         * whole-granularity refills already honor this. */
        {
            xof_ctx c2;
            uint8_t a[K], b[K];
            xof256_init(&c2, seed, sizeof(seed));
            xof256_squeeze(&c2, a, K); /* first Generate */
            xof256_squeeze(&c2, b,
                           K); /* second Generate: advanced state */
            report(
                "xof256 successive squeezes differ (per-call DRBG "
                "Generate)",
                memcmp(a, b, K) != 0);

            xof256_init(&c2, seed, sizeof(seed));
            xof256_squeeze(&c2, b,
                           K); /* re-seed reproduces the first block */
            report(
                "xof256 is deterministic under re-init (KAT determinism)",
                memcmp(a, b, K) == 0);
        }

        hexpfx("xof256[0..15] =", w_xof256, 16);
    }
#else
    /* ---------------- SHA3_MODE: round-trip vs one-shot SHAKE
     * ------------- */
    {
        xof_ctx ctx;
        uint8_t w_128[K], w_256[K];
        uint8_t oneshot_128[K], oneshot_256[K];

        xof128_init(&ctx, seed, sizeof(seed));
        xof128_squeeze(&ctx, w_128, K);
        shake128(oneshot_128, K, seed, sizeof(seed));
        report("xof128 (init+absorb_once+squeeze) == one-shot shake128",
               memcmp(w_128, oneshot_128, K) == 0);

        xof256_init(&ctx, seed, sizeof(seed));
        xof256_squeeze(&ctx, w_256, K);
        shake256(oneshot_256, K, seed, sizeof(seed));
        report("xof256 (init+absorb_once+squeeze) == one-shot shake256",
               memcmp(w_256, oneshot_256, K) == 0);

        report("xof128 != xof256 (distinct SHAKE128/256 primitives)",
               memcmp(w_128, w_256, K) != 0);

        /* Chaining for SHAKE256 too. */
        {
            xof_ctx c2;
            uint8_t a[K], b[K];
            xof256_init(&c2, seed, sizeof(seed));
            xof256_squeeze(&c2, a, K / 2);
            xof256_squeeze(&c2, a + K / 2, K - K / 2);
            xof256_init(&c2, seed, sizeof(seed));
            xof256_squeeze(&c2, b, K);
            report("xof256 split-squeeze chains == single squeeze",
                   memcmp(a, b, K) == 0);
        }

        /* Known SHAKE128 test vector: SHAKE128("", outlen) starts 7F 9C 2B
         * ... (NIST/standard empty-input vector).  Confirms the vendored
         * FIPS-202 is the standard one and not silently mis-padded. */
        {
            static const uint8_t kShake128Empty16[16] = {
                0x7F, 0x9C, 0x2B, 0xA4, 0xE8, 0x8F, 0x82, 0x7D,
                0x61, 0x60, 0x45, 0x50, 0x76, 0x05, 0x85, 0x3E};
            uint8_t e[16];
            shake128(e, 16, (const uint8_t *)"", 0);
            report("shake128(\"\") matches the known FIPS-202 vector",
                   memcmp(e, kShake128Empty16, 16) == 0);
            hexpfx("shake128(\"\")[0..15] =", e, 16);
        }

        hexpfx("xof128[0..15] =", w_128, 16);
        hexpfx("xof256[0..15] =", w_256, 16);
    }
#endif

    if (fail) {
        printf("test_xof: FAIL\n");
        return 1;
    }
    printf("test_xof: PASS\n");
    return 0;
}
