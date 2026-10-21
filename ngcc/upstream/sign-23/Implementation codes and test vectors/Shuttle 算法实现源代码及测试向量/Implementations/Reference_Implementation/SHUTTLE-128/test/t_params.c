/*
 * t_params.c - structural self-check for the SHUTTLE parameter headers.
 *
 * Compile-and-run gate: prints the key per-set constants for the
 * built SHUTTLE_MODE and compile-time-asserts the structural invariants
 * (vector lengths, the H_h / mask-width / 2n-th-root relations, and the exact
 * public-key sizes pk == 1264 / 1952 / 3648).
 *
 * Built for all three modes by the Makefile (`TESTS` includes t_params).  Must
 * be -Werror-clean under the reference flags (-std=c99 -Wpedantic -Wall
 * -Wextra).  Exits 0 on success.
 */

#include <stdint.h>
#include <stdio.h>

#include "api.h" /* pulls params.h -> config.h -> namespace.h */

/* ---- structural invariants (compile-time) ---- */
_Static_assert(KVEC == 1 + ELL + EM, "KVEC == 1+ELL+EM");
_Static_assert(Z1LEN == 1 + ELL, "Z1LEN == 1+ELL");
_Static_assert(HH == 2 * (Q - 1) / ALPHA_H, "HH == 2(q-1)/alpha_h");
_Static_assert((INT64_C(1) << DQ_BITS) >= Q, "2^DQ_BITS >= Q");
_Static_assert((INT64_C(1) << (DQ_BITS - 1)) < Q, "DQ_BITS is tight (ceil)");
_Static_assert(DQ == 2 * Q, "DQ == 2*Q");
_Static_assert(SEEDBYTES == LAMBDA / 8, "SEEDBYTES == lambda/8");
_Static_assert(CHALLENGESEEDBYTES == LAMBDA / 4, "CHALLENGESEEDBYTES == lambda/4");
_Static_assert(CHALLENGE_PACKEDBYTES == (N + 7) / 8, "CHALLENGE_PACKEDBYTES == (N+7)/8");
_Static_assert(CRYPTO_PUBLICKEYBYTES == SEEDBYTES + EM * POLYPK_PACKEDBYTES,
               "pk == SEEDBYTES + EM*POLYPK_PACKEDBYTES");

/* Per-set pinned public-key sizes (the headline acceptance check). */
#if SHUTTLE_MODE == 128
_Static_assert(CRYPTO_PUBLICKEYBYTES == 1264, "pk(128) == 1264");
_Static_assert(N == 256 && Q == 15361 && ELL == 3 && EM == 3 && KVEC == 7,
               "SHUTTLE-128 primaries");
#elif SHUTTLE_MODE == 256
_Static_assert(CRYPTO_PUBLICKEYBYTES == 1952, "pk(256) == 1952");
_Static_assert(N == 512 && Q == 61441 && ELL == 3 && EM == 2 && KVEC == 6,
               "SHUTTLE-256 primaries");
#elif SHUTTLE_MODE == 512
_Static_assert(CRYPTO_PUBLICKEYBYTES == 3648, "pk(512) == 3648");
_Static_assert(N == 1024 && Q == 59393 && ELL == 3 && EM == 2 && KVEC == 6,
               "SHUTTLE-512 primaries");
#endif

int main(void)
{
    printf("=== %s (SHUTTLE_MODE=%d) ===\n", CRYPTO_ALGNAME, SHUTTLE_MODE);
    printf("N                     = %d\n", N);
    printf("Q                     = %d\n", Q);
    printf("ELL                   = %d\n", ELL);
    printf("EM                    = %d\n", EM);
    printf("KVEC                  = %d\n", KVEC);
    printf("Z1LEN                 = %d\n", Z1LEN);
    printf("SEEDBYTES             = %d\n", SEEDBYTES);
    printf("CHALLENGESEEDBYTES    = %d\n", CHALLENGESEEDBYTES);
    printf("DB_BITS               = %d\n", DB_BITS);
    printf("HH                    = %d\n", HH);
    printf("ZETA                  = %d\n", ZETA);
    printf("NINV                  = %d\n", NINV);
    printf("CRYPTO_PUBLICKEYBYTES = %d\n", CRYPTO_PUBLICKEYBYTES);
    return 0;
}
