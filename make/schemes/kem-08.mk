# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# BW-KEM / BW_KEM_C128 (128-bit)
ALGORITHM := BW-KEM
SRCDIR := Implementations/Reference_Implementation/BW_KEM_C128
SRCS := BWcoding.c KEM_AlgorithmInstance.c auxfunc.c cbd.c drng.c indcpa.c kem.c ntt.c poly.c     \
    polyvec.c randombytes_api.c reduce.c symmetric-iccs.c verify.c
INC := 
DEFS := -DBWKEM128_INTERNAL_COMPAT -DBWKEM128_USE_ICCS_AUXFUNC
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-08,BW_KEM_C128,kem,128,kem-bw-kem-128,ngcc_ref_kem_08_BW_KEM_C128))

# BW-KEM / BW_KEM_C256 (256-bit)
ALGORITHM := BW-KEM
SRCDIR := Implementations/Reference_Implementation/BW_KEM_C256
SRCS := BWcoding.c KEM_AlgorithmInstance.c auxfunc.c cbd.c drng.c indcpa.c kem.c ntt.c poly.c     \
    polyvec.c randombytes_api.c reduce.c symmetric-iccs.c verify.c
INC := 
DEFS := -DBWKEM_C256_USE_ICCS_AUXFUNC
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-08,BW_KEM_C256,kem,256,kem-bw-kem-256,ngcc_ref_kem_08_BW_KEM_C256))

