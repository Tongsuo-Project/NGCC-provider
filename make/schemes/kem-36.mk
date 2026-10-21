# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# TRIKE / TRIKE-1 (128-bit)
ALGORITHM := TRIKE
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/TRIKE-1
SRCS := ICCS/auxfunc.c ICCS/drng.c src/KEM_AlgorithmInstance.c src/decoder.c src/gf2x.c src/sample.c
INC := -Isrc/TRIKE-1/ICCS -Isrc/TRIKE-1/src
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"src/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-36,TRIKE-1,kem,128,kem-36-trike-1,ngcc_ref_kem_36_TRIKE_1))

# TRIKE / TRIKE-5 (256-bit)
ALGORITHM := TRIKE
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/TRIKE-5
SRCS := ICCS/auxfunc.c ICCS/drng.c src/KEM_AlgorithmInstance.c src/decoder.c src/gf2x.c src/sample.c
INC := -Isrc/TRIKE-5/ICCS -Isrc/TRIKE-5/src
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"src/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-36,TRIKE-5,kem,256,kem-36-trike-5,ngcc_ref_kem_36_TRIKE_5))

