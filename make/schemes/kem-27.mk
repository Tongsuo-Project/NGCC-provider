# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# NTRE / NTRE-128 (128-bit)
ALGORITHM := NTRE
SRCDIR := Implementations/Reference_Implementation/NTRE-128
SRCS := src/poly.c src/ntt.c src/symmetric.c KEM_AlgorithmInstance.c auxfunc.c drng.c
INC := -Isrc/NTRE-128/src
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-27,NTRE-128,kem,128,kem-27-ntre-128,ngcc_ref_kem_27_NTRE_128))

# NTRE / NTRE-256 (256-bit)
ALGORITHM := NTRE
SRCDIR := Implementations/Reference_Implementation/NTRE-256
SRCS := src/poly.c src/ntt.c src/symmetric.c KEM_AlgorithmInstance.c auxfunc.c drng.c
INC := -Isrc/NTRE-256/src
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-27,NTRE-256,kem,256,kem-27-ntre-256,ngcc_ref_kem_27_NTRE_256))

