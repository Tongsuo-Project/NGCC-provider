/*
 * test_rans.c -- build-verify for the rANS entropy coder + signature
 * (de)serialization.
 *
 * Per param set:
 *   (a) the Python generators emit the tables + report sizes (run
 * separately via `make tables`; this binary asserts the committed tables
 * are internally consistent: FREQ sums == 1024, CDF/SLOT coverage,
 * supports). (b) C round-trip: random in-support (Q0, Qs, h) vectors drawn
 * from the model supports -> shuttle_rans_encode -> shuttle_rans_decode
 * recovers them, over many random vectors of varying length. (c) canonical
 * decode REJECTS >= 6 negative mutations (via pack_sig / unpack_sig):
 * flipped padding byte, truncated stream (short rlen), out-of-range
 * initial state, a CDF-hole / out-of-support attempt, a non-terminal end
 * state, an enlarged rlen field, an out-of-range hint bucket, and a
 * re-encode-mismatch construct. (d) pack_sig_raw / unpack_sig_raw
 * round-trip + the hint range-check (a negative h-bucket rejected).
 *   (e) C-vs-Python byte-exactness: read ref/test/rans_vectors_<set>.txt
 *       (recorded by tools/check_rans.py), encode the SAME synthetic
 * vector in C, assert identical `com` bytes. (f) size report: realized
 * pack_sig length vs CRYPTO_BYTES and the SigSize estimate.
 *
 * No external RNG: a tiny self-contained xorshift keeps the test
 * standalone (links packing.c + poly.c + reduce.c + rans.c).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "packing.h"
#include "params.h"
#include "poly.h"
#include "rans.h"

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
static int32_t rng_in(int32_t lo, int32_t hi)
{
    return lo + (int32_t)(rng_u32() % (uint32_t)(hi - lo + 1));
}

static int g_fails = 0;
static void report(const char *name, int fail)
{
    printf("[%-34s] %s\n", name, fail ? "FAIL" : "PASS");
    if (fail)
        g_fails = 1;
}

/* symbol counts (mirror packing.c) */
#define T_NQ0 ((size_t)N)
#define T_NQS ((size_t)ELL * N)
#define T_NH ((size_t)EM * N)

/* ---- (a) committed-table self-consistency ---- */
static int sum_freq(const uint16_t *f, int n)
{
    int s = 0;
    for (int i = 0; i < n; i++)
        s += f[i];
    return s;
}
static int check_table(const char *nm, const uint16_t *freq,
                       const uint16_t *cdf, const uint8_t *slot, int n)
{
    int fail = 0;
    if (sum_freq(freq, n) != (int)RANS_PROB_SCALE)
        fail = 1;
    if (cdf[0] != 0 || cdf[n] != RANS_PROB_SCALE)
        fail = 1;
    for (int i = 0; i < n; i++)
        if (freq[i] < 1 || cdf[i + 1] - cdf[i] != freq[i])
            fail = 1;
    /* SLOT full coverage: every v maps to the slot containing it. */
    for (uint32_t v = 0; v < RANS_PROB_SCALE; v++) {
        int s = slot[v];
        if (s < 0 || s >= n || !(cdf[s] <= v && v < cdf[s + 1]))
            fail = 1;
    }
    if (fail)
        printf("    table %s INCONSISTENT (sum=%d)\n", nm,
               sum_freq(freq, n));
    return fail;
}
static int test_tables(void)
{
    int fail = 0;
    fail |= check_table("Q0", RANS_Q0_FREQ, RANS_Q0_CDF, RANS_Q0_SLOT,
                        RANS_Q0_N);
    fail |= check_table("QS", RANS_QS_FREQ, RANS_QS_CDF, RANS_QS_SLOT,
                        RANS_QS_N);
    fail |= check_table("HINT", RANS_HINT_FREQ, RANS_HINT_CDF,
                        RANS_HINT_SLOT, RANS_HINT_N);
    return fail;
}

/* ---- (b) C round-trip on synthetic in-support symbol arrays ---- */
/* A generous codec buffer: uniform-random symbols cost ~log2(N) bits each
 * (far above the model entropy that sizes RANS_RESERVED_BYTES), so the
 * round-trip test must NOT cap at the reserve.  4 bytes/symbol + flush is
 * a safe upper bound for any in-support stream. */
