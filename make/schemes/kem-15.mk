# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# FLIT / FLIT128_REF (128-bit)
ALGORITHM := FLIT
SRCDIR := Implementations/Reference_Implementation/FLIT128
SRCS := kem.c ntt.c poly.c decode.c packing.c reduce.c verify.c indcpa.c auxfunc.c drng.c         \
    KEM_AlgorithmInstance.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-15,FLIT128_REF,kem,128,kem-15-flit128-ref,ngcc_ref_kem_15_FLIT128_REF))

# FLIT / FLIT256_REF (256-bit)
ALGORITHM := FLIT
SRCDIR := Implementations/Reference_Implementation/FLIT256
SRCS := kem.c ntt.c poly.c decode.c packing.c reduce.c verify.c indcpa.c auxfunc.c drng.c         \
    KEM_AlgorithmInstance.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-15,FLIT256_REF,kem,256,kem-15-flit256-ref,ngcc_ref_kem_15_FLIT256_REF))

