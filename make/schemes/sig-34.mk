# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# YuanYang.DSA / yuanyang-1024 (256-bit)
ALGORITHM := YuanYang.DSA
SRCDIR := Implementations/Reference_Implementation/yuanyang-1024
SRCS := common.c prng.c fpr.c fft.c auxfunc.c drng.c ntt.c codec.c poly_inv_ntt.c                 \
    SIG_AlgorithmInstance.c sign.c sign_ntt.c vrfy.c sampler.c keygen/keygen.c keygen/pairgen.c   \
    keygen/perturbation.c keygen/fft.c keygen/security_loss.c keygen/ntrugen/src/ntrugen.c        \
    keygen/ntrugen/src/bigint.c keygen/ntrugen/src/bigintgcd.c keygen/ntrugen/src/cfft.c          \
    keygen/ntrugen/src/complex.c keygen/ntrugen/src/fft.c keygen/ntrugen/src/twiddles10.c
INC := -Isrc/yuanyang-1024/keygen/ntrugen/src
DEFS := -DNGCC_KATS
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-34,yuanyang-1024,sig,256,sig-34-yuanyang-1024,ngcc_ref_sig_34_yuanyang_1024))

# YuanYang.DSA / yuanyang-512 (128-bit)
ALGORITHM := YuanYang.DSA
SRCDIR := Implementations/Reference_Implementation/yuanyang-512
SRCS := common.c prng.c fpr.c fft.c auxfunc.c drng.c ntt.c codec.c poly_inv_ntt.c                 \
    SIG_AlgorithmInstance.c sign.c sign_ntt.c vrfy.c sampler.c keygen/keygen.c keygen/pairgen.c   \
    keygen/perturbation.c keygen/fft.c keygen/security_loss.c keygen/ntrugen/src/ntrugen.c        \
    keygen/ntrugen/src/bigint.c keygen/ntrugen/src/bigintgcd.c keygen/ntrugen/src/cfft.c          \
    keygen/ntrugen/src/complex.c keygen/ntrugen/src/fft.c keygen/ntrugen/src/twiddles9.c
INC := -Isrc/yuanyang-512/keygen/ntrugen/src
DEFS := -DNGCC_KATS
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-34,yuanyang-512,sig,128,sig-34-yuanyang-512,ngcc_ref_sig_34_yuanyang_512))

