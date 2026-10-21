SQIsignTriangle_lvl5 — Reference_Implementation

This directory instantiates the ICCS API_PKC digital-signature interface for the
SQIsignTriangle "SQIsignTriangle_lvl5" parameter set, built with the "ref" backend.

Files:
- SIG_AlgorithmInstance.h : API_PKC SIG interface and instance name.
- SIG_AlgorithmInstance.c : wrapper from the API_PKC SIG interface to the
                            SQIsignTriangle C NIST API (crypto_sign_keypair /
                            crypto_sign / crypto_sign_open). Also routes
                            randombytes() through the ICCS DRNG.
- KAT_SIG.c, drng.c, drng.h, auxfunc.c, auxfunc.h : unchanged ICCS API_PKC
                            template files. auxfunc.c provides the SM3-based
                            hash (sm3hash) and XOF (pseudoXOF) primitives.
- CMakeLists.txt          : automated build entry (single parameter set).
- build.sh                : configure + build the KAT generator.
- generate_kat.sh         : build, run, and copy the KAT vector to
                            ../../../Test_Vectors/.
- sqisigntriangle/        : self-contained SQIsignTriangle C source. Its
                            FIPS-202 module (src/common/generic/fips202.c) has
                            been replaced by an adapter that implements the
                            SHAKE/SHA3 API on top of the mandated ICCS
                            auxiliary functions (auxfunc.c), so every hash/XOF
                            call in the scheme uses SM3 (see Note 5 of th