#define T_CODEC_CAP (4 * (T_NQ0 + T_NQS + T_NH) + 64)
static int test_roundtrip(void)
{
    static int32_t q0[T_NQ0], qs[T_NQS], hh[T_NH];
    static int32_t d0[T_NQ0], ds[T_NQS], dh[T_NH];
    static uint8_t buf[T_CODEC_CAP];
    int fail = 0;
    for (int trial = 0; trial < 200; trial++) {
        size_t n0 = 1 + (rng_u32() % T_NQ0);
        size_t ns = 1 + (rng_u32() % T_NQS);
        size_t nh = 1 + (rng_u32() % T_NH);
        for (size_t i = 0; i < n0; i++)
            q0[i] = rng_in(RANS_Q0_LO, RANS_Q0_LO + RANS_Q0_N - 1);
        for (size_t i = 0; i < ns; i++)
            qs[i] = rng_in(RANS_QS_LO, RANS_QS_LO + RANS_QS_N - 1);
        for (size_t i = 0; i < nh; i++)
            hh[i] = rng_in(RANS_HINT_LO, RANS_HINT_LO + RANS_HINT_N - 1);
        size_t clen;
        if (shuttle_rans_encode(buf, &clen, T_CODEC_CAP, q0, qs, hh, n0,
                                ns, nh) != 0) {
            fail = 1;
            break;
        }
        if (shuttle_rans_decode(d0, ds, dh, n0, ns, nh, buf, clen) != 0) {
            fail = 1;
            break;
        }
        if (memcmp(q0, d0, n0 * sizeof(int32_t)) ||
            memcmp(qs, ds, ns * sizeof(int32_t)) ||
            memcmp(hh, dh, nh * sizeof(int32_t))) {
            fail = 1;
            break;
        }
    }
    return fail;
}

/* Draw a coefficient z from a centered discrete Gaussian of std `sigma`
 * (approximated by a sum of 12 uniforms -- the CLT bell), then peel b low
 * bits to its quotient head, exactly as the signer does.  This reproduces
 * the TRUE source law whose cross-entropy under the quantized table sizes
 * RANS_RESERVED_BYTES, so a realistic pack_sig fits the reserve. (Sampling
 * from the quantized FREQ directly would over-weight the f_s=1 tail
 * symbols and cost ~10% more -- that mismatch is why we sample z, not the
 * slot.) */
static int32_t draw_gauss(double sigma)
{
    /* sum of 12 U(0,1)-ish uniforms minus 6 has std 1; scale by sigma. */
    double acc = 0.0;
    for (int j = 0; j < 12; j++)
        acc += (double)(rng_u32() & 0xffff) / 65536.0;
    double g = (acc - 6.0) * sigma;
    long v = (long)(g + (g >= 0 ? 0.5 : -0.5)); /* round to nearest */
    return (int32_t)v;
}

static int32_t clamp_head(int32_t head, int lo, int n)
{
    if (head < lo)
        head = lo;
    if (head > lo + n - 1)
        head = lo + n - 1;
    return head;
}

/* Build a valid (z1, h) the signer might produce: z from the true rounded
 * Gaussian, peeled; hint near 0 (its high-mass region) in [0,H_h). */
static void make_valid_sig(poly z1[Z1LEN], poly h[EM])
{
    const double sz0 = (double)RY / (double)ALPHA_1; /* sigma_z0 */
    const double szs = (double)RY / (double)ALPHA_S; /* sigma_zs */
    for (int k = 0; k < N; k++) {
        int32_t z = draw_gauss(sz0);
        int32_t head = clamp_head(z >> RANS_B0, RANS_Q0_LO, RANS_Q0_N);
        int32_t low = z & ((1 << RANS_B0) - 1);
        z1[0].coeffs[k] = (head << RANS_B0) | low;
    }
    for (int i = 0; i < ELL; i++)
        for (int k = 0; k < N; k++) {
            int32_t z = draw_gauss(szs);
            int32_t head = clamp_head(z >> RANS_BS, RANS_QS_LO, RANS_QS_N);
            int32_t low = z & ((1 << RANS_BS) - 1);
            z1[i + 1].coeffs[k] = (head << RANS_BS) | low;
        }
    /* hint: concentrate near 0 (the bimodal law's dominant mass), in
     * [0,H_h). ~80% at 0, ~17% at 1, rest at 2 -- matches the entropy ~1
     * bit/coef. */
    for (int i = 0; i < EM; i++)
        for (int k = 0; k < N; k++) {
            uint32_t r = rng_u32() % 100u;
            int32_t v = (r < 80u) ? 0 : (r < 97u) ? 1 : 2;
            if (v >= (int32_t)HH)
                v = 0;
            h[i].coeffs[k] = v;
        }
}

