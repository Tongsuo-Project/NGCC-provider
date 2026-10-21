# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# MAMBA-Viper / MAMBA-Viper-128 (128-bit)
ALGORITHM := MAMBA-Viper
SRCDIR := Implementations/Reference_Implementation/MAMBA-Viper-128
SRCS := KEM_AlgorithmInstance.c auxfunc.c drng.c kem.c verify.c viper.c viper_api_hash.c          \
    viper_api_rng.c viper_arith.c viper_e8.c viper_message_codec.c
INC := 
DEFS := -DVIPER_LEVEL=128 -DVIPER_USE_E8_CODEC=1
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-21,MAMBA-Viper-128,kem,128,kem-21-mamba-viper-128,ngcc_ref_kem_21_MAMBA_Viper_128))

# MAMBA-Viper / MAMBA-Viper-192 (192-bit)
ALGORITHM := MAMBA-Viper
SRCDIR := Implementations/Reference_Implementation/MAMBA-Viper-192
SRCS := KEM_AlgorithmInstance.c auxfunc.c drng.c kem.c verify.c viper.c viper_api_hash.c          \
    viper_api_rng.c viper_arith.c viper_e8.c viper_message_codec.c
INC := 
DEFS := -DVIPER_LEVEL=192 -DVIPER_USE_E8_CODEC=1
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-21,MAMBA-Viper-192,kem,192,kem-21-mamba-viper-192,ngcc_ref_kem_21_MAMBA_Viper_192))

# MAMBA-Viper / MAMBA-Viper-256 (256-bit)
ALGORITHM := MAMBA-Viper
SRCDIR := Implementations/Reference_Implementation/MAMBA-Viper-256
SRCS := KEM_AlgorithmInstance.c auxfunc.c drng.c kem.c verify.c viper.c viper_api_hash.c          \
    viper_api_rng.c viper_arith.c viper_e8.c viper_message_codec.c
INC := 
DEFS := -DVIPER_LEVEL=256 -DVIPER_USE_E8_CODEC=1
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-21,MAMBA-Viper-256,kem,256,kem-21-mamba-viper-256,ngcc_ref_kem_21_MAMBA_Viper_256))

