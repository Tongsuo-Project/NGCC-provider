# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# TRINE / TRINE-128-Balanced (128-bit)
ALGORITHM := TRINE
SRCDIR := Implementations/Reference_Implementation/TRINE-128-balanced
SRCS := trine.c util.c field.c matrixmod.c matrixelim.c triform.c canonical.c corank1.c           \
    trine_expand.c trine_codec.c bitstream.c hashkdf.c randombytes_iccs.c                         \
    iccs/SIG_AlgorithmInstance.c iccs/drng.c iccs/auxfunc.c
INC := -Isrc/TRINE-128-Balanced/iccs
DEFS := -DPARAMS=1 -DUSE_ICCS -DTRINE_ALGORITHM_INSTANCE='"TRINE-128-Balanced"'
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-30,TRINE-128-Balanced,sig,128,sig-30-trine-128-balanced,ngcc_ref_sig_30_TRINE_128_Balanced))

# TRINE / TRINE-128-ShortSig (128-bit)
ALGORITHM := TRINE
SRCDIR := Implementations/Reference_Implementation/TRINE-128-ShortSig
SRCS := trine.c util.c field.c matrixmod.c matrixelim.c triform.c canonical.c corank1.c           \
    trine_expand.c trine_codec.c bitstream.c hashkdf.c randombytes_iccs.c                         \
    iccs/SIG_AlgorithmInstance.c iccs/drng.c iccs/auxfunc.c
INC := -Isrc/TRINE-128-ShortSig/iccs
DEFS := -DPARAMS=4 -DUSE_ICCS -DTRINE_ALGORITHM_INSTANCE='"TRINE-128-ShortSig"'
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-30,TRINE-128-ShortSig,sig,128,sig-30-trine-128-shortsig,ngcc_ref_sig_30_TRINE_128_ShortSig))

# TRINE / TRINE-256-Balanced (256-bit)
ALGORITHM := TRINE
SRCDIR := Implementations/Reference_Implementation/TRINE-256-balanced
SRCS := trine.c util.c field.c matrixmod.c matrixelim.c triform.c canonical.c corank1.c           \
    trine_expand.c trine_codec.c bitstream.c hashkdf.c randombytes_iccs.c                         \
    iccs/SIG_AlgorithmInstance.c iccs/drng.c iccs/auxfunc.c
INC := -Isrc/TRINE-256-Balanced/iccs
DEFS := -DPARAMS=2 -DUSE_ICCS -DTRINE_ALGORITHM_INSTANCE='"TRINE-256-Balanced"'
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-30,TRINE-256-Balanced,sig,256,sig-30-trine-256-balanced,ngcc_ref_sig_30_TRINE_256_Balanced))

# TRINE / TRINE-256-ShortSig (256-bit)
ALGORITHM := TRINE
SRCDIR := Implementations/Reference_Implementation/TRINE-256-ShortSig
SRCS := trine.c util.c field.c matrixmod.c matrixelim.c triform.c canonical.c corank1.c           \
    trine_expand.c trine_codec.c bitstream.c hashkdf.c randombytes_iccs.c                         \
    iccs/SIG_AlgorithmInstance.c iccs/drng.c iccs/auxfunc.c
INC := -Isrc/TRINE-256-ShortSig/iccs
DEFS := -DPARAMS=5 -DUSE_ICCS -DTRINE_ALGORITHM_INSTANCE='"TRINE-256-ShortSig"'
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-30,TRINE-256-ShortSig,sig,256,sig-30-trine-256-shortsig,ngcc_ref_sig_30_TRINE_256_ShortSig))

