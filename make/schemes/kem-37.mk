# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# TriQ-KEM / TriQ-KEM-128 (128-bit)
ALGORITHM := TriQ-KEM
SRCDIR := Implementations/Reference_Implementation/TriQ-KEM-128
SRCS := KEM_AlgorithmInstance.c drng.c auxfunc.c src/common/code.c src/common/crypto_memset.c     \
    src/common/fft.c src/common/kem.c src/common/symmetric.c src/ref/gf.c src/ref/gf2x.c          \
    src/ref/triq_pke.c src/ref/parsing.c src/ref/reed_muller.c src/ref/reed_solomon.c             \
    src/ref/vector.c
INC := -Isrc/TriQ-KEM-128/src/common -Isrc/TriQ-KEM-128/src/common/triq-128                       \
    -Isrc/TriQ-KEM-128/src/ref -Isrc/TriQ-KEM-128/src/ref/triq-128
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-37,TriQ-KEM-128,kem,128,kem-37-triq-kem-128,ngcc_ref_kem_37_TriQ_KEM_128))

# TriQ-KEM / TriQ-KEM-256 (256-bit)
ALGORITHM := TriQ-KEM
SRCDIR := Implementations/Reference_Implementation/TriQ-KEM-256
SRCS := KEM_AlgorithmInstance.c drng.c auxfunc.c src/common/code.c src/common/crypto_memset.c     \
    src/common/fft.c src/common/kem.c src/common/symmetric.c src/ref/gf.c src/ref/gf2x.c          \
    src/ref/triq_pke.c src/ref/parsing.c src/ref/reed_muller.c src/ref/reed_solomon.c             \
    src/ref/vector.c
INC := -Isrc/TriQ-KEM-256/src/common -Isrc/TriQ-KEM-256/src/common/triq-256                       \
    -Isrc/TriQ-KEM-256/src/ref -Isrc/TriQ-KEM-256/src/ref/triq-256
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-37,TriQ-KEM-256,kem,256,kem-37-triq-kem-256,ngcc_ref_kem_37_TriQ_KEM_256))

