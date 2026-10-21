# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# DTRU / DTRU-1024 (256-bit)
ALGORITHM := DTRU
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/DTRU-1024
SRCS := KEM_AlgorithmInstance.c auxfunc.c cbd.c coding.c drng.c dtru.c inverse.c ntt.c pack.c     \
    poly.c reduce.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-14,DTRU-1024,kem,256,kem-14-dtru-1024,ngcc_ref_kem_14_DTRU_1024))

# DTRU / DTRU-1024-PACK_PK (256-bit)
ALGORITHM := DTRU
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/DTRU-1024
SRCS := KEM_AlgorithmInstance.c auxfunc.c cbd.c coding.c drng.c dtru.c inverse.c ntt.c pack.c     \
    poly.c reduce.c
INC := 
DEFS := -DPK_PACK_OPT=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE='"DTRU-1024-PACK_PK"'
NOAUX := 
$(eval $(call ngcc_instance,kem-14,DTRU-1024-PACK_PK,kem,256,kem-14-dtru-1024-pack-pk,ngcc_ref_kem_14_DTRU_1024_PACK_PK))

# DTRU / DTRU-648 (128-bit)
ALGORITHM := DTRU
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/DTRU-648
SRCS := KEM_AlgorithmInstance.c auxfunc.c cbd.c coding.c drng.c dtru.c inverse.c ntt.c pack.c     \
    poly.c reduce.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-14,DTRU-648,kem,128,kem-14-dtru-648,ngcc_ref_kem_14_DTRU_648))

# DTRU / DTRU-648-PACK_PK (128-bit)
ALGORITHM := DTRU
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/DTRU-648
SRCS := KEM_AlgorithmInstance.c auxfunc.c cbd.c coding.c drng.c dtru.c inverse.c ntt.c pack.c     \
    poly.c reduce.c
INC := 
DEFS := -DPK_PACK_OPT=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE='"DTRU-648-PACK_PK"'
NOAUX := 
$(eval $(call ngcc_instance,kem-14,DTRU-648-PACK_PK,kem,128,kem-14-dtru-648-pack-pk,ngcc_ref_kem_14_DTRU_648_PACK_PK))

# DTRU / DTRU-768 (192-bit)
ALGORITHM := DTRU
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/DTRU-768
SRCS := KEM_AlgorithmInstance.c auxfunc.c cbd.c coding.c drng.c dtru.c inverse.c ntt.c pack.c     \
    poly.c reduce.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-14,DTRU-768,kem,192,kem-14-dtru-768,ngcc_ref_kem_14_DTRU_768))

# DTRU / DTRU-768-PACK_PK (192-bit)
ALGORITHM := DTRU
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/DTRU-768
SRCS := KEM_AlgorithmInstance.c auxfunc.c cbd.c coding.c drng.c dtru.c inverse.c ntt.c pack.c     \
    poly.c reduce.c
INC := 
DEFS := -DPK_PACK_OPT=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE='"DTRU-768-PACK_PK"'
NOAUX := 
$(eval $(call ngcc_instance,kem-14,DTRU-768-PACK_PK,kem,192,kem-14-dtru-768-pack-pk,ngcc_ref_kem_14_DTRU_768_PACK_PK))

# DTRU / DTRU-Light (128-bit)
ALGORITHM := DTRU
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/DTRU-Light
SRCS := KEM_AlgorithmInstance.c auxfunc.c cbd.c coding.c drng.c dtru.c inverse.c ntt.c pack.c     \
    poly.c reduce.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-14,DTRU-Light,kem,128,kem-14-dtru-light,ngcc_ref_kem_14_DTRU_Light))

# DTRU / DTRU-Prime (128-bit)
ALGORITHM := DTRU
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/DTRU-Prime
SRCS := KEM_AlgorithmInstance.c auxfunc.c cbd.c coding.c drng.c dtru.c inverse.c pack.c poly.c    \
    radix_ntt_n1087.c reduce.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-14,DTRU-Prime,kem,128,kem-14-dtru-prime,ngcc_ref_kem_14_DTRU_Prime))

