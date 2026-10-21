/*
 * test_pack.c -- build-verify for the (de)serialization layer.
 *
 * Covers:
 *   (a) poly_to_bytes / bytes_to_poly round-trip over random in-range
 * polys for every field width d in {1, d_s, d_e, d_h, d_b} (and the
 *       cross-set widths 1/5/6/7/13/14/15); output length == ceil(N*d/8);
 *       final-byte zero-pad bits are zero.
 *       integer_to_bytes / bytes_to_integer round-trip + overflow -> -1.
 *   (b) pack_pk / unpack_pk round-trip (random seedA + random b in [0,q),
 *       each coeff a multiple of alpha_b); packed length ==
 * 1264/1952/3648. unpack_pk_bn (fast trusted path) byte-identical to
 * unpack_pk's b. (c) pack_sk / unpack_sk round-trip (seedA/masterSeed/tr
 * verbatim; s/e' exact after shift); realized sk length == 2288/3680/7104;
 * reported. (d) pack_com / unpack_com round-trip for in-range commitments.
 *   (e) NEGATIVE (range-reject): a comY_h coeff set to a value in [H_h,
 * 2^d_h) makes unpack_com return -1 (range-CHECK-then-reject, NOT mod-H_h
 * wrap); plus pk_decode rejects an over-range b1 and sk_decode rejects an
 *       out-of-range secret coeff.
 *
 * No external RNG: a tiny self-contained xorshift keeps the test
 * standalone (it builds with only packing.c + poly.c + reduce.c).
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "packing.h"
#include "params.h"
#include "poly.h"

/* ---- tiny deterministic PRNG (xorshift128+) ---- */
static uint64_t rng_s0 = 0x0123456789abcdefULL;
static uint64_t rng_s1 = 0xfedcba9876543210ULL;
static uint64_t rng_next(void)
{
    uint64_t x = rng_s0, y = rng_s1;
    rng_s0 = y;
    x ^= x << 23;
    rng_s1 = x ^ y ^ (x >> 17) ^ (y >> 26);
    return rng_s1 + y;
}
static uint32_t rng_u32(void)
{
    return (uint32_t)(rng_next() >> 11);
}
static uint32_t rng_range(uint32_t bound) /* [0, bound) */
{
    return bound ? (rng_u32() % bound) : 0;
}

static int g_fails = 0;
static void report(const char *name, int fail)
{
    printf("[%-30s] %s\n", name, fail ? "FAIL" : "PASS");
    if (fail)
        g_fails++;
}

/* ====================================================================== *
 *  (a) byte primitives                                                   *
 * ======================================================================
 */
static void test_integer_bytes(void)
{
    int fail = 0;
    /* round-trip for 2- and 4-byte LE integers (the SHUTTLE call widths)
     */
    for (int t = 0; t < 100000; ++t) {
        uint8_t buf[8];
        unsigned len = (rng_u32() & 1) ? 2u : 4u;
        uint64_t x = rng_next() & (((uint64_t)1 << (8 * len)) - 1);
        if (integer_to_bytes(buf, x, len) != 0)
            fail = 1;
        if (bytes_to_integer(buf, len) != x)
            fail = 1;
    }
    /* overflow: x that does not fit in `len` bytes -> -1 */
    {
        uint8_t buf[8];
        if (integer_to_bytes(buf, 0x10000ULL, 2) != -1) /* 2^16 needs 3B */
            fail = 1;
        if (integer_to_bytes(buf, 0xFFFFULL, 2) != 0) /* fits exactly */
            fail = 1;
        if (integer_to_bytes(buf, 0x100000000ULL, 4) != -1) /* 2^32 */
            fail = 1;
    }
    report("integer_to/from_bytes (LE+ovf)", fail);
}

