# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Sigurd / Sigurd-128 (128-bit)
ALGORITHM := Sigurd
SRCDIR := Implementations/Reference_Implementation/Sigurd-128
SRCS := SIG_AlgorithmInstance.c auxfunc.c drng.c rsencode.c rsencode_common.c sig_core.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-24,Sigurd-128,sig,128,sig-24-sigurd-128,ngcc_ref_sig_24_Sigurd_128))

# Sigurd / Sigurd-256 (256-bit)
ALGORITHM := Sigurd
SRCDIR := Implementations/Reference_Implementation/Sigurd-256
SRCS := SIG_AlgorithmInstance.c auxfunc.c drng.c rsencode.c rsencode_common.c sig_core.c
INC := 
DEFS := 
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-24,Sigurd-256,sig,256,sig-24-sigurd-256,ngcc_ref_sig_24_Sigurd_256))

