/*
 * test_kat.c -- self-contained deterministic KAT regression baseline for
 * SHUTTLE (the correctness capstone).
 *
 * This is the AUTHORITATIVE regression hash.  It replays the EXACT NGCC
 * KAT_SIG.c byte protocol (the three SM3 Hash-DRBG chains drng_seed /
 * drng_msg / drng_algorithm, seeded from the fixed per-MODE nonces) so the
 * FNV-1a hash it computes is aligned with the institute KAT file
 * (output/KAT_SIG_SHUTTLE-<set>.txt), yet it is fully self-contained: it
 * links NO external randombytes.c -- the deterministic source IS the NGCC
 * drng machinery.
 *
 * Build (per mode m in 128/256/512), NGCC_MODE (default) RAW or rANS:
 *   gcc -O2 -std=c99 -Wall -Wextra -I. -Itest -I../tools -Intt/<qset> \
 *       [-DSIG_RAW] -DSHUTTLE_MODE=<m> \
 *       test/test_kat.c SIG_AlgorithmInstance.c sign.c polyvec.c sampler.c \
 *       sampler_u.c irs.c rounding.c packing.c poly.c poly_ntt.c reduce.c \
 *       rans.c approx_exp.c approx_log.c symmetric.c ntt/<qset>/ntt_ref.c \
 *       drng.c auxfunc.c -o out/test_kat_<m>
 *   (SHA3_MODE: add -DSHA3_MODE and swap drng.c+auxfunc.c -> fips202.c.)
 *
 * Protocol (mirrors KAT_SIG.c lines 56-155 exactly):
 *   - drng_seed  : init from nonce "seed" repeated to 64 B.
 *   - drng_msg   : init from nonce "msg" repeated (21x = 63 B) then byte
 *                  63 = 'm'  (== "msg"x21 + "m", the KAT_SIG.c layout).
 *   - For Count = 0..9:
 *       seed <- get_random_number(drng_seed, 64*8 bits)
 *       m    <- get_random_number(drng_msg , m_len*8 bits), m_len = 56..128
 *       init_random_number(drng_algorithm, seed, 64)   (xi+rnd source)
 *       sig_keygen -> pk, sk ; sig_sign(m) -> sn ; FNV over pk||sk||sn
 *       assert sig_verify(pk, sn, m) == 0   (ACCEPT)
 *       tamper sn[sn_len/2] ^= 0x40 ; assert sig_verify != 0 (REJECT)
 *       m_len += 8
 *   - print "KAT_hash=<u64 decimal>"; exit nonzero on any verify/tamper
 *     failure or any size mismatch.
 *
 * The hashed transcript is the concatenation, in Count order, of
 *   pk (pk_len) || sk (sk_len) || sn (sn_len)
 * with the realized lengths -- identical to what KAT_SIG.c emits as the
 * PK/SK/Sn hex fields.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include "params.h" /* for the size sanity-prints only */

#define SEED_LEN_BYTES 64
#define NKAT 10

/* The scheme-internal DRBG the NGCC adapter (SIG_AlgorithmInstance.c) draws
 * xi/rnd from; KAT_SIG.c owns this global.  We provide it here so the
 * adapter's `extern DRNG_ctx drng_algorithm;` resolves. */
DRNG_ctx drng_algorithm;

/* ---- FNV-1a (64-bit), Lithium/test_kat convention ----
 * offset basis 0xcbf29ce484222325, prime 0x100000001b3. */
