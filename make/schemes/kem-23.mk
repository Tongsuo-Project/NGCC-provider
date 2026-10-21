# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Mito / Mito-1-128 (128-bit)
ALGORITHM := Mito
SRCDIR := Implementations/Reference_Implementation/Mito-1-128
SRCS := KEM_AlgorithmInstance.c auxfunc.c code.c drng.c fft.c gf.c gf2x.c mito_kem.c mito_pke.c   \
    parsing.c reed_muller.c reed_solomon.c symmetric.c vector.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-23,Mito-1-128,kem,128,kem-23-mito-1-128,ngcc_ref_kem_23_Mito_1_128))

# Mito / Mito-1-256 (256-bit)
ALGORITHM := Mito
SRCDIR := Implementations/Reference_Implementation/Mito-1-256
SRCS := KEM_AlgorithmInstance.c auxfunc.c code.c drng.c fft.c gf.c gf2x.c mito_kem.c mito_pke.c   \
    parsing.c reed_muller.c reed_solomon.c symmetric.c vector.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-23,Mito-1-256,kem,256,kem-23-mito-1-256,ngcc_ref_kem_23_Mito_1_256))

# Mito / Mito-1-E-128 (128-bit)
ALGORITHM := Mito
SRCDIR := Implementations/Reference_Implementation/Mito-1-E-128
SRCS := KEM_AlgorithmInstance.c auxfunc.c code.c drng.c fft.c gf.c gf2x.c mito_kem.c mito_pke.c   \
    parsing.c reed_muller.c reed_solomon.c symmetric.c vector.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-23,Mito-1-E-128,kem,128,kem-23-mito-1-e-128,ngcc_ref_kem_23_Mito_1_E_128))

# Mito / Mito-1-E-256 (256-bit)
ALGORITHM := Mito
SRCDIR := Implementations/Reference_Implementation/Mito-1-E-256
SRCS := KEM_AlgorithmInstance.c auxfunc.c code.c drng.c fft.c gf.c gf2x.c mito_kem.c mito_pke.c   \
    parsing.c reed_muller.c reed_solomon.c symmetric.c vector.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-23,Mito-1-E-256,kem,256,kem-23-mito-1-e-256,ngcc_ref_kem_23_Mito_1_E_256))

# Mito / Mito-2-E-128 (128-bit)
ALGORITHM := Mito
SRCDIR := Implementations/Reference_Implementation/Mito-2-E-128
SRCS := KEM_AlgorithmInstance.c auxfunc.c code.c drng.c fft.c gf.c gf2x.c mito_kem.c mito_pke.c   \
    parsing.c reed_muller.c reed_solomon.c symmetric.c vector.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-23,Mito-2-E-128,kem,128,kem-23-mito-2-e-128,ngcc_ref_kem_23_Mito_2_E_128))

# Mito / Mito-2-E-256 (256-bit)
ALGORITHM := Mito
SRCDIR := Implementations/Reference_Implementation/Mito-2-E-256
SRCS := KEM_AlgorithmInstance.c auxfunc.c code.c drng.c fft.c gf.c gf2x.c mito_kem.c mito_pke.c   \
    parsing.c reed_muller.c reed_solomon.c symmetric.c vector.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-23,Mito-2-E-256,kem,256,kem-23-mito-2-e-256,ngcc_ref_kem_23_Mito_2_E_256))

