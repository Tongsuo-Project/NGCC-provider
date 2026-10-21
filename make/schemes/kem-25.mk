# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# NEV / NEV_1024_1409_ICCS (256-bit)
ALGORITHM := NEV
SRCDIR := Implementations/Reference_Implementation/NEV-R2
SRCS := poly.c ntt.c owpke.c verify.c sample.c pack.c cca.c cpa.c symmetrics/hashkdf.c            \
    symmetrics/fips202.c symmetrics/auxfunc.c kat_test/rng.c kat_test/drng.c                      \
    kat_test/KEM_AlgorithmInstance.c
INC := -Isrc/NEV_1024_1409_ICCS/symmetrics -Isrc/NEV_1024_1409_ICCS/kat_test
DEFS := -DPARAMS=5 -DUSE_ICCS
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"kat_test/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-25,NEV_1024_1409_ICCS,kem,256,kem-25-nev-1024-1409-iccs,ngcc_ref_kem_25_NEV_1024_1409_ICCS))

# NEV / NEV_1024_3329_ICCS (256-bit)
ALGORITHM := NEV
SRCDIR := Implementations/Reference_Implementation/NEV-D2
SRCS := poly.c ntt.c owpke.c verify.c sample.c pack.c cca.c cpa.c symmetrics/hashkdf.c            \
    symmetrics/fips202.c symmetrics/auxfunc.c kat_test/rng.c kat_test/drng.c                      \
    kat_test/KEM_AlgorithmInstance.c
INC := -Isrc/NEV_1024_3329_ICCS/symmetrics -Isrc/NEV_1024_3329_ICCS/kat_test
DEFS := -DPARAMS=8 -DUSE_ICCS
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"kat_test/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-25,NEV_1024_3329_ICCS,kem,256,kem-25-nev-1024-3329-iccs,ngcc_ref_kem_25_NEV_1024_3329_ICCS))

# NEV / NEV_1024_769_C_ICCS (256-bit)
ALGORITHM := NEV
SRCDIR := Implementations/Reference_Implementation/NEV-C2-c
SRCS := poly.c ntt.c owpke.c verify.c sample.c pack.c cca.c cpa.c symmetrics/hashkdf.c            \
    symmetrics/fips202.c symmetrics/auxfunc.c kat_test/rng.c kat_test/drng.c                      \
    kat_test/KEM_AlgorithmInstance.c
INC := -Isrc/NEV_1024_769_C_ICCS/symmetrics -Isrc/NEV_1024_769_C_ICCS/kat_test
DEFS := -DPARAMS=2 -DUSE_ICCS
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"kat_test/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-25,NEV_1024_769_C_ICCS,kem,256,kem-25-nev-256,ngcc_ref_kem_25_NEV_1024_769_C_ICCS))

# NEV / NEV_1024_769_ICCS (256-bit)
ALGORITHM := NEV
SRCDIR := Implementations/Reference_Implementation/NEV-C2
SRCS := poly.c ntt.c owpke.c verify.c sample.c pack.c cca.c cpa.c symmetrics/hashkdf.c            \
    symmetrics/fips202.c symmetrics/auxfunc.c kat_test/rng.c kat_test/drng.c                      \
    kat_test/KEM_AlgorithmInstance.c
INC := -Isrc/NEV_1024_769_ICCS/symmetrics -Isrc/NEV_1024_769_ICCS/kat_test
DEFS := -DPARAMS=11 -DUSE_ICCS
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"kat_test/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-25,NEV_1024_769_ICCS,kem,256,kem-25-nev-1024-769-iccs,ngcc_ref_kem_25_NEV_1024_769_ICCS))

# NEV / NEV_512_1409_ICCS (128-bit)
ALGORITHM := NEV
SRCDIR := Implementations/Reference_Implementation/NEV-R1
SRCS := poly.c ntt.c owpke.c verify.c sample.c pack.c cca.c cpa.c symmetrics/hashkdf.c            \
    symmetrics/fips202.c symmetrics/auxfunc.c kat_test/rng.c kat_test/drng.c                      \
    kat_test/KEM_AlgorithmInstance.c
INC := -Isrc/NEV_512_1409_ICCS/symmetrics -Isrc/NEV_512_1409_ICCS/kat_test
DEFS := -DPARAMS=4 -DUSE_ICCS
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"kat_test/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-25,NEV_512_1409_ICCS,kem,128,kem-25-nev-512-1409-iccs,ngcc_ref_kem_25_NEV_512_1409_ICCS))

# NEV / NEV_512_3329_ICCS (128-bit)
ALGORITHM := NEV
SRCDIR := Implementations/Reference_Implementation/NEV-D1
SRCS := poly.c ntt.c owpke.c verify.c sample.c pack.c cca.c cpa.c symmetrics/hashkdf.c            \
    symmetrics/fips202.c symmetrics/auxfunc.c kat_test/rng.c kat_test/drng.c                      \
    kat_test/KEM_AlgorithmInstance.c
INC := -Isrc/NEV_512_3329_ICCS/symmetrics -Isrc/NEV_512_3329_ICCS/kat_test
DEFS := -DPARAMS=7 -DUSE_ICCS
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"kat_test/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-25,NEV_512_3329_ICCS,kem,128,kem-25-nev-512-3329-iccs,ngcc_ref_kem_25_NEV_512_3329_ICCS))

# NEV / NEV_512_769_C_ICCS (128-bit)
ALGORITHM := NEV
SRCDIR := Implementations/Reference_Implementation/NEV-C1-c
SRCS := poly.c ntt.c owpke.c verify.c sample.c pack.c cca.c cpa.c symmetrics/hashkdf.c            \
    symmetrics/fips202.c symmetrics/auxfunc.c kat_test/rng.c kat_test/drng.c                      \
    kat_test/KEM_AlgorithmInstance.c
INC := -Isrc/NEV_512_769_C_ICCS/symmetrics -Isrc/NEV_512_769_C_ICCS/kat_test
DEFS := -DPARAMS=1 -DUSE_ICCS
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"kat_test/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-25,NEV_512_769_C_ICCS,kem,128,kem-25-nev-128,ngcc_ref_kem_25_NEV_512_769_C_ICCS))

# NEV / NEV_512_769_ICCS (128-bit)
ALGORITHM := NEV
SRCDIR := Implementations/Reference_Implementation/NEV-C1
SRCS := poly.c ntt.c owpke.c verify.c sample.c pack.c cca.c cpa.c symmetrics/hashkdf.c            \
    symmetrics/fips202.c symmetrics/auxfunc.c kat_test/rng.c kat_test/drng.c                      \
    kat_test/KEM_AlgorithmInstance.c
INC := -Isrc/NEV_512_769_ICCS/symmetrics -Isrc/NEV_512_769_ICCS/kat_test
DEFS := -DPARAMS=10 -DUSE_ICCS
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"kat_test/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-25,NEV_512_769_ICCS,kem,128,kem-25-nev-512-769-iccs,ngcc_ref_kem_25_NEV_512_769_ICCS))

