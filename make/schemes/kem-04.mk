# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# BAG-Piglet / bag_piglet_128 (128-bit)
ALGORITHM := BAG-Piglet
SRCDIR := Implementations/Reference_Implementation/bag_piglet128
SRCS := src/common/aes256ctr.c src/common/augabidulin.c src/common/ccakem.c src/common/ccapke.c   \
    src/common/gabidulin.c src/common/parsing.c src/common/q_polynomial.c src/common/tools.c      \
    src/ffi/ffi_elt.c src/ffi/ffi_field.c src/ffi/ffi_vec.c src/rbc/rbc_elt.c src/rbc/rbc_poly.c  \
    src/rbc/rbc_qre.c src/rbc/rbc_vec.c src/rbc/rbc_vspace.c src/scheme/bag_piglet.c              \
    src/kat/KEM_AlgorithmInstance.c lib/api_pkc/auxfunc.c lib/api_pkc/drng.c                      \
    lib/randombytes/randombytes.c
INC := -Isrc/bag_piglet_128/src/common -Isrc/bag_piglet_128/src/scheme                            \
    -Isrc/bag_piglet_128/src/ffi -Isrc/bag_piglet_128/src/rbc -Isrc/bag_piglet_128/src/kat        \
    -Isrc/bag_piglet_128/lib/api_pkc -Isrc/bag_piglet_128/lib/randombytes
DEFS := -DBAG_PIGLET_KAT=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"src/kat/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-04,bag_piglet_128,kem,128,kem-04-bag-piglet-128,ngcc_ref_kem_04_bag_piglet_128))

# BAG-Piglet / bag_piglet_256 (256-bit)
ALGORITHM := BAG-Piglet
SRCDIR := Implementations/Reference_Implementation/bag_piglet256
SRCS := src/common/aes256ctr.c src/common/augabidulin.c src/common/ccakem.c src/common/ccapke.c   \
    src/common/gabidulin.c src/common/parsing.c src/common/q_polynomial.c src/common/tools.c      \
    src/ffi/ffi_elt.c src/ffi/ffi_field.c src/ffi/ffi_vec.c src/rbc/rbc_elt.c src/rbc/rbc_poly.c  \
    src/rbc/rbc_qre.c src/rbc/rbc_vec.c src/rbc/rbc_vspace.c src/scheme/bag_piglet.c              \
    src/kat/KEM_AlgorithmInstance.c lib/api_pkc/auxfunc.c lib/api_pkc/drng.c                      \
    lib/randombytes/randombytes.c
INC := -Isrc/bag_piglet_256/src/common -Isrc/bag_piglet_256/src/scheme                            \
    -Isrc/bag_piglet_256/src/ffi -Isrc/bag_piglet_256/src/rbc -Isrc/bag_piglet_256/src/kat        \
    -Isrc/bag_piglet_256/lib/api_pkc -Isrc/bag_piglet_256/lib/randombytes
DEFS := -DBAG_PIGLET_KAT=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"src/kat/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-04,bag_piglet_256,kem,256,kem-04-bag-piglet-256,ngcc_ref_kem_04_bag_piglet_256))
