# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# DKEM / DKEM-128 (128-bit)
ALGORITHM := DKEM
SRCDIR := Implementations/Reference_Implementation/DKEM-128
SRCS := sm3.c dke_sm3.c dke_hash.c reduce.c ntt.c poly.c polyvec.c random_sampling.c dke_utils.c  \
    packing.c verify.c dkecpa.c dkecca.c KEM_AlgorithmInstance.c drng.c auxfunc.c
INC := 
DEFS := -DDKE_FORCE_SCALAR -DDKE_MODE=128 -DDKE_HASH=0 -DDKE_RANDOM=0
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_NO_DRNG
NOAUX := 
$(eval $(call ngcc_instance,kem-13,DKEM-128,kem,128,kem-13-dkem-128,ngcc_ref_kem_13_DKEM_128))

# DKEM / DKEM-256 (256-bit)
ALGORITHM := DKEM
SRCDIR := Implementations/Reference_Implementation/DKEM-256
SRCS := sm3.c dke_sm3.c dke_hash.c reduce.c ntt.c poly.c polyvec.c random_sampling.c dke_utils.c  \
    packing.c verify.c dkecpa.c dkecca.c KEM_AlgorithmInstance.c drng.c auxfunc.c
INC := 
DEFS := -DDKE_FORCE_SCALAR -DDKE_MODE=256 -DDKE_HASH=0 -DDKE_RANDOM=0
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_NO_DRNG
NOAUX := 
$(eval $(call ngcc_instance,kem-13,DKEM-256,kem,256,kem-13-dkem-256,ngcc_ref_kem_13_DKEM_256))

