# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# ZEN / ZEN_128 (128-bit)
ALGORITHM := ZEN
SRCDIR := Implementations/Reference_Implementation/ZEN_128
SRCS := auxfunc.c drng.c sample.c ntt.c poly.c symmetric.c pke.c KEM_AlgorithmInstance.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-41,ZEN_128,kem,128,kem-zen-128,ngcc_ref_kem_41_ZEN_128))

# ZEN / ZEN_256 (256-bit)
ALGORITHM := ZEN
SRCDIR := Implementations/Reference_Implementation/ZEN_256
SRCS := auxfunc.c drng.c sample.c ntt.c poly.c symmetric.c pke.c KEM_AlgorithmInstance.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-41,ZEN_256,kem,256,kem-zen-256,ngcc_ref_kem_41_ZEN_256))