static uint64_t fnv1a(uint64_t h, const uint8_t *p, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i) {
        h ^= (uint64_t)p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

int main(void)
{
    DRNG_ctx drng_seed;
    DRNG_ctx drng_msg;
    uint8_t nonce1[SEED_LEN_BYTES];
    uint8_t nonce2[SEED_LEN_BYTES];
    unsigned long long pk_len, sk_len, sn_len_cap;
    uint8_t *pk, *sk, *sn, *sn_tampered;
    uint8_t seed[SEED_LEN_BYTES];
    uint8_t m[128];
    int m_len = 56;
    int fails = 0;
    int i;
    uint64_t hash = 0xcbf29ce484222325ULL;

#if defined(SIG_RAW)
    const char *path = "RAW";
#else
    const char *path = "rANS";
#endif

    /* drng_seed nonce = "seed" repeated to 64 B (SEED_LEN_BYTES/4 = 16). */
    for (i = 0; i < SEED_LEN_BYTES / 4; ++i)
        memcpy(nonce1 + 4 * i, "seed", 4);
    init_random_number(&drng_seed, nonce1, SEED_LEN_BYTES);

    /* drng_msg nonce = "msg" repeated (SEED_LEN_BYTES/3 = 21 copies = 63 B)
     * then byte 63 overwritten with 'm' -- exactly KAT_SIG.c:71-77. */
    for (i = 0; i < SEED_LEN_BYTES / 3; ++i)
        memcpy(nonce2 + 3 * i, "msg", 3);
    memcpy(nonce2 + SEED_LEN_BYTES - 1, "m", 1);
    init_random_number(&drng_msg, nonce2, SEED_LEN_BYTES);

    pk_len = sig_get_pk_len_bytes();
    sk_len = sig_get_sk_len_bytes();
    sn_len_cap = sig_get_sn_len_bytes();

    pk = calloc((size_t)pk_len, 1);
    sk = calloc((size_t)sk_len, 1);
    sn = calloc((size_t)sn_len_cap, 1);
    sn_tampered = calloc((size_t)sn_len_cap, 1);
    if (!pk || !sk || !sn || !sn_tampered) {
        printf("FAIL: allocation\n");
        return 2;
    }

    printf("== SHUTTLE-%d test_kat (%s packing) ==\n", (int)LAMBDA, path);
    printf("  pk_len=%llu sk_len=%llu sn_cap=%llu\n", pk_len, sk_len,
           sn_len_cap);

    for (i = 0; i < NKAT; ++i) {
        unsigned long long pkl = pk_len, skl = sk_len, snl = 0;
        int rc, v;

        /* seed (64 B) then message (m_len B) from the harness DRBGs. */
        get_random_number(&drng_seed, seed, (unsigned long long)SEED_LEN_BYTES * 8);
        get_random_number(&drng_msg, m, (unsigned long long)m_len * 8);

        /* seed the scheme-internal DRBG (xi for KeyGen, rnd for Sign). */
        init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);

        rc = sig_keygen(pk, &pkl, sk, &skl);
        if (rc != 0) {
            printf("FAIL[%d]: sig_keygen returned %d\n", i, rc);
            fails++;
            break;
        }
        if (pkl != pk_len || skl != sk_len) {
            printf("FAIL[%d]: keygen len drift pk=%llu sk=%llu\n", i, pkl,
                   skl);
            fails++;
        }

        rc = sig_sign(sk, sk_len, m, (unsigned long long)m_len, sn, &snl);
        if (rc != 0) {
            printf("FAIL[%d]: sig_sign returned %d\n", i, rc);
            fails++;
            break;
        }
        if (snl > sn_len_cap) {
            printf("FAIL[%d]: Sn_Len %llu > sn buffer cap %llu\n", i,
                   snl, sn_len_cap);
            fails++;
        }

        /* accumulate FNV-1a over pk || sk || sn (realized lengths). */
        hash = fnv1a(hash, pk, (size_t)pk_len);
        hash = fnv1a(hash, sk, (size_t)sk_len);
        hash = fnv1a(hash, sn, (size_t)snl);

        /* verify ACCEPT. */
        v = sig_verify(pk, pk_len, sn, snl, m, (unsigned long long)m_len);
        if (v != 0) {
            printf("FAIL[%d]: verify returned %d (expected 0 accept)\n", i,
                   v);
            fails++;
        }

        /* tamper one byte -> verify REJECT (sig[len/2] ^= 0x40). */
        memcpy(sn_tampered, sn, (size_t)snl);
        sn_tampered[snl / 2] ^= 0x40;
        v = sig_verify(pk, pk_len, sn_tampered, snl, m,
                       (unsigned long long)m_len);
        if (v == 0) {
            printf("FAIL[%d]: tampered sig verified (expected reject)\n", i);
            fails++;
        }

        m_len += 8;
    }

    printf("KAT_hash=%llu\n", (unsigned long long)hash);

    free(pk);
    free(sk);
    free(sn);
    free(sn_tampered);

    printf("== SHUTTLE-%d test_kat (%s): %s ==\n", (int)LAMBDA, path,
           fails == 0 ? "PASS" : "FAIL");
    return fails == 0 ? 0 : 1;
}
