# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Scloud+ / Scloudplus-128-AES-packed10 (128-bit)
ALGORITHM := Scloud+
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Scloudplus-128/kem
SRCS := KEM_AlgorithmInstance.c ../../../_shared/scloudplus_core/common/encode.c                  \
    ../../../_shared/scloudplus_core/common/kem.c ../../../_shared/scloudplus_core/common/pke.c   \
    ../../../_shared/scloudplus_core/common/sample.c                                              \
    ../../../_shared/scloudplus_core/common/util.c                                                \
    ../../../_shared/scloudplus_core/ref/matrix_reference.c                                       \
    ../../../_shared/scloudplus_core/ref/pack_reference.c                                         \
    ../../../_shared/scloudplus_core/common/hash_aes_shake.c                                      \
    ../../../_shared/scloudplus_core/ref/aes_reference.c ../../../_shared/api_pkc/kat_random.c    \
    ../../../_shared/api_pkc/drng.c
INC := -Isrc/Scloudplus-128-AES-packed10/../../../_shared/api_pkc                                 \
    -Isrc/Scloudplus-128-AES-packed10/../../../_shared/scloudplus_core/include                    \
    -Isrc/Scloudplus-128-AES-packed10/../../../_shared/scloudplus_core/common
DEFS := -DSCLOUDPLUS_FAMILY_AES -DSCLOUDPLUS_TIER_REFERENCE -DSCLOUDPLUS_REF_FAMILY_AES
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 1
$(eval $(call ngcc_instance,kem-35,Scloudplus-128-AES-packed10,kem,128,kem-35-scloudplus-128-aes-packed10,ngcc_ref_kem_35_Scloudplus_128_AES_packed10))

# Scloud+ / Scloudplus-128-SM3-packed10 (128-bit)
ALGORITHM := Scloud+
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Scloudplus-128/kem
SRCS := KEM_AlgorithmInstance.c ../../../_shared/scloudplus_core/common/encode.c                  \
    ../../../_shared/scloudplus_core/common/kem.c ../../../_shared/scloudplus_core/common/pke.c   \
    ../../../_shared/scloudplus_core/common/sample.c                                              \
    ../../../_shared/scloudplus_core/common/util.c                                                \
    ../../../_shared/scloudplus_core/ref/matrix_reference.c                                       \
    ../../../_shared/scloudplus_core/ref/pack_reference.c                                         \
    ../../../_shared/scloudplus_core/common/hash_sm3_portable.c                                   \
    ../../../_shared/scloudplus_core/ref/sm3_reference.c ../../../_shared/api_pkc/kat_random.c    \
    ../../../_shared/api_pkc/drng.c
INC := -Isrc/Scloudplus-128-SM3-packed10/../../../_shared/api_pkc                                 \
    -Isrc/Scloudplus-128-SM3-packed10/../../../_shared/scloudplus_core/include                    \
    -Isrc/Scloudplus-128-SM3-packed10/../../../_shared/scloudplus_core/common
DEFS := -DSCLOUDPLUS_FAMILY_SM3 -DSCLOUDPLUS_TIER_REFERENCE -DSCLOUDPLUS_REF_FAMILY_SM3
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 1
$(eval $(call ngcc_instance,kem-35,Scloudplus-128-SM3-packed10,kem,128,kem-35-scloudplus-128-sm3-packed10,ngcc_ref_kem_35_Scloudplus_128_SM3_packed10))

# Scloud+ / Scloudplus-192-AES-packed10 (192-bit)
ALGORITHM := Scloud+
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Scloudplus-192/kem
SRCS := KEM_AlgorithmInstance.c ../../../_shared/scloudplus_core/common/encode.c                  \
    ../../../_shared/scloudplus_core/common/kem.c ../../../_shared/scloudplus_core/common/pke.c   \
    ../../../_shared/scloudplus_core/common/sample.c                                              \
    ../../../_shared/scloudplus_core/common/util.c                                                \
    ../../../_shared/scloudplus_core/ref/matrix_reference.c                                       \
    ../../../_shared/scloudplus_core/ref/pack_reference.c                                         \
    ../../../_shared/scloudplus_core/common/hash_aes_shake.c                                      \
    ../../../_shared/scloudplus_core/ref/aes_reference.c ../../../_shared/api_pkc/kat_random.c    \
    ../../../_shared/api_pkc/drng.c
INC := -Isrc/Scloudplus-192-AES-packed10/../../../_shared/api_pkc                                 \
    -Isrc/Scloudplus-192-AES-packed10/../../../_shared/scloudplus_core/include                    \
    -Isrc/Scloudplus-192-AES-packed10/../../../_shared/scloudplus_core/common
DEFS := -DSCLOUDPLUS_FAMILY_AES -DSCLOUDPLUS_TIER_REFERENCE -DSCLOUDPLUS_REF_FAMILY_AES
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 1
$(eval $(call ngcc_instance,kem-35,Scloudplus-192-AES-packed10,kem,192,kem-35-scloudplus-192-aes-packed10,ngcc_ref_kem_35_Scloudplus_192_AES_packed10))

