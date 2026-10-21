# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# DARTS / DARTS128 (128-bit)
ALGORITHM := DARTS
SRCDIR := Implementations/Reference_Implementation/DARTS128
SRCS := sign.c packing.c polyvec.c poly.c ntt.c reduce.c sampler.c encoding.c polyfix.c polymat.c \
    fft.c fixpoint.c symmetric.c drng.c auxfunc.c SIG_AlgorithmInstance.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_NO_DRNG
NOAUX := 
$(eval $(call ngcc_instance,sign-08,DARTS128,sig,128,sig-08-darts128,ngcc_ref_sig_08_DARTS128))

# DARTS / DARTS256 (256-bit)
ALGORITHM := DARTS
SRCDIR := Implementations/Reference_Implementation/DARTS256
SRCS := sign.c packing.c polyvec.c poly.c ntt.c reduce.c sampler.c encoding.c polyfix.c polymat.c \
    fft.c fixpoint.c symmetric.c drng.c auxfunc.c SIG_AlgorithmInstance.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_NO_DRNG
NOAUX := 
$(eval $(call ngcc_instance,sign-08,DARTS256,sig,256,sig-08-darts256,ngcc_ref_sig_08_DARTS256))

