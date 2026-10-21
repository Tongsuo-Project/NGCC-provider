# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Mithril / Mithril-128 (128-bit)
ALGORITHM := Mithril
SRCDIR := Implementations/Reference_Implementation/Mithril-128
SRCS := arith/poly.c arith/ring.c arith/packing.c arith/uniform.c pke.c kem.c utils/auxfunc.c     \
    utils/drng.c
INC := -Isrc/Mithril-128/utils -Isrc/Mithril-128/arith
DEFS := -DRRLWR_SECURITY_LEVEL=128 -DNDEBUG
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"kem.h"' -DNGCC_INSTANCE='"Mithril-128"'
NOAUX := 
$(eval $(call ngcc_instance,kem-22,Mithril-128,kem,128,kem-22-mithril-128,ngcc_ref_kem_22_Mithril_128))

# Mithril / Mithril-256 (256-bit)
ALGORITHM := Mithril
SRCDIR := Implementations/Reference_Implementation/Mithril-256
SRCS := arith/poly.c arith/ring.c arith/packing.c arith/uniform.c pke.c kem.c utils/auxfunc.c     \
    utils/drng.c
INC := -Isrc/Mithril-256/utils -Isrc/Mithril-256/arith
DEFS := -DRRLWR_SECURITY_LEVEL=256 -DNDEBUG
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"kem.h"' -DNGCC_INSTANCE='"Mithril-256"'
NOAUX := 
$(eval $(call ngcc_instance,kem-22,Mithril-256,kem,256,kem-22-mithril-256,ngcc_ref_kem_22_Mithril_256))

