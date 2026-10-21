# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# CEDRUS+C / CEDRUSC-160f (160-bit)
ALGORITHM := CEDRUS+C
SRCDIR := Implementations/Reference_Implementation/CEDRUSC-160f
SRCS := SIG_AlgorithmInstance.c address.c auxfunc.c counters.c drng.c fors.c hash_sm3.c merkle.c  \
    randombytes.c sign.c thash_sm3_simple.c utils.c utilsx1.c wots.c wotsx1.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-03,CEDRUSC-160f,sig,160,sig-03-cedrusc-160f,ngcc_ref_sig_03_CEDRUSC_160f))

# CEDRUS+C / CEDRUSC-160s (160-bit)
ALGORITHM := CEDRUS+C
SRCDIR := Implementations/Reference_Implementation/CEDRUSC-160s
SRCS := SIG_AlgorithmInstance.c address.c auxfunc.c counters.c drng.c fors.c hash_sm3.c merkle.c  \
    randombytes.c sign.c thash_sm3_simple.c utils.c utilsx1.c wots.c wotsx1.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-03,CEDRUSC-160s,sig,160,sig-03-cedrusc-160s,ngcc_ref_sig_03_CEDRUSC_160s))

# CEDRUS+C / CEDRUSC-256f (256-bit)
ALGORITHM := CEDRUS+C
SRCDIR := Implementations/Reference_Implementation/CEDRUSC-256f
SRCS := SIG_AlgorithmInstance.c address.c auxfunc.c counters.c drng.c fors.c hash_sm3.c merkle.c  \
    randombytes.c sign.c thash_sm3_simple.c utils.c utilsx1.c wots.c wotsx1.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-03,CEDRUSC-256f,sig,256,sig-03-cedrusc-256f,ngcc_ref_sig_03_CEDRUSC_256f))

# CEDRUS+C / CEDRUSC-256s (256-bit)
ALGORITHM := CEDRUS+C
SRCDIR := Implementations/Reference_Implementation/CEDRUSC-256s
SRCS := SIG_AlgorithmInstance.c address.c auxfunc.c counters.c drng.c fors.c hash_sm3.c merkle.c  \
    randombytes.c sign.c thash_sm3_simple.c utils.c utilsx1.c wots.c wotsx1.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-03,CEDRUSC-256s,sig,256,sig-03-cedrusc-256s,ngcc_ref_sig_03_CEDRUSC_256s))

