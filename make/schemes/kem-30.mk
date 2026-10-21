# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# PolarLAC / POLARLAC-128 (128-bit)
ALGORITHM := PolarLAC
SRCDIR := Implementations/Reference_Implementation/x86/POLARLAC-128
SRCS := KEM_AlgorithmInstance.c auxfunc.c drng.c fft.c fips202.c ntt.c pke.c polar.c poly.c       \
    sample.c symmetric.c
INC := 
DEFS := -DBIT_USE_SHAKE=0 -DRL_KEM_USE_CONJ_NTT_REJECTION=0
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-30,POLARLAC-128,kem,128,kem-30-polarlac-128,ngcc_ref_kem_30_POLARLAC_128))

# PolarLAC / POLARLAC-256 (256-bit)
ALGORITHM := PolarLAC
SRCDIR := Implementations/Reference_Implementation/x86/POLARLAC-256
SRCS := KEM_AlgorithmInstance.c auxfunc.c drng.c fft.c fips202.c ntt.c pke.c polar.c poly.c       \
    sample.c symmetric.c
INC := 
DEFS := -DBIT_USE_SHAKE=0 -DRL_KEM_USE_CONJ_NTT_REJECTION=0
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-30,POLARLAC-256,kem,256,kem-30-polarlac-256,ngcc_ref_kem_30_POLARLAC_256))

# PolarLAC / POLARLAC-Light (128-bit)
ALGORITHM := PolarLAC
SRCDIR := Implementations/Reference_Implementation/x86/POLARLAC-Light
SRCS := KEM_AlgorithmInstance.c auxfunc.c drng.c fft.c fips202.c ntt.c pke.c polar.c poly.c       \
    sample.c symmetric.c
INC := 
DEFS := -DBIT_USE_SHAKE=0 -DRL_KEM_USE_CONJ_NTT_REJECTION=0
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-30,POLARLAC-Light,kem,128,kem-30-polarlac-light,ngcc_ref_kem_30_POLARLAC_Light))

