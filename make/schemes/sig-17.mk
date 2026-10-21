# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# OPS Digital Signature Algorithm / OPSsig-128 (128-bit)
ALGORITHM := OPS Digital Signature Algorithm
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/OPSsig-128
SRCS := sign.c mult.c packing.c polyvec.c poly.c ntt.c reduce.c rounding.c auxfunc.c drng.c       \
    SIG_AlgorithmInstance.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-17,OPSsig-128,sig,128,sig-17-opssig-128,ngcc_ref_sig_17_OPSsig_128))

# OPS Digital Signature Algorithm / OPSsig-256 (256-bit)
ALGORITHM := OPS Digital Signature Algorithm
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/OPSsig-256
SRCS := sign.c mult.c packing.c polyvec.c poly.c ntt.c reduce.c rounding.c auxfunc.c drng.c       \
    SIG_AlgorithmInstance.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-17,OPSsig-256,sig,256,sig-17-opssig-256,ngcc_ref_sig_17_OPSsig_256))

