# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# OAEP-NTRU / OAEP-NTRU-1296 (256-bit)
ALGORITHM := OAEP-NTRU
SRCDIR := Implementations/Reference_Implementation/OAEP-NTRU-1296
SRCS := KEM_AlgorithmInstance.c auxfunc.c drng.c ntt.c poly.c symmetric.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-28,OAEP-NTRU-1296,kem,256,kem-28-oaep-ntru-1296,ngcc_ref_kem_28_OAEP_NTRU_1296))

# OAEP-NTRU / OAEP-NTRU-648 (128-bit)
ALGORITHM := OAEP-NTRU
SRCDIR := Implementations/Reference_Implementation/OAEP-NTRU-648
SRCS := KEM_AlgorithmInstance.c auxfunc.c drng.c ntt.c poly.c symmetric.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-28,OAEP-NTRU-648,kem,128,kem-28-oaep-ntru-648,ngcc_ref_kem_28_OAEP_NTRU_648))

