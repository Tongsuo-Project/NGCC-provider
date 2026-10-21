# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# COMPASS-SIG / COMPASS-SIG-128 (128-bit)
ALGORITHM := COMPASS-SIG
SRCDIR := Implementations/Reference_Implementation/COMPASS-SIG-128
SRCS := sign.c packing.c polyvec.c poly.c ntt.c reduce.c rounding.c symmetric-shake.c drng.c      \
    auxfunc.c SIG_AlgorithmInstance.c
INC := 
DEFS := -DCOMPASS_SIG_MODE=128
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-06,COMPASS-SIG-128,sig,128,sig-06-compass-sig-128,ngcc_ref_sig_06_COMPASS_SIG_128))

# COMPASS-SIG / COMPASS-SIG-256 (256-bit)
ALGORITHM := COMPASS-SIG
SRCDIR := Implementations/Reference_Implementation/COMPASS-SIG-256
SRCS := sign.c packing.c polyvec.c poly.c ntt.c reduce.c rounding.c symmetric-shake.c drng.c      \
    auxfunc.c SIG_AlgorithmInstance.c
INC := 
DEFS := -DCOMPASS_SIG_MODE=256
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-06,COMPASS-SIG-256,sig,256,sig-06-compass-sig-256,ngcc_ref_sig_06_COMPASS_SIG_256))

