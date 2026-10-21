# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Aigis-Sig+ / Aigis-sig1 (128-bit)
ALGORITHM := Aigis-Sig+
SRCDIR := Implementations/Implementations/Reference_Implementation/Aigis-Sig+-I
SRCS := sign.c polyvec.c packing.c poly.c reduce.c ntt.c rounding.c fips202.c hashkdf.c auxfunc.c \
    SIG_AlgorithmInstance.c drng.c pspm.c rng.c
INC := 
DEFS := -DPARAMS=1 -DUSE_ICCS
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-01,Aigis-sig1,sig,128,sig-01-aigis-sig1,ngcc_ref_sig_01_Aigis_sig1))

# Aigis-Sig+ / Aigis-sig2 (256-bit)
ALGORITHM := Aigis-Sig+
SRCDIR := Implementations/Implementations/Reference_Implementation/Aigis-Sig+-II
SRCS := sign.c polyvec.c packing.c poly.c reduce.c ntt.c rounding.c fips202.c hashkdf.c auxfunc.c \
    SIG_AlgorithmInstance.c drng.c pspm.c rng.c
INC := 
DEFS := -DPARAMS=2 -DUSE_ICCS
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-01,Aigis-sig2,sig,256,sig-01-aigis-sig2,ngcc_ref_sig_01_Aigis_sig2))

