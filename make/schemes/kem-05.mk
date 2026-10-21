# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# BIKE-MLThre / BIKE_v2_128 (128-bit)
ALGORITHM := BIKE-MLThre
SRCDIR := Implementations/reference
SRCS := auxfunc.c conversions.c decode_ml.c hash_wrapper.c kem.c mlthre_model.c mlthre_policy.c   \
    mlthre_runtime_sampling.c ntl.c ring_buffer.c sampling.c threshold.c utilities.c xof_prng.c   \
    FromNIST/rng.c kat/KEM_AlgorithmInstance.c kat/drng.c
INC := -Isrc/BIKE_v2_128/kat
DEFS := -DNIST_RAND=1 -DBIKE_SECURITY_128 -DBIKE_MLTHRE_ENABLED=1
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-05,BIKE_v2_128,kem,128,kem-05-bike-v2-128,ngcc_ref_kem_05_BIKE_v2_128))

# BIKE-MLThre / BIKE_v2_256 (256-bit)
ALGORITHM := BIKE-MLThre
SRCDIR := Implementations/reference
SRCS := auxfunc.c conversions.c decode_ml.c hash_wrapper.c kem.c mlthre_model.c mlthre_policy.c   \
    mlthre_runtime_sampling.c ntl.c ring_buffer.c sampling.c threshold.c utilities.c xof_prng.c   \
    FromNIST/rng.c kat/KEM_AlgorithmInstance.c kat/drng.c
INC := -Isrc/BIKE_v2_256/kat
DEFS := -DNIST_RAND=1 -DBIKE_SECURITY_256 -DBIKE_MLTHRE_ENABLED=1
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-05,BIKE_v2_256,kem,256,kem-05-bike-v2-256,ngcc_ref_kem_05_BIKE_v2_256))

