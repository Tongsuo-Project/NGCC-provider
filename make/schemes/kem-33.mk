# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# QUBE / qube-128 (128-bit)
ALGORITHM := QUBE
SRCDIR := Implementations/Reference_Implementation/qube-128
SRCS := src/common/code.c src/common/crypto_memset.c src/common/fft.c src/common/kem.c            \
    src/common/kem_qube.c src/common/symmetric.c src/ref/gf.c src/ref/gf2x.c src/ref/parsing.c    \
    src/ref/qube.c src/ref/reed_muller.c src/ref/reed_solomon.c src/ref/vector.c                  \
    lib/api_pkc/auxfunc.c lib/api_pkc/drng.c
INC := -Isrc/qube-128/src/common -Isrc/qube-128/src/ref -Isrc/qube-128/lib/api_pkc
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"kem_qube.h"' -DNGCC_INSTANCE='"qube_128"'
NOAUX := 
$(eval $(call ngcc_instance,kem-33,qube-128,kem,128,kem-33-qube-128,ngcc_ref_kem_33_qube_128))

# QUBE / qube-192 (192-bit)
ALGORITHM := QUBE
SRCDIR := Implementations/Reference_Implementation/qube-192
SRCS := src/common/code.c src/common/crypto_memset.c src/common/fft.c src/common/kem.c            \
    src/common/kem_qube.c src/common/symmetric.c src/ref/gf.c src/ref/gf2x.c src/ref/parsing.c    \
    src/ref/qube.c src/ref/reed_muller.c src/ref/reed_solomon.c src/ref/vector.c                  \
    lib/api_pkc/auxfunc.c lib/api_pkc/drng.c
INC := -Isrc/qube-192/src/common -Isrc/qube-192/src/ref -Isrc/qube-192/lib/api_pkc
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"kem_qube.h"' -DNGCC_INSTANCE='"qube_192"'
NOAUX := 
$(eval $(call ngcc_instance,kem-33,qube-192,kem,192,kem-33-qube-192,ngcc_ref_kem_33_qube_192))

# QUBE / qube-256 (256-bit)
ALGORITHM := QUBE
SRCDIR := Implementations/Reference_Implementation/qube-256
SRCS := src/common/code.c src/common/crypto_memset.c src/common/fft.c src/common/kem.c            \
    src/common/kem_qube.c src/common/symmetric.c src/ref/gf.c src/ref/gf2x.c src/ref/parsing.c    \
    src/ref/qube.c src/ref/reed_muller.c src/ref/reed_solomon.c src/ref/vector.c                  \
    lib/api_pkc/auxfunc.c lib/api_pkc/drng.c
INC := -Isrc/qube-256/src/common -Isrc/qube-256/src/ref -Isrc/qube-256/lib/api_pkc
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"kem_qube.h"' -DNGCC_INSTANCE='"qube_256"'
NOAUX := 
$(eval $(call ngcc_instance,kem-33,qube-256,kem,256,kem-33-qube-256,ngcc_ref_kem_33_qube_256))