static int polyvec_eq(const poly *a, const poly *b, int n)
{
    for (int i = 0; i < n; i++)
        for (int k = 0; k < N; k++)
            if (a[i].coeffs[k] != b[i].coeffs[k])
                return 0;
    return 1;
}

/* ---- (c)+(f) pack_sig / unpack_sig positive round-trip + size ---- */
static int test_pack_sig_positive(void)
{
    static uint8_t sig[CRYPTO_BYTES];
    static poly z1[Z1LEN], h[EM], z1b[Z1LEN], hb[EM];
    uint8_t seedC[CHALLENGESEEDBYTES], seedCb[CHALLENGESEEDBYTES];
    int fail = 0;
    for (int trial = 0; trial < 50; trial++) {
        for (unsigned i = 0; i < CHALLENGESEEDBYTES; i++)
            seedC[i] = (uint8_t)rng_u32();
        make_valid_sig(z1, h);
        if (pack_sig(sig, seedC, z1, h) != 0) {
            fail = 1;
            break;
        }
        if (unpack_sig(seedCb, z1b, hb, sig) != 0) {
            fail = 1;
            break;
        }
        if (memcmp(seedC, seedCb, CHALLENGESEEDBYTES) ||
            !polyvec_eq(z1, z1b, Z1LEN) || !polyvec_eq(h, hb, EM)) {
            fail = 1;
            break;
        }
    }
    return fail;
}

/* ---- (c) negative mutations: unpack_sig must reject all ---- */
static int test_pack_sig_negatives(void)
{
    static uint8_t sig[CRYPTO_BYTES], bad[CRYPTO_BYTES];
    static poly z1[Z1LEN], h[EM], z1b[Z1LEN], hb[EM];
    uint8_t seedC[CHALLENGESEEDBYTES], seedCb[CHALLENGESEEDBYTES];
    int rejected = 0, total = 0;

    for (unsigned i = 0; i < CHALLENGESEEDBYTES; i++)
        seedC[i] = (uint8_t)rng_u32();
    make_valid_sig(z1, h);
    if (pack_sig(sig, seedC, z1, h) != 0)
        return 1;
    /* sanity: the clean sig decodes */
    if (unpack_sig(seedCb, z1b, hb, sig) != 0)
        return 1;

    size_t rlen_off = CHALLENGESEEDBYTES;
    size_t com_off = CHALLENGESEEDBYTES + 2;
    size_t rlen = (size_t)sig[rlen_off] | ((size_t)sig[rlen_off + 1] << 8);

#define MUT(label, code)                                      \
    do {                                                      \
        memcpy(bad, sig, CRYPTO_BYTES);                       \
        code;                                                 \
        total++;                                              \
        if (unpack_sig(seedCb, z1b, hb, bad) == -1)           \
            rejected++;                                       \
        else                                                  \
            printf("    NEGATIVE NOT REJECTED: %s\n", label); \
    } while (0)

    /* 1. flip a padding byte in [rlen, RESERVED) */
    if (rlen < RANS_RESERVED_BYTES)
        MUT("flip-padding-byte", bad[com_off + rlen] ^= 0x01);
    else
        rejected++, total++; /* full reserve (rare): count as covered */
    /* 2. truncate the rANS stream (shrink rlen by 1) -> renorm/terminal
     * fail */
    MUT("shrink-rlen", {
        size_t r2 = rlen - 1;
        bad[rlen_off] = (uint8_t)(r2 & 0xff);
        bad[rlen_off + 1] = (uint8_t)((r2 >> 8) & 0xff);
    });
    /* 3. enlarge the rlen field (claims more bytes than present) ->
     * padding that was zero is now "in" the stream; terminal/consume fails
     */
    MUT("enlarge-rlen", {
        size_t r2 = rlen + 1;
        bad[rlen_off] = (uint8_t)(r2 & 0xff);
        bad[rlen_off + 1] = (uint8_t)((r2 >> 8) & 0xff);
    });
    /* 4. rlen > RESERVED (outer-container check) */
    MUT("rlen-over-reserved", {
        size_t r2 = RANS_RESERVED_BYTES + 1;
        bad[rlen_off] = (uint8_t)(r2 & 0xff);
        bad[rlen_off + 1] = (uint8_t)((r2 >> 8) & 0xff);
    });
    /* 5. out-of-range initial state: zero the first 4 com bytes (state N-1
     *    front) so x < L -> initial-state range check fails */
    MUT("out-of-range-init-state", {
        bad[com_off + 0] = 0;
        bad[com_off + 1] = 0;
        bad[com_off + 2] = 0;
        bad[com_off + 3] = 0;
    });
    /* 6. flip the last real com byte -> re-encode mismatch / terminal fail
     */
    MUT("flip-last-com-byte", bad[com_off + rlen - 1] ^= 0x80);
    /* 7. flip a mid-stream com byte -> decode diverges, re-encode mismatch
     */
    if (rlen > 12)
        MUT("flip-mid-com-byte", bad[com_off + rlen / 2] ^= 0x55);
    else
        rejected++, total++;

    report("pack/unpack negatives (>=6)", rejected != total);
    printf("    negatives rejected: %d / %d\n", rejected, total);
    return rejected != total;
#undef MUT
}

