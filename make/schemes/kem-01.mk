# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Aigis-Enc+ / Aigis-enc1 (128-bit)
ALGORITHM := Aigis-Enc+
SRCDIR := Implementations/Reference_Implementation/Aigis-Enc+-I
SRCS := cbd.c fips202.c poly.c reduce.c precomp.c ntt.c polyvec.c owcpa.c kem.c verify.c          \
    hashkdf.c gen_a.c pack.c kat_test/auxfunc.c kat_test/KEM_AlgorithmInstance.c kat_test/drng.c  \
    kat_test/rng.c
INC := -Isrc/Aigis-enc1/kat_test
DEFS := -DPARAMS=1 -DUSE_NICCS_API
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"kat_test/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-01,Aigis-enc1,kem,128,kem-01-aigis-enc1,ngcc_ref_kem_01_Aigis_enc1))

# Aigis-Enc+ / Aigis-enc2 (256-bit)
ALGORITHM := Aigis-Enc+
SRCDIR := Implementations/Reference_Implementation/Aigis-Enc+-II
SRCS := cbd.c fips202.c poly.c reduce.c precomp.c ntt.c polyvec.c owcpa.c kem.c verify.c          \
    hashkdf.c gen_a.c pack.c kat_test/auxfunc.c kat_test/KEM_AlgorithmInstance.c kat_test/drng.c  \
    kat_test/rng.c
INC := -Isrc/Aigis-enc2/kat_test
DEFS := -DPARAMS=2 -DUSE_NICCS_API
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"kat_test/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-01,Aigis-enc2,kem,256,kem-01-aigis-enc2,ngcc_ref_kem_01_Aigis_enc2))

