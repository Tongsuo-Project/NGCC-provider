# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# BiT / BiT-128 (128-bit)
ALGORITHM := BiT
SRCDIR := Implementations/Reference_Implementation/BiT-128
SRCS := SIG_AlgorithmInstance.c auxfunc.c drng.c fips202.c ntt.c packing.c parabola.c spectral.c  \
    poly.c polyvec.c sample.c sign.c symmetric.c
INC := 
DEFS := -DBIT_USE_SHAKE=0
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-02,BiT-128,sig,128,sig-02-bit-128,ngcc_ref_sig_02_BiT_128))

# BiT / BiT-256 (256-bit)
ALGORITHM := BiT
SRCDIR := Implementations/Reference_Implementation/BiT-256
SRCS := SIG_AlgorithmInstance.c auxfunc.c drng.c fips202.c ntt.c packing.c parabola.c spectral.c  \
    poly.c polyvec.c sample.c sign.c symmetric.c
INC := 
DEFS := -DBIT_USE_SHAKE=0
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-02,BiT-256,sig,256,sig-02-bit-256,ngcc_ref_sig_02_BiT_256))