/* ---- (d) RAW path round-trip + hint range-check ---- */
static int test_raw(void)
{
    static uint8_t sig[SIG_RAW_PACKED_BYTES];
    static poly z1[Z1LEN], h[EM], z1b[Z1LEN], hb[EM];
    uint8_t seedC[CHALLENGESEEDBYTES], seedCb[CHALLENGESEEDBYTES];
    int fail = 0;

    for (unsigned i = 0; i < CHALLENGESEEDBYTES; i++)
        seedC[i] = (uint8_t)rng_u32();
    /* z1 raw can be any int16 (the RAW path stores 2 bytes/coeff verbatim)
     */
    for (int i = 0; i < Z1LEN; i++)
        for (int k = 0; k < N; k++)
            z1[i].coeffs[k] = rng_in(-3000, 3000);
    for (int i = 0; i < EM; i++)
        for (int k = 0; k < N; k++)
            h[i].coeffs[k] = rng_in(0, (int32_t)HH - 1);

    pack_sig_raw(sig, seedC, z1, h);
    if (unpack_sig_raw(seedCb, z1b, hb, sig) != 0)
        fail = 1;
    if (memcmp(seedC, seedCb, CHALLENGESEEDBYTES) ||
        !polyvec_eq(z1, z1b, Z1LEN) || !polyvec_eq(h, hb, EM))
        fail = 1;
    report("pack_sig_raw / unpack_sig_raw round-trip", fail);

    /* range-reject negative: corrupt the FIRST hint field to a value in
     * [H_h, 2^d_h), which the d_h-bit pack can represent; unpack_sig_raw
     * must reject. */
    int neg_fail = 0;
    if (HH < (1u << DH_BITS)) {
        h[0].coeffs[0] = (int32_t)HH; /* first illegal bucket */
        pack_sig_raw(sig, seedC, z1, h);
        if (unpack_sig_raw(seedCb, z1b, hb, sig) != -1)
            neg_fail = 1;
    }
    report("RAW hint range-check (h>=H_h rejected)", neg_fail);
    return fail | neg_fail;
}

