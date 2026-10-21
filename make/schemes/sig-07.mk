# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# CS / CS-128 (128-bit)
ALGORITHM := CS
SRCDIR := Implementations/Reference_Implementation/CS-128
SRCS := SIG_AlgorithmInstance.c auxfunc.c bits-hints.c conversion.c cs.c drng.c encodings.c ntt.c \
    sampling.c
INC :=
DEFS :=
INSTANCE_CFLAGS := -fcommon -w -Imake/compat/sign-07
SHIMDEFS := -DNGCC_INSTANCE_HEADER_NONE -DNGCC_INSTANCE='"CS_128"' -DNGCC_NO_DRNG
NOAUX := 
$(eval $(call ngcc_instance,sign-07,CS-128,sig,128,sig-07-cs-128,ngcc_ref_sig_07_CS_128))

# CS / CS-256 (256-bit)
ALGORITHM := CS
SRCDIR := Implementations/Reference_Implementation/CS-256
SRCS := SIG_AlgorithmInstance.c auxfunc.c bits-hints.c conversion.c cs.c drng.c encodings.c ntt.c \
    sampling.c
INC :=
DEFS :=
INSTANCE_CFLAGS := -fcommon -w -Imake/compat/sign-07
SHIMDEFS := -DNGCC_INSTANCE_HEADER_NONE -DNGCC_INSTANCE='"CS_256"' -DNGCC_NO_DRNG
NOAUX := 
$(eval $(call ngcc_instance,sign-07,CS-256,sig,256,sig-07-cs-256,ngcc_ref_sig_07_CS_256))
