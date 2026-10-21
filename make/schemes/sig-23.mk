# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Shuttle / SHUTTLE-128 (128-bit)
ALGORITHM := Shuttle
SRCDIR := Implementation codes and test vectors/Shuttle 算法实现源代码及测试向量/Implementations/Reference_Implementation/SHUTTLE-128
SRCS := SIG_AlgorithmInstance.c sign.c polyvec.c sampler.c sampler_u.c irs.c rounding.c packing.c \
    poly.c poly_ntt.c reduce.c rans.c approx_exp.c approx_log.c symmetric.c drng.c auxfunc.c      \
    ntt/q15361n256/ntt_ref.c
INC := -Isrc/SHUTTLE-128/ntt/q15361n256 -Iauthor-tools
DEFS := -DSHUTTLE_MODE=128
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-23,SHUTTLE-128,sig,128,sig-23-shuttle-128,ngcc_ref_sig_23_SHUTTLE_128))

# Shuttle / SHUTTLE-256 (256-bit)
ALGORITHM := Shuttle
SRCDIR := Implementation codes and test vectors/Shuttle 算法实现源代码及测试向量/Implementations/Reference_Implementation/SHUTTLE-256
SRCS := SIG_AlgorithmInstance.c sign.c polyvec.c sampler.c sampler_u.c irs.c rounding.c packing.c \
    poly.c poly_ntt.c reduce.c rans.c approx_exp.c approx_log.c symmetric.c drng.c auxfunc.c      \
    ntt/q61441n512/ntt_ref.c
INC := -Isrc/SHUTTLE-256/ntt/q61441n512 -Iauthor-tools
DEFS := -DSHUTTLE_MODE=256
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-23,SHUTTLE-256,sig,256,sig-23-shuttle-256,ngcc_ref_sig_23_SHUTTLE_256))

