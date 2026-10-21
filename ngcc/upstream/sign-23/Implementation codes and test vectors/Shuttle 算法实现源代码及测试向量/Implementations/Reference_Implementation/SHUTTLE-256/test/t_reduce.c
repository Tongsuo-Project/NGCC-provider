/*
 * t_reduce.c -- correctness + constant-time gates for reduce.{c,h}.
 *
 * Asserts (fails=0 required):
 *   - reduce32 / freeze / reduce_mod_2q / caddq / caddq2 are BIT-IDENTICAL
 * to a scalar reference (%q / %2q / cond-add) over a dense int32 sweep
 * (full range sampled at a stride + dense windows around 0, q, 2q,
 * +-2^31).
 *   - montgomery_reduce16 / fqmul16 / addm16 / subm16 are correct over
 * [0,q) slices (exhaustive on a/b grids). The no-idiv / no-branch
 * constant-time gate is a separate objdump scan in the Makefile (these
 * helpers run on secret NTT residues).
 */
#include <stdint.h>
#include <stdio.h>

#include "params.h"
#include "reduce.h"

/* ---- scalar reference oracles (the spec of each helper) ---- */
static int32_t ref_reduce32(int32_t a)
{
    return (int32_t)(a % (int32_t)Q); /* truncate toward zero, in (-q,q) */
}
static int32_t ref_freeze(int32_t a)
{
    int32_t r = a % (int32_t)Q;
    if (r < 0)
        r += Q;
    return r;
}
static int32_t ref_caddq(int32_t a)
{
    return a < 0 ? a + Q : a;
}
static int32_t ref_caddq2(int32_t a)
{
    return a < 0 ? a + DQ : a;
}
static int32_t ref_reduce_mod_2q(int32_t a)
{
    int32_t r = (int32_t)(a % (int32_t)DQ);
    if (r < 0)
        r += DQ;
    return r; /* in [0,2q) */
}

static int check_one(int32_t a, int *fr32, int *ffrz, int *fr2q, int *fca,
                     int *fca2)
{
    int bad = 0;
    if (reduce32(a) != ref_reduce32(a)) {
        (*fr32)++;
        bad = 1;
    }
    if (freeze(a) != ref_freeze(a)) {
        (*ffrz)++;
        bad = 1;
    }
    if (reduce_mod_2q(a) != ref_reduce_mod_2q(a)) {
        (*fr2q)++;
        bad = 1;
    }
    /* caddq / caddq2 are only well-defined for reduced inputs in (-q,q) /
     * (-2q,2q); test them on the reduced output of reduce32 (their real
     * use). */
    {
        int32_t ra = reduce32(a);
        if (caddq(ra) != ref_caddq(ra)) {
            (*fca)++;
            bad = 1;
        }
        if (caddq2(ra) != ref_caddq2(ra)) {
            (*fca2)++;
            bad = 1;
        }
    }
    return bad;
}

int main(void)
{
    int fr32 = 0, ffrz = 0, fr2q = 0, fca = 0, fca2 = 0;

    /* dense sweep across the full int32 range at a coprime-ish stride */
    for (int64_t a = INT32_MIN; a <= INT32_MAX; a += 9973)
        check_one((int32_t)a, &fr32, &ffrz, &fr2q, &fca, &fca2);

    /* dense windows around the interesting boundaries */
    int32_t centers[] = {0,         Q,      -Q,      DQ,
                         -DQ,       2 * DQ, -2 * DQ, INT32_MAX,
                         INT32_MIN, 3 * Q,  -3 * Q,  7 * Q};
    for (unsigned ci = 0; ci < sizeof(centers) / sizeof(centers[0]);
         ci++) {
        for (int64_t d = -3000; d <= 3000; d++) {
            int64_t a = (int64_t)centers[ci] + d;
            if (a < INT32_MIN || a > INT32_MAX)
                continue;
            check_one((int32_t)a, &fr32, &ffrz, &fr2q, &fca, &fca2);
        }
    }

    printf(
        "[reduce32]      bit-identical to a%%q                : %s (%d)\n",
        fr32 ? "FAIL" : "PASS", fr32);
    printf(
        "[freeze]        bit-identical to a mod^+ q          : %s (%d)\n",
        ffrz ? "FAIL" : "PASS", ffrz);
    printf(
        "[reduce_mod_2q] bit-identical to a mod^+ 2q         : %s (%d)\n",
        fr2q ? "FAIL" : "PASS", fr2q);
    printf(
        "[caddq]         conditional +q (on reduced input)  : %s (%d)\n",
        fca ? "FAIL" : "PASS", fca);
    printf(
        "[caddq2]        conditional +2q (on reduced input) : %s (%d)\n",
        fca2 ? "FAIL" : "PASS", fca2);

    /* ---- uint16 Montgomery core over [0,q) ---- */
    /* montgomery_reduce16(a) = a * 2^-16 mod q, for 0 <= a < q*2^16.  Test
     * on a grid of products a*b with a,b in [0,q). */
    int fm = 0;
    uint32_t rinv = 1; /* (2^16)^-1 mod q, by extended check below */
    {
        /* compute RINV = 2^16^-1 mod q via Fermat (q prime) */
        uint64_t base = (1u << 16) % Q, r = 1, e = Q - 2;
        while (e) {
            if (e & 1)
                r = r * base % Q;
            base = base * base % Q;
            e >>= 1;
        }
        rinv = (uint32_t)r;
    }
    for (uint32_t a = 0; a < (uint32_t)Q; a += 257) {
        for (uint32_t b = 0; b < (uint32_t)Q; b += 263) {
            uint16_t got = fqmul16((uint16_t)a, (uint16_t)b);
            uint16_t want = (uint16_t)((uint64_t)a * b % Q * rinv % Q);
            if (got != want) {
                fm++;
                if (fm < 4)
                    printf("  [fqmul] a=%u b=%u got=%u want=%u\n", a, b,
                           got, want);
            }
        }
    }
    printf(
        "[fqmul16/montgomery_reduce16] correct over [0,q) grid: %s (%d)\n",
        fm ? "FAIL" : "PASS", fm);

    int fas = 0;
    for (uint32_t a = 0; a < (uint32_t)Q; a += 251) {
        for (uint32_t b = 0; b < (uint32_t)Q; b += 269) {
            if (addm16((uint16_t)a, (uint16_t)b) !=
                (uint16_t)((a + b) % Q))
                fas++;
            uint16_t ws = (uint16_t)(((int)a - (int)b % Q + Q) % Q);
            if (subm16((uint16_t)a, (uint16_t)b) != ws)
                fas++;
        }
    }
    printf(
        "[addm16/subm16] correct over [0,q) grid             : %s (%d)\n",
        fas ? "FAIL" : "PASS", fas);

    int fails = fr32 + ffrz + fr2q + fca + fca2 + fm + fas;
    printf("\nSUMMARY t_reduce (SHUTTLE-%d) fails=%d\n", SHUTTLE_MODE,
           fails);
    return fails ? 1 : 0;
}
