# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Octarine / Octarine-128 (128-bit)
ALGORITHM := Octarine
SRCDIR := Implementations/Reference_Implementation/Octarine-128
SRCS := arith/fprime.c arith/poly.c arith/ring.c arith/packing.c arith/uniform.c sign_ring.c      \
    sign.c crt.c utils/auxfunc.c utils/drng.c
INC := -Isrc/Octarine-128/arith -Isrc/Octarine-128/utils
DEFS := -DRRLWR_SECURITY_LEVEL=128
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sign.h"' -DNGCC_INSTANCE='"Octarine-128"'
NOAUX := 
$(eval $(call ngcc_instance,sign-16,Octarine-128,sig,128,sig-16-octarine-128,ngcc_ref_sig_16_Octarine_128))

# Octarine / Octarine-256 (256-bit)
ALGORITHM := Octarine
SRCDIR := Implementations/Reference_Implementation/Octarine-256
SRCS := arith/fprime.c arith/poly.c arith/ring.c arith/packing.c arith/uniform.c sign_ring.c      \
    sign.c crt.c utils/auxfunc.c utils/drng.c
INC := -Isrc/Octarine-256/arith -Isrc/Octarine-256/utils
DEFS := -DRRLWR_SECURITY_LEVEL=256
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sign.h"' -DNGCC_INSTANCE='"Octarine-256"'
NOAUX := 
$(eval $(call ngcc_instance,sign-16,Octarine-256,sig,256,sig-16-octarine-256,ngcc_ref_sig_16_Octarine_256))

