# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# HARE / HARE-128-kr (128-bit)
ALGORITHM := HARE
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/HARE-128/kr
SRCS := KEM_AlgorithmInstance.c ../../../_shared/hare_core/common/code.c                          \
    ../../../_shared/hare_core/common/crypto_memset.c ../../../_shared/hare_core/common/fft.c     \
    ../../../_shared/hare_core/common/kem.c                                                       \
    ../../../_shared/hare_core/common/parameter_contract.c                                        \
    ../../../_shared/hare_core/common/symmetric_api_pkc.c                                         \
    ../../../_shared/hare_core/ref/compression_kr.c ../../../_shared/hare_core/ref/gf2x.c         \
    ../../../_shared/hare_core/ref/gf.c ../../../_shared/hare_core/ref/hqc.c                      \
    ../../../_shared/hare_core/ref/parsing.c ../../../_shared/hare_core/ref/reed_muller.c         \
    ../../../_shared/hare_core/ref/reed_solomon.c ../../../_shared/hare_core/ref/vector.c
INC := -Isrc/HARE-128-kr/../../../_shared/hare_core/common                                        \
    -Isrc/HARE-128-kr/../../../_shared/hare_core/ref                                              \
    -Isrc/HARE-128-kr/../../../_shared/hare_core/x86_64/avx256                                    \
    -Isrc/HARE-128-kr/../../../_shared/api_pkc
DEFS := -DHARE_SYMMETRIC_MODE_B -DHARE_BACKEND_KR=1
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-16,HARE-128-kr,kem,128,kem-16-hare-128-kr,ngcc_ref_kem_16_HARE_128_kr))

# HARE / HARE-256-kr (256-bit)
ALGORITHM := HARE
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/HARE-256/kr
SRCS := KEM_AlgorithmInstance.c ../../../_shared/hare_core/common/code.c                          \
    ../../../_shared/hare_core/common/crypto_memset.c ../../../_shared/hare_core/common/fft.c     \
    ../../../_shared/hare_core/common/kem.c                                                       \
    ../../../_shared/hare_core/common/parameter_contract.c                                        \
    ../../../_shared/hare_core/common/symmetric_api_pkc.c                                         \
    ../../../_shared/hare_core/ref/compression_kr.c ../../../_shared/hare_core/ref/gf2x.c         \
    ../../../_shared/hare_core/ref/gf.c ../../../_shared/hare_core/ref/hqc.c                      \
    ../../../_shared/hare_core/ref/parsing.c ../../../_shared/hare_core/ref/reed_muller.c         \
    ../../../_shared/hare_core/ref/reed_solomon.c ../../../_shared/hare_core/ref/vector.c
INC := -Isrc/HARE-256-kr/../../../_shared/hare_core/common                                        \
    -Isrc/HARE-256-kr/../../../_shared/hare_core/ref                                              \
    -Isrc/HARE-256-kr/../../../_shared/hare_core/x86_64/avx256                                    \
    -Isrc/HARE-256-kr/../../../_shared/api_pkc
DEFS := -DHARE_SYMMETRIC_MODE_B -DHARE_BACKEND_KR=1
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-16,HARE-256-kr,kem,256,kem-16-hare-256-kr,ngcc_ref_kem_16_HARE_256_kr))

