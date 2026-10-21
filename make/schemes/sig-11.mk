# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# FlexTree / Flextree-160f (160-bit)
ALGORITHM := FlexTree
SRCDIR := Implementations/Reference_Implementation/Flextree-160f
SRCS := SIG_AlgorithmInstance.c address.c auxfunc.c counters.c drng.c hash_sm3.c merkle.c         \
    pors_fp.c randombytes.c sign.c thash_sm3_simple.c utils.c utilsx1.c wots.c wotsx1.c
INC := 
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-11,Flextree-160f,sig,160,sig-11-flextree-160f,ngcc_ref_sig_11_Flextree_160f))

# FlexTree / Flextree-160s (160-bit)
ALGORITHM := FlexTree
SRCDIR := Implementations/Reference_Implementation/Flextree-160s
SRCS := SIG_AlgorithmInstance.c address.c auxfunc.c counters.c drng.c hash_sm3.c merkle.c         \
    pors_fp.c randombytes.c sign.c thash_sm3_simple.c utils.c utilsx1.c wots.c wotsx1.c
INC := 
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-11,Flextree-160s,sig,160,sig-11-flextree-160s,ngcc_ref_sig_11_Flextree_160s))

# FlexTree / Flextree-256f (256-bit)
ALGORITHM := FlexTree
SRCDIR := Implementations/Reference_Implementation/Flextree-256f
SRCS := SIG_AlgorithmInstance.c address.c auxfunc.c counters.c drng.c hash_sm3.c merkle.c         \
    pors_fp.c randombytes.c sign.c thash_sm3_simple.c utils.c utilsx1.c wots.c wotsx1.c
INC := 
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-11,Flextree-256f,sig,256,sig-11-flextree-256f,ngcc_ref_sig_11_Flextree_256f))

# FlexTree / Flextree-256s (256-bit)
ALGORITHM := FlexTree
SRCDIR := Implementations/Reference_Implementation/Flextree-256s
SRCS := SIG_AlgorithmInstance.c address.c auxfunc.c counters.c drng.c hash_sm3.c merkle.c         \
    pors_fp.c randombytes.c sign.c thash_sm3_simple.c utils.c utilsx1.c wots.c wotsx1.c
INC := 
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-11,Flextree-256s,sig,256,sig-11-flextree-256s,ngcc_ref_sig_11_Flextree_256s))

