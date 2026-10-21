# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Rhyme / Rhyme-SM3-128 (128-bit)
ALGORITHM := Rhyme
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Rhyme-SM3/Rhyme-SM3-128
SRCS := src/poly.c src/ntt.c src/ntt_tables.c src/sampler.c src/encoding.c src/packing.c          \
    src/sign.c src/zpntt.c src/symmetric-shake.c rhyme_xof.c src/keygen/kg_main.c                 \
    src/keygen/kg_solver.c src/keygen/kg_zint.c src/keygen/kg_ntt.c src/keygen/kg_primes.c        \
    randombytes.c auxfunc.c drng.c SIG_AlgorithmInstance.c sm3.c sm3_xof.c
INC := -Isrc/Rhyme-SM3-128/include -Isrc/Rhyme-SM3-128/src/keygen
DEFS := -DRHYME_NO_AES -DRHYME_MODE=128 -DOUTPUT_BLANK_TEST_VECTORS=0 -DALGORITHM_INSTANCE='"Rhyme-SM3-128"'
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-22,Rhyme-SM3-128,sig,128,sig-22-rhyme-sm3-128,ngcc_ref_sig_22_Rhyme_SM3_128))

# Rhyme / Rhyme-SM3-256 (256-bit)
ALGORITHM := Rhyme
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Rhyme-SM3/Rhyme-SM3-256
SRCS := src/poly.c src/ntt.c src/ntt_tables.c src/sampler.c src/encoding.c src/packing.c          \
    src/sign.c src/zpntt.c src/symmetric-shake.c rhyme_xof.c src/keygen/kg_main.c                 \
    src/keygen/kg_solver.c src/keygen/kg_zint.c src/keygen/kg_ntt.c src/keygen/kg_primes.c        \
    randombytes.c auxfunc.c drng.c SIG_AlgorithmInstance.c sm3.c sm3_xof.c
INC := -Isrc/Rhyme-SM3-256/include -Isrc/Rhyme-SM3-256/src/keygen
DEFS := -DRHYME_NO_AES -DRHYME_MODE=256 -DOUTPUT_BLANK_TEST_VECTORS=0 -DALGORITHM_INSTANCE='"Rhyme-SM3-256"'
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-22,Rhyme-SM3-256,sig,256,sig-22-rhyme-sm3-256,ngcc_ref_sig_22_Rhyme_SM3_256))

