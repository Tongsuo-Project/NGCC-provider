# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Facto-DSA / Facto-DSA-128 (128-bit)
ALGORITHM := Facto-DSA
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Facto-DSA-128
SRCS := SIG_AlgorithmInstance.c auxfunc.c drng.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-10,Facto-DSA-128,sig,128,sig-10-facto-dsa-128,ngcc_ref_sig_10_Facto_DSA_128))

# Facto-DSA / Facto-DSA-256 (256-bit)
ALGORITHM := Facto-DSA
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Facto-DSA-256
SRCS := SIG_AlgorithmInstance.c auxfunc.c drng.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-10,Facto-DSA-256,sig,256,sig-10-facto-dsa-256,ngcc_ref_sig_10_Facto_DSA_256))

