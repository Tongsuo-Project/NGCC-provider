# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Rudraksh2 / lwekem128 (128-bit)
ALGORITHM := Rudraksh2
SRCDIR := Implementations/Reference_Implementation/lwekem128
SRCS := KEM_lwekem128.c indcpa.c poly.c polyvec.c cbd.c minal.c reduce.c ntt.c verify.c auxfunc.c \
    drng.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_lwekem128.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-34,lwekem128,kem,128,kem-rudraksh2-128,ngcc_ref_kem_34_lwekem128))

# Rudraksh2 / lwekem256 (256-bit)
ALGORITHM := Rudraksh2
SRCDIR := Implementations/Reference_Implementation/lwekem256
SRCS := KEM_lwekem256.c indcpa.c poly.c polyvec.c cbd.c minal.c reduce.c ntt.c verify.c auxfunc.c \
    drng.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_lwekem256.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-34,lwekem256,kem,256,kem-rudraksh2-256,ngcc_ref_kem_34_lwekem256))

