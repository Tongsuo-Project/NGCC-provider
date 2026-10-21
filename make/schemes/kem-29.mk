# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Polar-KEM / PolarKEM-128 (128-bit)
ALGORITHM := Polar-KEM
SRCDIR := Submission_Package/Implementations/Reference_Implementation/PolarKEM-128
SRCS := KEM_AlgorithmInstance.c auxfunc.c drng.c polarkem_core.c polarkem_ct.c polarkem_pack.c    \
    polarkem_polar.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-29,PolarKEM-128,kem,128,kem-29-polarkem-128,ngcc_ref_kem_29_PolarKEM_128))

# Polar-KEM / PolarKEM-256 (256-bit)
ALGORITHM := Polar-KEM
SRCDIR := Submission_Package/Implementations/Reference_Implementation/PolarKEM-256
SRCS := KEM_AlgorithmInstance.c auxfunc.c drng.c polarkem_core.c polarkem_ct.c polarkem_pack.c    \
    polarkem_polar.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-29,PolarKEM-256,kem,256,kem-29-polarkem-256,ngcc_ref_kem_29_PolarKEM_256))