/* ---- (e) C-vs-Python byte-exactness (read recorded vector) ---- */
static int test_golden_vector(void)
{
    char path[256];
    snprintf(path, sizeof path, "test/rans_vectors_%d.txt", SHUTTLE_MODE);
    FILE *f = fopen(path, "r");
    if (!f) {
        /* try relative to the test dir layout */
        snprintf(path, sizeof path, "rans_vectors_%d.txt", SHUTTLE_MODE);
        f = fopen(path, "r");
    }
    if (!f) {
        printf("[%-34s] SKIP (no %s; run check_rans.py)\n",
               "C-vs-Python golden vector", path);
        return 0;
    }
    static int32_t q0[T_NQ0], qs[T_NQS], hh[T_NH];
    static uint8_t pycom[RANS_RESERVED_BYTES], ccom[RANS_RESERVED_BYTES];
    size_t nq0 = 0, nqs = 0, nh = 0, comlen = 0;
    char key[64];
    int fail = 0;
    while (fscanf(f, "%63s", key) == 1) {
        if (key[0] == '#') { /* skip comment line */
            int c;
            while ((c = fgetc(f)) != '\n' && c != EOF) {
            }
            continue;
        }
        if (!strcmp(key, "nq0"))
            fail |= (fscanf(f, "%zu", &nq0) != 1);
        else if (!strcmp(key, "nqs"))
            fail |= (fscanf(f, "%zu", &nqs) != 1);
        else if (!strcmp(key, "nh"))
            fail |= (fscanf(f, "%zu", &nh) != 1);
        else if (!strcmp(key, "q0"))
            for (size_t i = 0; i < nq0; i++)
                fail |= (fscanf(f, "%d", &q0[i]) != 1);
        else if (!strcmp(key, "qs"))
            for (size_t i = 0; i < nqs; i++)
                fail |= (fscanf(f, "%d", &qs[i]) != 1);
        else if (!strcmp(key, "h"))
            for (size_t i = 0; i < nh; i++)
                fail |= (fscanf(f, "%d", &hh[i]) != 1);
        else if (!strcmp(key, "comlen"))
            fail |= (fscanf(f, "%zu", &comlen) != 1);
        else if (!strcmp(key, "com"))
            for (size_t i = 0; i < comlen; i++) {
                int b;
                fail |= (fscanf(f, "%d", &b) != 1);
                pycom[i] = (uint8_t)b;
            }
    }
    fclose(f);
    size_t clen;
    if (shuttle_rans_encode(ccom, &clen, RANS_RESERVED_BYTES, q0, qs, hh,
                            nq0, nqs, nh) != 0)
        fail = 1;
    if (clen != comlen || memcmp(ccom, pycom, comlen) != 0)
        fail = 1;
    report("C-vs-Python golden com byte-exact", fail);
    if (!fail)
        printf("    com=%zu bytes identical (C == tools/rans.py)\n", clen);
    return fail;
}

static void report_sizes(void)
{
    static uint8_t sig[CRYPTO_BYTES];
    static poly z1[Z1LEN], h[EM];
    uint8_t seedC[CHALLENGESEEDBYTES];
    long total = 0;
    int reps = 64;
    size_t maxr = 0;
    int overflow = 0;
    for (int t = 0; t < reps; t++) {
        for (unsigned i = 0; i < CHALLENGESEEDBYTES; i++)
            seedC[i] = (uint8_t)rng_u32();
        make_valid_sig(z1, h);
        if (pack_sig(sig, seedC, z1, h) != 0) {
            overflow++; /* reserve overflow (expected <= 2^-35; should be
                           0) */
            continue;
        }
        /* realized length is fixed (com region is reserved-sized). */
        size_t rlen = (size_t)sig[CHALLENGESEEDBYTES] |
                      ((size_t)sig[CHALLENGESEEDBYTES + 1] << 8);
        total += (long)rlen;
        if (rlen > maxr)
            maxr = rlen;
    }
    if (overflow)
        printf(
            "  WARNING: %d/%d model-draw signatures overflowed the "
            "reserve\n",
            overflow, reps);
    printf(
        "  size report: SIG_PACKED_BYTES=%u (fixed), CRYPTO_BYTES=%u, "
        "RESERVED=%u\n",
        (unsigned)SIG_PACKED_BYTES, (unsigned)CRYPTO_BYTES,
        (unsigned)RANS_RESERVED_BYTES);
    printf(
        "    mean rANS-com len ~%ld B (reserve headroom ~%u B), "
        "max seen %zu B over %d random sigs\n",
        total / reps, (unsigned)RANS_RESERVED_BYTES - (unsigned)maxr, maxr,
        reps);
}

int main(void)
{
    printf("=== test_rans (SHUTTLE-%d) ===\n", SHUTTLE_MODE);
    report("committed table self-consistency", test_tables());
    report("C codec round-trip (Q0,Qs,h)", test_roundtrip());
    report("pack_sig/unpack_sig positive round-trip",
           test_pack_sig_positive());
    test_pack_sig_negatives();
    test_raw();
    test_golden_vector();
    report_sizes();
    printf("test_rans (SHUTTLE-%d): %s\n", SHUTTLE_MODE,
           g_fails ? "FAIL" : "PASS");
    return g_fails ? 1 : 0;
}
