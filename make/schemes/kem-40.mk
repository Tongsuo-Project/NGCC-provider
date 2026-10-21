# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# YuanYang.KEM / yuanyang-1024 (256-bit)
ALGORITHM := YuanYang.KEM
SRCDIR := Implementations/Reference_Implementation/yuanyang-1024
SRCS := prng.c auxfunc.c drng.c ntt.c codec.c poly_inv_ntt.c KEM_AlgorithmInstance.c kem.c kem-keygen.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-40,yuanyang-1024,kem,256,kem-40-yuanyang-1024,ngcc_ref_kem_40_yuanyang_1024))

# YuanYang.KEM / yuanyang-512 (128-bit)
ALGORITHM := YuanYang.KEM
SRCDIR := Implementations/Reference_Implementation/yuanyang-512
SRCS := prng.c auxfunc.c drng.c ntt.c codec.c poly_inv_ntt.c KEM_AlgorithmInstance.c kem.c kem-keygen.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-40,yuanyang-512,kem,128,kem-40-yuanyang-512,ngcc_ref_kem_40_yuanyang_512))