/* round-trip + length + zero-pad for one width d */
static int rt_width(unsigned d)
{
    int fail = 0;
    uint8_t buf[2 * 4 * 1024]; /* >= ceil(N*d/8) for any set/width */
    poly w, r;
    unsigned expect_bytes = (N * d + 7) / 8;
    unsigned valid_bits = N * d; /* total payload bits */

    for (int t = 0; t < 200; ++t) {
        for (unsigned k = 0; k < N; ++k)
            w.coeffs[k] = (int32_t)rng_range((uint32_t)1u << d);
        memset(buf, 0xAA, sizeof(buf));
        poly_to_bytes(buf, &w, d);
        bytes_to_poly(&r, buf, d);
        if (memcmp(w.coeffs, r.coeffs, sizeof(w.coeffs)) != 0)
            fail = 1;
        /* final-byte zero-pad bits (above valid_bits) must be zero */
        if (valid_bits % 8 != 0) {
            unsigned padbits = 8 - (valid_bits % 8);
            uint8_t last = buf[expect_bytes - 1];
            if ((last >> (8 - padbits)) != 0)
                fail = 1;
        }
        /* the byte AFTER the field must be untouched (0xAA), proving the
         * writer emitted exactly expect_bytes. */
        if (buf[expect_bytes] != 0xAA)
            fail = 1;
    }
    return fail;
}

static void test_poly_bytes(void)
{
    int fail = 0;
    unsigned widths[] = {1, 5, 6, 7, 13, 14, 15};
    for (unsigned i = 0; i < sizeof(widths) / sizeof(widths[0]); ++i)
        fail |= rt_width(widths[i]);
    /* also exercise the exact per-set field widths explicitly */
    fail |= rt_width(DB_BITS);
    fail |= rt_width(DS_BITS);
    fail |= rt_width(DE_BITS);
    fail |= rt_width(DH_BITS);
    report("poly_to/from_bytes round-trip", fail);
}

/* ====================================================================== *
 *  (b) public key                                                        *
 * ======================================================================
 */
static void rand_b(poly b[EM])
{
    /* random b in [0,q), each coeff a multiple of alpha_b (as RoundB
     * guarantees): pick b1 in [0, ceil(q/alpha_b)) then *alpha_b. */
    uint32_t ceilq = ((uint32_t)Q + ALPHA_B - 1) / ALPHA_B;
    for (unsigned i = 0; i < (unsigned)EM; ++i)
        for (unsigned k = 0; k < N; ++k)
            b[i].coeffs[k] = (int32_t)(rng_range(ceilq) * ALPHA_B);
}

static void test_pk(void)
{
    int fail = 0;
    /* size assertion (compile-time-equivalent, checked at runtime too) */
    if (CRYPTO_PUBLICKEYBYTES != PK_SIZE_EXPECT)
        fail = 1;

    for (int t = 0; t < 500; ++t) {
        uint8_t seedA[SEEDBYTES], seedA2[SEEDBYTES];
        uint8_t pk[CRYPTO_PUBLICKEYBYTES];
        poly b[EM], b2[EM], bbn[EM];
        for (unsigned i = 0; i < SEEDBYTES; ++i)
            seedA[i] = (uint8_t)rng_u32();
        rand_b(b);

        pack_pk(pk, seedA, b);
        if (unpack_pk(seedA2, b2, pk) != 0)
            fail = 1;
        if (memcmp(seedA, seedA2, SEEDBYTES) != 0)
            fail = 1;
        for (unsigned i = 0; i < (unsigned)EM; ++i)
            if (memcmp(b[i].coeffs, b2[i].coeffs, sizeof(b[i].coeffs)) !=
                0)
                fail = 1;

        /* unpack_pk_bn (fast trusted path) must be byte-identical to the
         * robust unpack_pk's reconstructed b (skips the body of pk). */
        unpack_pk_bn(bbn, pk + SEEDBYTES);
        for (unsigned i = 0; i < (unsigned)EM; ++i)
            if (memcmp(b2[i].coeffs, bbn[i].coeffs,
                       sizeof(b2[i].coeffs)) != 0)
                fail = 1;

        /* every reconstructed coeff in [0,q) and a multiple of alpha_b */
        for (unsigned i = 0; i < (unsigned)EM; ++i)
            for (unsigned k = 0; k < N; ++k)
                if (b2[i].coeffs[k] < 0 || b2[i].coeffs[k] >= Q ||
                    (b2[i].coeffs[k] % ALPHA_B) != 0)
                    fail = 1;
    }
    report("pack_pk/unpack_pk + unpack_pk_bn", fail);
}

