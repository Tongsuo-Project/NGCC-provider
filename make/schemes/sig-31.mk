# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# TSUOV / TSUOV_128 (128-bit)
ALGORITHM := TSUOV
SRCDIR := Implementations/Digital_Signature-TSUOV-x86-Reference_Implementation/API_PKC/Implementations/Reference_Implementation/TSUOV_128
SRCS := Fql.c SIG_AlgorithmInstance.c auxfunc.c drng.c mgf.c tsuov_core.c
INC := 
DEFS := -DTSUOV_VARIANT=128
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-31,TSUOV_128,sig,128,sig-31-tsuov-128,ngcc_ref_sig_31_TSUOV_128))

# TSUOV / TSUOV_256 (256-bit)
ALGORITHM := TSUOV
SRCDIR := Implementations/Digital_Signature-TSUOV-x86-Reference_Implementation/API_PKC/Implementations/Reference_Implementation/TSUOV_256
SRCS := Fql.c SIG_AlgorithmInstance.c auxfunc.c drng.c mgf.c tsuov_core.c
INC := 
DEFS := -DTSUOV_VARIANT=256
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-31,TSUOV_256,sig,256,sig-31-tsuov-256,ngcc_ref_sig_31_TSUOV_256))

