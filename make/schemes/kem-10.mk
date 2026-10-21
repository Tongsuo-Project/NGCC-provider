# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# C-Multi-UR-AG / CMultiURAG-128 (128-bit)
ALGORITHM := C-Multi-UR-AG
SRCDIR := Implementations/Reference_Implementation/CMultiURAG-128
SRCS := KEM_CMultiURAG-128.c drng.c auxfunc.c lib/random_source/random_source.c                   \
    lib/XKCP/KeccakHash.c lib/XKCP/KeccakHashtimes4.c lib/XKCP/KeccakSponge.c                     \
    lib/XKCP/KeccakSpongetimes4.c lib/XKCP/SimpleFIPS202.c lib/XKCP/opt64/KeccakP-1600-opt64.c    \
    lib/XKCP/opt64/KeccakP-1600-times4-on1.c src/rbc-79/rbc_elt.c src/rbc-79/rbc_vec.c            \
    src/rbc-79/rbc_poly.c src/rbc-79/rbc_qre.c src/rbc-79/rbc_vspace.c src/rbc-79/rbc_mat.c       \
    src/parsing.c src/qpoly.c src/augmented_gabidulin.c src/cmultiurag.c src/kem.c
INC := -Isrc/CMultiURAG-128/src -Isrc/CMultiURAG-128/src/rbc-79                                   \
    -Isrc/CMultiURAG-128/lib/random_source -Isrc/CMultiURAG-128/lib/XKCP                          \
    -Isrc/CMultiURAG-128/lib/XKCP/opt64 -Isrc/CMultiURAG-128/lib/wrapper
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_CMultiURAG-128.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-10,CMultiURAG-128,kem,128,kem-10-cmultiurag-128,ngcc_ref_kem_10_CMultiURAG_128))

# C-Multi-UR-AG / CMultiURAG-256 (256-bit)
ALGORITHM := C-Multi-UR-AG
SRCDIR := Implementations/Reference_Implementation/CMultiURAG-256
SRCS := KEM_CMultiURAG-256.c drng.c auxfunc.c lib/random_source/random_source.c                   \
    lib/XKCP/KeccakHash.c lib/XKCP/KeccakHashtimes4.c lib/XKCP/KeccakSponge.c                     \
    lib/XKCP/KeccakSpongetimes4.c lib/XKCP/SimpleFIPS202.c lib/XKCP/opt64/KeccakP-1600-opt64.c    \
    lib/XKCP/opt64/KeccakP-1600-times4-on1.c src/rbc-127/rbc_elt.c src/rbc-127/rbc_vec.c          \
    src/rbc-127/rbc_poly.c src/rbc-127/rbc_qre.c src/rbc-127/rbc_vspace.c src/rbc-127/rbc_mat.c   \
    src/parsing.c src/qpoly.c src/augmented_gabidulin.c src/cmultiurag.c src/kem.c
INC := -Isrc/CMultiURAG-256/src -Isrc/CMultiURAG-256/src/rbc-127                                  \
    -Isrc/CMultiURAG-256/lib/random_source -Isrc/CMultiURAG-256/lib/XKCP                          \
    -Isrc/CMultiURAG-256/lib/XKCP/opt64 -Isrc/CMultiURAG-256/lib/wrapper
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_CMultiURAG-256.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-10,CMultiURAG-256,kem,256,kem-10-cmultiurag-256,ngcc_ref_kem_10_CMultiURAG_256))
