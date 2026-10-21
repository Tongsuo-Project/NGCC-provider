# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# LoongKEM / Loong128 (128-bit)
ALGORITHM := LoongKEM
SRCDIR := Implementations/Reference_Implementation/Loong128
SRCS := KEM_Loong.c auxfunc.c drng.c poly.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_Loong.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-18,Loong128,kem,128,kem-18-loong128,ngcc_ref_kem_18_Loong128))

# LoongKEM / Loong256 (256-bit)
ALGORITHM := LoongKEM
SRCDIR := Implementations/Reference_Implementation/Loong256
SRCS := KEM_Loong.c auxfunc.c drng.c poly.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_Loong.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-18,Loong256,kem,256,kem-18-loong256,ngcc_ref_kem_18_Loong256))

