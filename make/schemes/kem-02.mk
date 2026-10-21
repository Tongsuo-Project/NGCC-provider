# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Amoeba / Amoeba128 (128-bit)
ALGORITHM := Amoeba
SRCDIR := Implementations/Reference_Implementation/Amoeba-576/src
SRCS := api/KEM_AlgorithmInstance.c backend/ccakem.c backend/cpapke.c backend/hamming.c           \
    backend/ntt_2s3t.c backend/poly.c backend/randombytes.c backend/zetas.c symmetric/auxfunc.c   \
    symmetric/drng.c
INC := -Isrc/Amoeba128/symmetric -Isrc/Amoeba128/backend -Isrc/Amoeba128/api
DEFS := -DSECURITY_LEVEL=128
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"api/KEM_AlgorithmInstance.h"' -DNGCC_NO_DRNG
NOAUX := 
$(eval $(call ngcc_instance,kem-02,Amoeba128,kem,128,kem-02-amoeba128,ngcc_ref_kem_02_Amoeba128))

# Amoeba / Amoeba192 (192-bit)
ALGORITHM := Amoeba
SRCDIR := Implementations/Reference_Implementation/Amoeba-864/src
SRCS := api/KEM_AlgorithmInstance.c backend/ccakem.c backend/cpapke.c backend/hamming.c           \
    backend/ntt_2s3t.c backend/poly.c backend/randombytes.c backend/zetas.c symmetric/auxfunc.c   \
    symmetric/drng.c
INC := -Isrc/Amoeba192/symmetric -Isrc/Amoeba192/backend -Isrc/Amoeba192/api
DEFS := -DSECURITY_LEVEL=192
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"api/KEM_AlgorithmInstance.h"' -DNGCC_NO_DRNG
NOAUX := 
$(eval $(call ngcc_instance,kem-02,Amoeba192,kem,192,kem-02-amoeba192,ngcc_ref_kem_02_Amoeba192))

# Amoeba / Amoeba256 (256-bit)
ALGORITHM := Amoeba
SRCDIR := Implementations/Reference_Implementation/Amoeba-1152/src
SRCS := api/KEM_AlgorithmInstance.c backend/ccakem.c backend/cpapke.c backend/hamming.c           \
    backend/ntt_2s3t.c backend/poly.c backend/randombytes.c backend/zetas.c symmetric/auxfunc.c   \
    symmetric/drng.c
INC := -Isrc/Amoeba256/symmetric -Isrc/Amoeba256/backend -Isrc/Amoeba256/api
DEFS := -DSECURITY_LEVEL=256
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"api/KEM_AlgorithmInstance.h"' -DNGCC_NO_DRNG
NOAUX := 
$(eval $(call ngcc_instance,kem-02,Amoeba256,kem,256,kem-02-amoeba256,ngcc_ref_kem_02_Amoeba256))

