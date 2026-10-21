# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# MAMBA-Frost / MAMBA-Frost-128 (128-bit)
ALGORITHM := MAMBA-Frost
SRCDIR := Implementations/Reference_Implementation/MAMBA-Frost-128
SRCS := Frost/src/frost128.c Frost/src/util.c common/sha3/fips202.c common/aes/aes_c.c            \
    KEM_AlgorithmInstance.c randombytes_adapter.c drng.c auxfunc.c
INC := -Isrc/MAMBA-Frost-128/Frost/src -Isrc/MAMBA-Frost-128/common/sha3                          \
    -Isrc/MAMBA-Frost-128/common/aes -Isrc/MAMBA-Frost-128/common/random
DEFS := -DNIX -D_AMD64_ -D_REFERENCE_ -D_AES128_FOR_A_ -DFROST_USE_E8_CODE
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-20,MAMBA-Frost-128,kem,128,kem-20-mamba-frost-128,ngcc_ref_kem_20_MAMBA_Frost_128))

# MAMBA-Frost / MAMBA-Frost-192 (192-bit)
ALGORITHM := MAMBA-Frost
SRCDIR := Implementations/Reference_Implementation/MAMBA-Frost-192
SRCS := Frost/src/frost192.c Frost/src/util.c common/sha3/fips202.c common/aes/aes_c.c            \
    KEM_AlgorithmInstance.c randombytes_adapter.c drng.c auxfunc.c
INC := -Isrc/MAMBA-Frost-192/Frost/src -Isrc/MAMBA-Frost-192/common/sha3                          \
    -Isrc/MAMBA-Frost-192/common/aes -Isrc/MAMBA-Frost-192/common/random
DEFS := -DNIX -D_AMD64_ -D_REFERENCE_ -D_AES128_FOR_A_ -DFROST_USE_E8_CODE
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-20,MAMBA-Frost-192,kem,192,kem-20-mamba-frost-192,ngcc_ref_kem_20_MAMBA_Frost_192))

# MAMBA-Frost / MAMBA-Frost-256 (256-bit)
ALGORITHM := MAMBA-Frost
SRCDIR := Implementations/Reference_Implementation/MAMBA-Frost-256
SRCS := Frost/src/frost256.c Frost/src/util.c common/sha3/fips202.c common/aes/aes_c.c            \
    KEM_AlgorithmInstance.c randombytes_adapter.c drng.c auxfunc.c
INC := -Isrc/MAMBA-Frost-256/Frost/src -Isrc/MAMBA-Frost-256/common/sha3                          \
    -Isrc/MAMBA-Frost-256/common/aes -Isrc/MAMBA-Frost-256/common/random
DEFS := -DNIX -D_AMD64_ -D_REFERENCE_ -D_AES128_FOR_A_ -DFROST_USE_E8_CODE
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-20,MAMBA-Frost-256,kem,256,kem-20-mamba-frost-256,ngcc_ref_kem_20_MAMBA_Frost_256))

# MAMBA-Frost / MAMBA-Frost-CC-128 (128-bit)
ALGORITHM := MAMBA-Frost
SRCDIR := Implementations/Reference_Implementation/MAMBA-Frost-CC-128
SRCS := Frost-CC/src/frostcc128.c Frost-CC/src/util.c common/sha3/fips202.c common/aes/aes_c.c    \
    KEM_AlgorithmInstance.c randombytes_adapter.c drng.c auxfunc.c
INC := -Isrc/MAMBA-Frost-CC-128/Frost-CC/src -Isrc/MAMBA-Frost-CC-128/common/sha3                 \
    -Isrc/MAMBA-Frost-CC-128/common/aes -Isrc/MAMBA-Frost-CC-128/common/random
DEFS := -DNIX -D_AMD64_ -D_REFERENCE_ -D_AES128_FOR_A_ -DFROST_USE_E8_CODE
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-20,MAMBA-Frost-CC-128,kem,128,kem-20-mamba-frost-cc-128,ngcc_ref_kem_20_MAMBA_Frost_CC_128))

# MAMBA-Frost / MAMBA-Frost-CC-192 (192-bit)
ALGORITHM := MAMBA-Frost
SRCDIR := Implementations/Reference_Implementation/MAMBA-Frost-CC-192
SRCS := Frost-CC/src/frostcc192.c Frost-CC/src/util.c common/sha3/fips202.c common/aes/aes_c.c    \
    KEM_AlgorithmInstance.c randombytes_adapter.c drng.c auxfunc.c
INC := -Isrc/MAMBA-Frost-CC-192/Frost-CC/src -Isrc/MAMBA-Frost-CC-192/common/sha3                 \
    -Isrc/MAMBA-Frost-CC-192/common/aes -Isrc/MAMBA-Frost-CC-192/common/random
DEFS := -DNIX -D_AMD64_ -D_REFERENCE_ -D_AES128_FOR_A_ -DFROST_USE_E8_CODE
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-20,MAMBA-Frost-CC-192,kem,192,kem-20-mamba-frost-cc-192,ngcc_ref_kem_20_MAMBA_Frost_CC_192))

# MAMBA-Frost / MAMBA-Frost-CC-256 (256-bit)
ALGORITHM := MAMBA-Frost
SRCDIR := Implementations/Reference_Implementation/MAMBA-Frost-CC-256
SRCS := Frost-CC/src/frostcc256.c Frost-CC/src/util.c common/sha3/fips202.c common/aes/aes_c.c    \
    KEM_AlgorithmInstance.c randombytes_adapter.c drng.c auxfunc.c
INC := -Isrc/MAMBA-Frost-CC-256/Frost-CC/src -Isrc/MAMBA-Frost-CC-256/common/sha3                 \
    -Isrc/MAMBA-Frost-CC-256/common/aes -Isrc/MAMBA-Frost-CC-256/common/random
DEFS := -DNIX -D_AMD64_ -D_REFERENCE_ -D_AES128_FOR_A_ -DFROST_USE_E8_CODE
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-20,MAMBA-Frost-CC-256,kem,256,kem-20-mamba-frost-cc-256,ngcc_ref_kem_20_MAMBA_Frost_CC_256))

