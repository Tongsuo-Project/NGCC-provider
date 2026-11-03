# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# BAG-Loong / BAG-Loong-128 (128-bit)
ALGORITHM := BAG-Loong
SRCDIR := Implementations/Reference_Implementation/Loong-Block-ms-128
SRCS := src/KEM_AlgorithmInstance.c src/augabidulin.c src/ct_util.c src/gabidulin.c               \
    src/loong_api_random.c src/loong_hash.c src/loong_kem.c src/loong_pke.c src/loong_support.c   \
    src/loong_xof_reader.c src/parsing.c src/qpoly.c lib/api_pkc/auxfunc.c lib/api_pkc/drng.c     \
    lib/rbc-47/rbc_elt.c lib/rbc-47/rbc_vec.c
INC := -Isrc/BAG-Loong-128/src -Isrc/BAG-Loong-128/lib/api_pkc -Isrc/BAG-Loong-128/lib/rbc-47
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"src/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-03,BAG-Loong-128,kem,128,kem-03-bag-loong-128,ngcc_ref_kem_03_BAG_Loong_128))

# BAG-Loong / BAG-Loong-256 (256-bit)
ALGORITHM := BAG-Loong
SRCDIR := Implementations/Reference_Implementation/Loong-Block-ms-256
SRCS := src/KEM_AlgorithmInstance.c src/augabidulin.c src/ct_util.c src/gabidulin.c               \
    src/loong_api_random.c src/loong_hash.c src/loong_kem.c src/loong_pke.c src/loong_support.c   \
    src/loong_xof_reader.c src/parsing.c src/qpoly.c lib/api_pkc/auxfunc.c lib/api_pkc/drng.c     \
    lib/rbc-67/rbc_elt.c lib/rbc-67/rbc_vec.c
INC := -Isrc/BAG-Loong-256/src -Isrc/BAG-Loong-256/lib/api_pkc -Isrc/BAG-Loong-256/lib/rbc-67
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"src/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-03,BAG-Loong-256,kem,256,kem-03-bag-loong-256,ngcc_ref_kem_03_BAG_Loong_256))

