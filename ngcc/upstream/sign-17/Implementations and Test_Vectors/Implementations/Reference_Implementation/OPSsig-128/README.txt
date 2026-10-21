OPSsig-128 Reference Implementation

This directory contains the ISO C reference implementation for the
`OPSsig-128` parameter set.

Main files

sign.c / sign.h             Core key generation, signing and verification logic
mult.c / mult.h             Small-polynomial multiplication helpers
packing.c / packing.h       Public key, secret key and signature packing helpers
poly.c / poly.h             Polynomial arithmetic, sampling and packing helpers
polyvec.c / polyvec.h       Polynomial vector and matrix helpers
ntt.c / ntt.h               NTT and inverse NTT routines
reduce.c / reduce.h         Modular reduction helpers
rounding.c / rounding.h     Rounding and hint helpers
params.h                    Scheme parameter definitions
auxfunc.c / auxfunc.h       ICCS auxiliary hash and XOF functions
drng.c / drng.h             ICCS deterministic random number generator
SIG_AlgorithmInstance.c     ICCS SIG interface adapter
SIG_AlgorithmInstance.h     ICCS SIG interface declarations and metadata

Build and test files

Makefile                    Builds correctness, speed, memory and KAT targets
test_OPSsig.c               Correctness regression test
test_speed_OPSsig.c         Speed benchmark
test_mem_OPSsig.c           Runtime memory benchmark
test_static_mem_OPSsig.c    Static memory measurement binary
test_ntt.c                  NTT-specific verification test
test_ntt_260602.c           Extended NTT regression test
test_vectors-260602.c       Vector regression test
test_smallpoly_ops128.c     Small-polynomial regression test
test_bench_ops128.c         Additional benchmark entry point
KAT_SIG.c                   KAT generator for the signature scheme

Notes

1.  `make selfeval` exports correctness, speed, memory and KAT artifacts into
    `../../../Self_Evaluation/results/reference/OPSsig-128/`.
2.  The KAT generator produces `KAT_SIG_OPSsig.txt`; the maintained submission
    copy is stored under `../../../Test_Vectors/reference/OPSsig-128/`.
3.  This reference implementation targets ISO C99 compilation.
4.  The `KEM_*`, `KEX_*`, `KAT_KEM.c` and `KAT_KEX.c` files are template files
    from the API package and are not part of the submitted signature algorithm.
