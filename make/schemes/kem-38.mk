# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# UVW-KEM / UVW-KEM-128 (128-bit)
ALGORITHM := UVW-KEM
SRCDIR := Implementations/Reference_Implementation/UVW-KEM-128
SRCS := src/KEM_AlgorithmInstance.c src/drng.c src/auxfunc.c src/gf_math.c src/list_decoding.c    \
    src/uvw_constants.c
INC := -Isrc/UVW-KEM-128/include
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"include/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-38,UVW-KEM-128,kem,128,kem-38-uvw-kem-128,ngcc_ref_kem_38_UVW_KEM_128))

# UVW-KEM / UVW-KEM-256 (256-bit)
ALGORITHM := UVW-KEM
SRCDIR := Implementations/Reference_Implementation/UVW-KEM-256
SRCS := src/KEM_AlgorithmInstance.c src/drng.c src/auxfunc.c src/gf_math.c src/list_decoding.c    \
    src/uvw_constants.c
INC := -Isrc/UVW-KEM-256/include
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"include/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-38,UVW-KEM-256,kem,256,kem-38-uvw-kem-256,ngcc_ref_kem_38_UVW_KEM_256))