/* ====================================================================== *
 *  (c) secret key                                                        *
 * ======================================================================
 */
static void test_sk(void)
{
    int fail = 0;
    if (CRYPTO_SECRETKEYBYTES != SK_SIZE_EXPECT)
        fail = 1;

    for (int t = 0; t < 300; ++t) {
        uint8_t seedA[SEEDBYTES], seedA2[SEEDBYTES];
        uint8_t mseed[CHALLENGESEEDBYTES], mseed2[CHALLENGESEEDBYTES];
        uint8_t tr[CHALLENGESEEDBYTES], tr2[CHALLENGESEEDBYTES];
        uint8_t sk[CRYPTO_SECRETKEYBYTES];
        poly b[EM], b2[EM], s[ELL], s2[ELL], ep[EM], ep2[EM];

        for (unsigned i = 0; i < SEEDBYTES; ++i)
            seedA[i] = (uint8_t)rng_u32();
        for (unsigned i = 0; i < CHALLENGESEEDBYTES; ++i) {
            mseed[i] = (uint8_t)rng_u32();
            tr[i] = (uint8_t)rng_u32();
        }
        rand_b(b);
        /* s in [-BS_ENC, BS_ENC], e' in [-BE_ENC, BE_ENC] */
        for (unsigned i = 0; i < (unsigned)ELL; ++i)
            for (unsigned k = 0; k < N; ++k)
                s[i].coeffs[k] =
                    (int32_t)rng_range(2 * BS_ENC + 1) - BS_ENC;
        for (unsigned i = 0; i < (unsigned)EM; ++i)
            for (unsigned k = 0; k < N; ++k)
                ep[i].coeffs[k] =
                    (int32_t)rng_range(2 * BE_ENC + 1) - BE_ENC;

        pack_sk(sk, seedA, b, mseed, tr, s, ep);
        if (unpack_sk(seedA2, b2, mseed2, tr2, s2, ep2, sk) != 0)
            fail = 1;

        if (memcmp(seedA, seedA2, SEEDBYTES) != 0)
            fail = 1;
        if (memcmp(mseed, mseed2, CHALLENGESEEDBYTES) != 0)
            fail = 1;
        if (memcmp(tr, tr2, CHALLENGESEEDBYTES) != 0)
            fail = 1;
        for (unsigned i = 0; i < (unsigned)EM; ++i)
            if (memcmp(b[i].coeffs, b2[i].coeffs, sizeof(b[i].coeffs)) !=
                0)
                fail = 1;
        for (unsigned i = 0; i < (unsigned)ELL; ++i)
            if (memcmp(s[i].coeffs, s2[i].coeffs, sizeof(s[i].coeffs)) !=
                0)
                fail = 1;
        for (unsigned i = 0; i < (unsigned)EM; ++i)
            if (memcmp(ep[i].coeffs, ep2[i].coeffs,
                       sizeof(ep[i].coeffs)) != 0)
                fail = 1;
    }
    report("pack_sk/unpack_sk round-trip", fail);
    printf(
        "    sk size (SHUTTLE-%d): realized=%d  expected=%d  %s\n",
        SHUTTLE_MODE, (int)CRYPTO_SECRETKEYBYTES, (int)SK_SIZE_EXPECT,
        (CRYPTO_SECRETKEYBYTES == SK_SIZE_EXPECT) ? "MATCH" : "MISMATCH");
}

/* ====================================================================== *
 *  (d) commitment round-trip                                             *
 * ======================================================================
 */
