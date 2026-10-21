# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Lore / Lore-SM3-L1 (128-bit)
ALGORITHM := Lore
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Lore-SM3/Lore-L1
SRCS := kem.c pke.c indcpa.c polyvec.c poly.c ntt.c sampler.c reduce.c toomcook.c verify.c        \
    bch_codec.c randombytes.c KEM_AlgorithmInstance.c drng.c lore_symmetric.c lore_sm3_wrapper.c  \
    lore_frame.c auxfunc.c
INC := 
DEFS := -DLORE_LEVEL=1 -DLORE_USE_API_PKC_DRNG -DLORE_BACKEND_SM3
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE='"Lore-SM3-L1"'
NOAUX := 
$(eval $(call ngcc_instance,kem-19,Lore-SM3-L1,kem,128,kem-19-lore-sm3-l1,ngcc_ref_kem_19_Lore_SM3_L1))

# Lore / Lore-SM3-L2 (256-bit)
ALGORITHM := Lore
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Lore-SM3/Lore-L2
SRCS := kem.c pke.c indcpa.c polyvec.c poly.c ntt.c sampler.c reduce.c toomcook.c verify.c        \
    bch_codec.c randombytes.c KEM_AlgorithmInstance.c drng.c lore_symmetric.c lore_sm3_wrapper.c  \
    lore_frame.c auxfunc.c
INC := 
DEFS := -DLORE_LEVEL=2 -DLORE_USE_API_PKC_DRNG -DLORE_BACKEND_SM3
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE='"Lore-SM3-L2"'
NOAUX := 
$(eval $(call ngcc_instance,kem-19,Lore-SM3-L2,kem,256,kem-19-lore-sm3-l2,ngcc_ref_kem_19_Lore_SM3_L2))

