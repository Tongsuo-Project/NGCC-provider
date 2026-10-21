# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# CEDRUS-alpha / CEDRUSALPHA-160f (160-bit)
ALGORITHM := CEDRUS-alpha
SRCDIR := Implementations/Reference_Implementation/CEDRUSALPHA-160f
SRCS := SIG_AlgorithmInstance.c address.c auxfunc.c drng.c fors.c hash_sm3.c merkle.c             \
    randombytes.c sign.c thash_sm3_simple.c utils.c utilsx1.c wots.c wotsx1.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-04,CEDRUSALPHA-160f,sig,160,sig-04-cedrusalpha-160f,ngcc_ref_sig_04_CEDRUSALPHA_160f))

# CEDRUS-alpha / CEDRUSALPHA-160s (160-bit)
ALGORITHM := CEDRUS-alpha
SRCDIR := Implementations/Reference_Implementation/CEDRUSALPHA-160s
SRCS := SIG_AlgorithmInstance.c address.c auxfunc.c drng.c fors.c hash_sm3.c merkle.c             \
    randombytes.c sign.c thash_sm3_simple.c utils.c utilsx1.c wots.c wotsx1.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-04,CEDRUSALPHA-160s,sig,160,sig-04-cedrusalpha-160s,ngcc_ref_sig_04_CEDRUSALPHA_160s))

# CEDRUS-alpha / CEDRUSALPHA-256f (256-bit)
ALGORITHM := CEDRUS-alpha
SRCDIR := Implementations/Reference_Implementation/CEDRUSALPHA-256f
SRCS := SIG_AlgorithmInstance.c address.c auxfunc.c drng.c fors.c hash_sm3.c merkle.c             \
    randombytes.c sign.c thash_sm3_simple.c utils.c utilsx1.c wots.c wotsx1.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-04,CEDRUSALPHA-256f,sig,256,sig-04-cedrusalpha-256f,ngcc_ref_sig_04_CEDRUSALPHA_256f))

# CEDRUS-alpha / CEDRUSALPHA-256s (256-bit)
ALGORITHM := CEDRUS-alpha
SRCDIR := Implementations/Reference_Implementation/CEDRUSALPHA-256s
SRCS := SIG_AlgorithmInstance.c address.c auxfunc.c drng.c fors.c hash_sm3.c merkle.c             \
    randombytes.c sign.c thash_sm3_simple.c utils.c utilsx1.c wots.c wotsx1.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-04,CEDRUSALPHA-256s,sig,256,sig-04-cedrusalpha-256s,ngcc_ref_sig_04_CEDRUSALPHA_256s))

