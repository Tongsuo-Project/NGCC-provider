# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# MORNING-Scabbard / scabbard128 (128-bit)
ALGORITHM := MORNING-Scabbard
SRCDIR := Implementations/Reference_Implementation/scabbard128
SRCS := auxfunc.c cbd.c drng.c indcpa.c KEM_scabbard128.c packing.c poly.c polyvec.c verify.c minal.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_scabbard128.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-24,scabbard128,kem,128,kem-24-scabbard128,ngcc_ref_kem_24_scabbard128))

# MORNING-Scabbard / scabbard256 (256-bit)
ALGORITHM := MORNING-Scabbard
SRCDIR := Implementations/Reference_Implementation/scabbard256
SRCS := auxfunc.c cbd.c drng.c indcpa.c KEM_scabbard256.c packing.c poly.c polyvec.c verify.c minal.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_scabbard256.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-24,scabbard256,kem,256,kem-24-scabbard256,ngcc_ref_kem_24_scabbard256))

