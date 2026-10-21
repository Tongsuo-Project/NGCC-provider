# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# BRA / BRA-128 (128-bit)
ALGORITHM := BRA
SRCDIR := Implementations/Reference_Implementation/BRA-128
SRCS := src/augmented_gabidulin.c src/bra.c src/kem.c src/parsing.c src/qpoly.c                   \
    lib/random_source/random_source.c lib/XKCP/KeccakHash.c lib/XKCP/KeccakHashtimes4.c           \
    lib/XKCP/KeccakSponge.c lib/XKCP/KeccakSpongetimes4.c lib/XKCP/SimpleFIPS202.c                \
    lib/XKCP/opt64/KeccakP-1600-opt64.c lib/XKCP/opt64/KeccakP-1600-times4-on1.c drng.c auxfunc.c \
    KEM_BRA-128.c src/rbc-67/rbc_elt.c src/rbc-67/rbc_vec.c src/rbc-67/rbc_poly.c                 \
    src/rbc-67/rbc_qre.c src/rbc-67/rbc_vspace.c
INC := -Isrc/BRA-128/src -Isrc/BRA-128/src/rbc-67 -Isrc/BRA-128/lib/random_source                 \
    -Isrc/BRA-128/lib/XKCP -Isrc/BRA-128/lib/XKCP/opt64 -Isrc/BRA-128/lib/wrapper
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_BRA-128.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-06,BRA-128,kem,128,kem-06-bra-128,ngcc_ref_kem_06_BRA_128))

# BRA / BRA-256 (256-bit)
ALGORITHM := BRA
SRCDIR := Implementations/Reference_Implementation/BRA-256
SRCS := src/augmented_gabidulin.c src/bra.c src/kem.c src/parsing.c src/qpoly.c                   \
    lib/random_source/random_source.c lib/XKCP/KeccakHash.c lib/XKCP/KeccakHashtimes4.c           \
    lib/XKCP/KeccakSponge.c lib/XKCP/KeccakSpongetimes4.c lib/XKCP/SimpleFIPS202.c                \
    lib/XKCP/opt64/KeccakP-1600-opt64.c lib/XKCP/opt64/KeccakP-1600-times4-on1.c drng.c auxfunc.c \
    KEM_BRA-256.c src/rbc-83/rbc_elt.c src/rbc-83/rbc_vec.c src/rbc-83/rbc_poly.c                 \
    src/rbc-83/rbc_qre.c src/rbc-83/rbc_vspace.c
INC := -Isrc/BRA-256/src -Isrc/BRA-256/src/rbc-83 -Isrc/BRA-256/lib/random_source                 \
    -Isrc/BRA-256/lib/XKCP -Isrc/BRA-256/lib/XKCP/opt64 -Isrc/BRA-256/lib/wrapper
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_BRA-256.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-06,BRA-256,kem,256,kem-06-bra-256,ngcc_ref_kem_06_BRA_256))
