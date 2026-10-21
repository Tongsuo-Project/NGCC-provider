# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Weaver / WeaverKEM-128 (128-bit)
ALGORITHM := Weaver
SRCDIR := Implementations/Reference_Implementation/WeaverKEM-128
SRCS := drng.c auxfunc.c symmetric-iccs.c kem_cca.c indcpa.c polyvec.c poly.c ntt.c cbd.c         \
    reduce.c verify.c msgenc.c bch_high.c bch_low.c poly_invq.c KEM_WeaverKEM-128.c
INC := 
DEFS := -DWEAVER_MODE=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_WeaverKEM-128.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-39,WeaverKEM-128,kem,128,kem-39-weaverkem-128,ngcc_ref_kem_39_WeaverKEM_128))

# Weaver / WeaverKEM-256 (256-bit)
ALGORITHM := Weaver
SRCDIR := Implementations/Reference_Implementation/WeaverKEM-256
SRCS := drng.c auxfunc.c symmetric-iccs.c kem_cca.c indcpa.c polyvec.c poly.c ntt.c cbd.c         \
    reduce.c verify.c msgenc.c bch_high.c bch_low.c poly_invq.c KEM_WeaverKEM-256.c
INC := 
DEFS := -DWEAVER_MODE=3
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_WeaverKEM-256.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-39,WeaverKEM-256,kem,256,kem-39-weaverkem-256,ngcc_ref_kem_39_WeaverKEM_256))