# Scloud+ / Scloudplus-192-SM3-packed10 (192-bit)
ALGORITHM := Scloud+
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Scloudplus-192/kem
SRCS := KEM_AlgorithmInstance.c ../../../_shared/scloudplus_core/common/encode.c                  \
    ../../../_shared/scloudplus_core/common/kem.c ../../../_shared/scloudplus_core/common/pke.c   \
    ../../../_shared/scloudplus_core/common/sample.c                                              \
    ../../../_shared/scloudplus_core/common/util.c                                                \
    ../../../_shared/scloudplus_core/ref/matrix_reference.c                                       \
    ../../../_shared/scloudplus_core/ref/pack_reference.c                                         \
    ../../../_shared/scloudplus_core/common/hash_sm3_portable.c                                   \
    ../../../_shared/scloudplus_core/ref/sm3_reference.c ../../../_shared/api_pkc/kat_random.c    \
    ../../../_shared/api_pkc/drng.c
INC := -Isrc/Scloudplus-192-SM3-packed10/../../../_shared/api_pkc                                 \
    -Isrc/Scloudplus-192-SM3-packed10/../../../_shared/scloudplus_core/include                    \
    -Isrc/Scloudplus-192-SM3-packed10/../../../_shared/scloudplus_core/common
DEFS := -DSCLOUDPLUS_FAMILY_SM3 -DSCLOUDPLUS_TIER_REFERENCE -DSCLOUDPLUS_REF_FAMILY_SM3
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 1
$(eval $(call ngcc_instance,kem-35,Scloudplus-192-SM3-packed10,kem,192,kem-35-scloudplus-192-sm3-packed10,ngcc_ref_kem_35_Scloudplus_192_SM3_packed10))

# Scloud+ / Scloudplus-256-AES-packed10 (256-bit)
ALGORITHM := Scloud+
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Scloudplus-256/kem
SRCS := KEM_AlgorithmInstance.c ../../../_shared/scloudplus_core/common/encode.c                  \
    ../../../_shared/scloudplus_core/common/kem.c ../../../_shared/scloudplus_core/common/pke.c   \
    ../../../_shared/scloudplus_core/common/sample.c                                              \
    ../../../_shared/scloudplus_core/common/util.c                                                \
    ../../../_shared/scloudplus_core/ref/matrix_reference.c                                       \
    ../../../_shared/scloudplus_core/ref/pack_reference.c                                         \
    ../../../_shared/scloudplus_core/common/hash_aes_shake.c                                      \
    ../../../_shared/scloudplus_core/ref/aes_reference.c ../../../_shared/api_pkc/kat_random.c    \
    ../../../_shared/api_pkc/drng.c
INC := -Isrc/Scloudplus-256-AES-packed10/../../../_shared/api_pkc                                 \
    -Isrc/Scloudplus-256-AES-packed10/../../../_shared/scloudplus_core/include                    \
    -Isrc/Scloudplus-256-AES-packed10/../../../_shared/scloudplus_core/common
DEFS := -DSCLOUDPLUS_FAMILY_AES -DSCLOUDPLUS_TIER_REFERENCE -DSCLOUDPLUS_REF_FAMILY_AES
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 1
$(eval $(call ngcc_instance,kem-35,Scloudplus-256-AES-packed10,kem,256,kem-35-scloudplus-256-aes-packed10,ngcc_ref_kem_35_Scloudplus_256_AES_packed10))

# Scloud+ / Scloudplus-256-SM3-packed10 (256-bit)
ALGORITHM := Scloud+
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Scloudplus-256/kem
SRCS := KEM_AlgorithmInstance.c ../../../_shared/scloudplus_core/common/encode.c                  \
    ../../../_shared/scloudplus_core/common/kem.c ../../../_shared/scloudplus_core/common/pke.c   \
    ../../../_shared/scloudplus_core/common/sample.c                                              \
    ../../../_shared/scloudplus_core/common/util.c                                                \
    ../../../_shared/scloudplus_core/ref/matrix_reference.c                                       \
    ../../../_shared/scloudplus_core/ref/pack_reference.c                                         \
    ../../../_shared/scloudplus_core/common/hash_sm3_portable.c                                   \
    ../../../_shared/scloudplus_core/ref/sm3_reference.c ../../../_shared/api_pkc/kat_random.c    \
    ../../../_shared/api_pkc/drng.c
INC := -Isrc/Scloudplus-256-SM3-packed10/../../../_shared/api_pkc                                 \
    -Isrc/Scloudplus-256-SM3-packed10/../../../_shared/scloudplus_core/include                    \
    -Isrc/Scloudplus-256-SM3-packed10/../../../_shared/scloudplus_core/common
DEFS := -DSCLOUDPLUS_FAMILY_SM3 -DSCLOUDPLUS_TIER_REFERENCE -DSCLOUDPLUS_REF_FAMILY_SM3
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 1
$(eval $(call ngcc_instance,kem-35,Scloudplus-256-SM3-packed10,kem,256,kem-35-scloudplus-256-sm3-packed10,ngcc_ref_kem_35_Scloudplus_256_SM3_packed10))