static void test_com(void)
{
    int fail = 0;
    for (int t = 0; t < 500; ++t) {
        uint8_t buf[ENCODECOM_BYTES];
        poly wh, w0, wh2, w02;
        for (unsigned k = 0; k < N; ++k) {
            wh.coeffs[k] = (int32_t)rng_range(HH);   /* [0, H_h) */
            w0.coeffs[k] = (int32_t)(rng_u32() & 1); /* {0,1}    */
        }
        pack_com(buf, &wh, &w0);
        if (unpack_com(&wh2, &w02, buf) != 0)
            fail = 1;
        if (memcmp(wh.coeffs, wh2.coeffs, sizeof(wh.coeffs)) != 0)
            fail = 1;
        if (memcmp(w0.coeffs, w02.coeffs, sizeof(w0.coeffs)) != 0)
            fail = 1;
    }
    report("pack_com/unpack_com round-trip", fail);
}

/* ====================================================================== *
 *  (e) negative tests (range rejects)                                   *
 * ======================================================================
 */
static void test_negative(void)
{
    int fail = 0;

    /* a comY_h coeff in [H_h, 2^d_h) must be REJECTED, not wrapped.
     */
    if ((1u << DH_BITS) <= (unsigned)HH) {
        /* would mean no reject gap exists -- structural error */
        fail = 1;
    } else {
        int any_reject_ok = 1;
        for (int t = 0; t < 200; ++t) {
            uint8_t buf[ENCODECOM_BYTES];
            poly wh, w0, wh2, w02;
            for (unsigned k = 0; k < N; ++k) {
                wh.coeffs[k] = (int32_t)rng_range(HH);
                w0.coeffs[k] = (int32_t)(rng_u32() & 1);
            }
            /* poison one coeff into the reject gap [H_h, 2^d_h) */
            unsigned kk = rng_range(N);
            uint32_t gap = (1u << DH_BITS) - (uint32_t)HH; /* >0 */
            int32_t poison = (int32_t)HH + (int32_t)rng_range(gap);
            wh.coeffs[kk] = poison;
            pack_com(buf, &wh, &w0);
            /* MUST reject (return -1).  If it instead wrapped mod H_h it
             * would return 0 with a different value -> injectivity break.
             */
            if (unpack_com(&wh2, &w02, buf) != -1)
                any_reject_ok = 0;
        }
        if (!any_reject_ok)
            fail = 1;
    }
    report("decode_com REJECTS [H_h,2^d_h)", fail);

    /* pk_decode rejects an over-range b1 field (>= ceil(q/alpha_b)).  Only
     * exists when 2^d_b > ceil(q/alpha_b), which holds for all three sets.
     */
    {
        int rej_ok = 1;
        uint32_t ceilq = ((uint32_t)Q + ALPHA_B - 1) / ALPHA_B;
        if ((1u << DB_BITS) <= ceilq) {
            rej_ok = 0; /* structural: no over-range b1 possible */
        } else {
            for (int t = 0; t < 200; ++t) {
                uint8_t seedA[SEEDBYTES], seedA2[SEEDBYTES];
                uint8_t pk[CRYPTO_PUBLICKEYBYTES];
                poly b[EM], b2[EM], b1[EM];
                for (unsigned i = 0; i < SEEDBYTES; ++i)
                    seedA[i] = (uint8_t)rng_u32();
                rand_b(b);
                /* build a pk whose b1 field carries an over-range value by
                 * packing a poisoned b1 directly. */
                for (unsigned i = 0; i < (unsigned)EM; ++i)
                    for (unsigned k = 0; k < N; ++k)
                        b1[i].coeffs[k] =
                            (int32_t)(b[i].coeffs[k] / ALPHA_B);
                unsigned ki = rng_range(N);
                uint32_t gap = (1u << DB_BITS) - ceilq;
                b1[0].coeffs[ki] = (int32_t)(ceilq + rng_range(gap));
                memcpy(pk, seedA, SEEDBYTES);
                for (unsigned i = 0; i < (unsigned)EM; ++i)
                    poly_to_bytes(
                        pk + SEEDBYTES + (size_t)i * POLYPK_PACKEDBYTES,
                        &b1[i], DB_BITS);
                if (unpack_pk(seedA2, b2, pk) != -1)
                    rej_ok = 0;
            }
        }
        report("pk_decode REJECTS b1 >= ceil(q/ab)", rej_ok ? 0 : 1);
        if (!rej_ok)
            fail = 1;
    }

    /* sk_decode rejects an out-of-range secret coeff (e.g. an s field that
     * unshifts to a value outside [-BS_ENC, BS_ENC]).  We poison the
     * packed s segment with a d_s-bit value that maps to v > BS_ENC. */
    {
        int rej_ok = 1;
        /* 2^d_s = 32, legal shifted range is [0, 2*BS_ENC]; values in
         * (2*BS_ENC, 2^d_s) unshift to v > BS_ENC -> must reject. */
        if ((1u << DS_BITS) <= (unsigned)(2 * BS_ENC + 1)) {
            rej_ok = 0; /* structural: no reject gap */
        } else {
            for (int t = 0; t < 200; ++t) {
                uint8_t seedA[SEEDBYTES], seedA2[SEEDBYTES];
                uint8_t mseed[CHALLENGESEEDBYTES],
                    mseed2[CHALLENGESEEDBYTES];
                uint8_t tr[CHALLENGESEEDBYTES], tr2[CHALLENGESEEDBYTES];
                uint8_t sk[CRYPTO_SECRETKEYBYTES];
                poly b[EM], b2[EM], s[ELL], s2[ELL], ep[EM], ep2[EM];
                for (unsigned i = 0; i < SEEDBYTES; ++i)
                    seedA[i] = (uint8_t)rng_u32();
                for (unsigned i = 0; i < CHALLENGESEEDBYTES; ++i) {
                    mseed[i] = (uint8_t)rng_u32();
                    tr[i] = (uint8_t)rng_u32();
                }
                rand_b(b);
                for (unsigned i = 0; i < (unsigned)ELL; ++i)
                    for (unsigned k = 0; k < N; ++k)
                        s[i].coeffs[k] =
                            (int32_t)rng_range(2 * BS_ENC + 1) - BS_ENC;
                for (unsigned i = 0; i < (unsigned)EM; ++i)
                    for (unsigned k = 0; k < N; ++k)
                        ep[i].coeffs[k] =
                            (int32_t)rng_range(2 * BE_ENC + 1) - BE_ENC;
                /* poison: shift one s coeff into the illegal high band so
                 * the encoded (s+BS_ENC) field lands in (2*BS_ENC, 2^d_s).
                 * pack_sk applies +BS_ENC, so set s = (2^d_s - 1) - BS_ENC
                 * which encodes to 2^d_s-1 and decodes back to v = 2^d_s-1
                 * - BS_ENC > BS_ENC. */
                s[0].coeffs[0] = (int32_t)((1 << DS_BITS) - 1) - BS_ENC;
                pack_sk(sk, seedA, b, mseed, tr, s, ep);
                if (unpack_sk(seedA2, b2, mseed2, tr2, s2, ep2, sk) != -1)
                    rej_ok = 0;
            }
        }
        report("sk_decode REJECTS out-of-range s", rej_ok ? 0 : 1);
        if (!rej_ok)
            fail = 1;
    }

    (void)fail;
}

int main(void)
{
    printf("=== test_pack (SHUTTLE-%d) ===\n", SHUTTLE_MODE);
    printf(
        "  N=%d q=%d ell=%d m=%d  d_b=%d d_s=%d d_e=%d d_h=%d  "
        "alpha_b=%d H_h=%d\n",
        N, Q, ELL, EM, DB_BITS, DS_BITS, DE_BITS, DH_BITS, ALPHA_B, HH);
    printf("  pk=%d  sk=%d  ENCODECOM_BYTES=%d\n",
           (int)CRYPTO_PUBLICKEYBYTES, (int)CRYPTO_SECRETKEYBYTES,
           (int)ENCODECOM_BYTES);

    test_integer_bytes();
    test_poly_bytes();
    test_pk();
    test_sk();
    test_com();
    test_negative();

    printf("\nSUMMARY test_pack (SHUTTLE-%d) fails=%d\n", SHUTTLE_MODE,
           g_fails);
    return g_fails ? 1 : 0;
}
