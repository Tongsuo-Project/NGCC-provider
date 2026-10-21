# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# QCTM / QCTM128 (128-bit)
ALGORITHM := QCTM
SRCDIR := Implementations/Reference_Implementation/QCTM128
SRCS := keccak.c kem.c m2e.c permutation.c gf.c poly.c goppa.c seeded_keygen.c qctm128_kem_api.c  \
    decode.c roots.c rng.c KEM_AlgorithmInstance.c .api-pkc-kat/drng.c
INC := -Isrc/QCTM128/.api-pkc-kat
DEFS := -DAPI_PKC_KAT_DRNG_BRIDGE
INSTANCE_CFLAGS := -fcommon
SHIMDEFS := 
NOAUX := 1
$(eval $(call ngcc_instance,kem-32,QCTM128,kem,128,kem-32-qctm128,ngcc_ref_kem_32_QCTM128))

# QCTM / QCTM256 (256-bit)
ALGORITHM := QCTM
SRCDIR := Implementations/Reference_Implementation/QCTM256
SRCS := keccak.c kem.c m2e.c permutation.c gf.c poly.c goppa.c seeded_keygen.c qctm256_kem_api.c  \
    decode.c roots.c rng.c KEM_AlgorithmInstance.c .api-pkc-kat/drng.c
INC := -Isrc/QCTM256/.api-pkc-kat
DEFS := -DAPI_PKC_KAT_DRNG_BRIDGE
INSTANCE_CFLAGS := -fcommon
SHIMDEFS := 
NOAUX := 1
$(eval $(call ngcc_instance,kem-32,QCTM256,kem,256,kem-32-qctm256,ngcc_ref_kem_32_QCTM256))

