# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# HEP-QC / hep-qc-1 (128-bit)
ALGORITHM := HEP-QC
SRCDIR := Implementations/Implementations/ref/hep-qc-1
SRCS := ../KEM_HEP_QC.c ../PKE_HEP_QC.c ../../common/code.c ../../common/crypto_memset.c          \
    ../../common/fft.c ../../common/gf.c ../../common/gf2x.c ../../common/parsing.c               \
    ../../common/reed_muller.c ../../common/reed_solomon.c ../../common/symmetric.c               \
    ../../common/vector.c
INC := -IImplementations/Implementations/common -IImplementations/Implementations/common/hep-qc-1 \
    -IImplementations/Implementations/ref
DEFS := -DHEP_QC_ARCH_REF=1
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_HEP_QC.h"' -DNGCC_INSTANCE='"hep-qc-1"'
NOAUX := 
$(eval $(call ngcc_instance,kem-17,hep-qc-1,kem,128,kem-17-hep-qc-1,ngcc_ref_kem_17_hep_qc_1))

# HEP-QC / hep-qc-3 (192-bit)
ALGORITHM := HEP-QC
SRCDIR := Implementations/Implementations/ref/hep-qc-3
SRCS := ../KEM_HEP_QC.c ../PKE_HEP_QC.c ../../common/code.c ../../common/crypto_memset.c          \
    ../../common/fft.c ../../common/gf.c ../../common/gf2x.c ../../common/parsing.c               \
    ../../common/reed_muller.c ../../common/reed_solomon.c ../../common/symmetric.c               \
    ../../common/vector.c
INC := -IImplementations/Implementations/common -IImplementations/Implementations/common/hep-qc-3 \
    -IImplementations/Implementations/ref
DEFS := -DHEP_QC_ARCH_REF=1
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_HEP_QC.h"' -DNGCC_INSTANCE='"hep-qc-3"'
NOAUX := 
$(eval $(call ngcc_instance,kem-17,hep-qc-3,kem,192,kem-17-hep-qc-3,ngcc_ref_kem_17_hep_qc_3))

# HEP-QC / hep-qc-5 (256-bit)
ALGORITHM := HEP-QC
SRCDIR := Implementations/Implementations/ref/hep-qc-5
SRCS := ../KEM_HEP_QC.c ../PKE_HEP_QC.c ../../common/code.c ../../common/crypto_memset.c          \
    ../../common/fft.c ../../common/gf.c ../../common/gf2x.c ../../common/parsing.c               \
    ../../common/reed_muller.c ../../common/reed_solomon.c ../../common/symmetric.c               \
    ../../common/vector.c
INC := -IImplementations/Implementations/common -IImplementations/Implementations/common/hep-qc-5 \
    -IImplementations/Implementations/ref
DEFS := -DHEP_QC_ARCH_REF=1
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_HEP_QC.h"' -DNGCC_INSTANCE='"hep-qc-5"'
NOAUX := 
$(eval $(call ngcc_instance,kem-17,hep-qc-5,kem,256,kem-17-hep-qc-5,ngcc_ref_kem_17_hep_qc_5))

