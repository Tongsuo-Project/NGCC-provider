QingLuan-256 - Reference Implementation (single algorithm instance)
==================================================================
Security: classical 256-bit, quantum >= 128-bit. Post-quantum signature (R-SDP / MPC-in-the-Head / SM3).
Self-contained: the security level is fixed in include/params.h, so it builds with NO compile-time flags.

Build:  make            # or: gcc -Wall -Wextra -std=c11 -O2 -Iinclude src/*.c test/main.c -o qingluan_test -lbcrypt
Test:   ./qingluan_test # power-on self-test + keygen/sign/verify round-trip + tamper rejection
KAT:    ./qingluan_kat  # writes KAT_SIG_QingLuan-256.txt: 10 deterministic, self-verified vectors

Layout:
  include/  api.h (public API), params.h (fixed to this level), module headers
  src/      fq_arith, restr, rsdp, hash (multi-pipe SM3), mpc (CROSS-RSDP), keygen, sign, verify, utils (SM3 DRBG), selftest
  test/     main (round-trip), kat_gen (internal determinism KAT), unit + security-regression tests
  api_pkc/  API_PKC framework + adapter (cd api_pkc): `make run` -> OFFICIAL
            framework-format KATs; `make verify` -> validate published root vectors
Algorithm docs (shared):  ../../../Supporting_Documentation/{protocol_reference,parameters,security-argument}.md
Official KAT vectors:     submission-root Test_Vectors/ (../../../Test_Vectors/KAT_SIG_QingLuan-<level>.txt)
