# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# CheetahKEM / Cheetah128 (128-bit)
ALGORITHM := CheetahKEM
SRCDIR := Implementations/Reference_Implementation/Cheetah128
SRCS := KEM_Cheetah.c auxfunc.c drng.c mod.c ntt.c poly.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_Cheetah.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-09,Cheetah128,kem,128,kem-09-cheetah128,ngcc_ref_kem_09_Cheetah128))

# CheetahKEM / Cheetah256 (256-bit)
ALGORITHM := CheetahKEM
SRCDIR := Implementations/Reference_Implementation/Cheetah256
SRCS := KEM_Cheetah.c auxfunc.c drng.c mod.c ntt.c poly.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_Cheetah.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-09,Cheetah256,kem,256,kem-09-cheetah256,ngcc_ref_kem_09_Cheetah256))

