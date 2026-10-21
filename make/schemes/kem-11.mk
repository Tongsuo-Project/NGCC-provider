# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# COMPASS-KEM / COMPASS-KEM-128 (128-bit)
ALGORITHM := COMPASS-KEM
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/COMPASS-KEM-128
SRCS := kem.c indcpa.c polyvec.c poly.c ntt.c cbd.c reduce.c verify.c drng.c auxfunc.c            \
    symmetric-shake.c KEM_AlgorithmInstance.c
INC := 
DEFS := -DSECURITY_LEVEL=128
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-11,COMPASS-KEM-128,kem,128,kem-11-compass-kem-128,ngcc_ref_kem_11_COMPASS_KEM_128))

# COMPASS-KEM / COMPASS-KEM-256 (256-bit)
ALGORITHM := COMPASS-KEM
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/COMPASS-KEM-256
SRCS := kem.c indcpa.c polyvec.c poly.c ntt.c cbd.c reduce.c verify.c drng.c auxfunc.c            \
    symmetric-shake.c KEM_AlgorithmInstance.c
INC := 
DEFS := -DSECURITY_LEVEL=256
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-11,COMPASS-KEM-256,kem,256,kem-11-compass-kem-256,ngcc_ref_kem_11_COMPASS_KEM_256))

