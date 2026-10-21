# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# NSS-HQC / HQC-128 (128-bit)
ALGORITHM := NSS-HQC
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/HQC-128
SRCS := KEM_AlgorithmInstance.c auxfunc.c code_layer.c drng.c nss_hqc_core.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-26,HQC-128,kem,128,kem-26-hqc-128,ngcc_ref_kem_26_HQC_128))

# NSS-HQC / HQC-256 (256-bit)
ALGORITHM := NSS-HQC
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/HQC-256
SRCS := KEM_AlgorithmInstance.c auxfunc.c code_layer.c drng.c nss_hqc_core.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-26,HQC-256,kem,256,kem-26-hqc-256,ngcc_ref_kem_26_HQC_256))

