# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# UVW signature / UVW-128 (128-bit)
ALGORITHM := UVW signature
SRCDIR := Implementations/Reference_Implementation/UVW-128
SRCS := SIG_AlgorithmInstance.c uvw.c gauss.c auxfunc.c drng.c fq_arithmetic/vf3.c                \
    fq_arithmetic/mf3.c fq_arithmetic/dp.c
INC := 
DEFS := -DUSE_API_PKC
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-32,UVW-128,sig,128,sig-32-uvw-128,ngcc_ref_sig_32_UVW_128))

# UVW signature / UVW-256 (256-bit)
ALGORITHM := UVW signature
SRCDIR := Implementations/Reference_Implementation/UVW-256
SRCS := SIG_AlgorithmInstance.c uvw.c gauss.c auxfunc.c drng.c fq_arithmetic/vf3.c                \
    fq_arithmetic/mf3.c fq_arithmetic/dp.c
INC := 
DEFS := -DUSE_API_PKC
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-32,UVW-256,sig,256,sig-32-uvw-256,ngcc_ref_sig_32_UVW_256))
