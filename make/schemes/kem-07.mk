# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# BRQC / BRQC-128 (128-bit)
ALGORITHM := BRQC
SRCDIR := Implementations/Reference_Implementation/BRQC-128
SRCS := src/brqc.c src/gabidulin.c src/kem.c src/parsing.c src/qpoly.c                            \
    lib/random_source/random_source.c lib/XKCP/KeccakHash.c lib/XKCP/KeccakHashtimes4.c           \
    lib/XKCP/KeccakSponge.c lib/XKCP/KeccakSpongetimes4.c lib/XKCP/SimpleFIPS202.c                \
    lib/XKCP/opt64/KeccakP-1600-opt64.c lib/XKCP/opt64/KeccakP-1600-times4-on1.c drng.c auxfunc.c \
    KEM_BRQC-128.c src/rbc-127/rbc_elt.c src/rbc-127/rbc_vec.c src/rbc-127/rbc_poly.c             \
    src/rbc-127/rbc_qre.c src/rbc-127/rbc_vspace.c
INC := -Isrc/BRQC-128/src -Isrc/BRQC-128/src/rbc-127 -Isrc/BRQC-128/lib/random_source             \
    -Isrc/BRQC-128/lib/XKCP -Isrc/BRQC-128/lib/XKCP/opt64 -Isrc/BRQC-128/lib/wrapper
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_BRQC-128.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-07,BRQC-128,kem,128,kem-07-brqc-128,ngcc_ref_kem_07_BRQC_128))

# BRQC / BRQC-256 (256-bit)
ALGORITHM := BRQC
SRCDIR := Implementations/Reference_Implementation/BRQC-256
SRCS := src/brqc.c src/gabidulin.c src/kem.c src/parsing.c src/qpoly.c                            \
    lib/random_source/random_source.c lib/XKCP/KeccakHash.c lib/XKCP/KeccakHashtimes4.c           \
    lib/XKCP/KeccakSponge.c lib/XKCP/KeccakSpongetimes4.c lib/XKCP/SimpleFIPS202.c                \
    lib/XKCP/opt64/KeccakP-1600-opt64.c lib/XKCP/opt64/KeccakP-1600-times4-on1.c drng.c auxfunc.c \
    KEM_BRQC-256.c src/rbc-163/rbc_elt.c src/rbc-163/rbc_vec.c src/rbc-163/rbc_poly.c             \
    src/rbc-163/rbc_qre.c src/rbc-163/rbc_vspace.c
INC := -Isrc/BRQC-256/src -Isrc/BRQC-256/src/rbc-163 -Isrc/BRQC-256/lib/random_source             \
    -Isrc/BRQC-256/lib/XKCP -Isrc/BRQC-256/lib/XKCP/opt64 -Isrc/BRQC-256/lib/wrapper
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_BRQC-256.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-07,BRQC-256,kem,256,kem-07-brqc-256,ngcc_ref_kem_07_BRQC_256))
