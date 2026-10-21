# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# DOVE / dove_classic_128 (128-bit)
ALGORITHM := DOVE
SRCDIR := Implementations/Digital_Signature-DOVE-x86-reference_implementation/API_PKC/Implementations/Reference_Implementation/DOVE_classic_ref
SRCS := gf256.c matrix.c uov/blas_matrix_ref.c auxfunc.c drng.c SIG_AlgorithmInstance.c
INC := -Isrc/dove_classic_128/uov
DEFS := -DDOVE_PARAM_SET=DOVE128
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE='"dove_classic_128"'
NOAUX := 
$(eval $(call ngcc_instance,sign-09,dove_classic_128,sig,128,sig-09-dove-classic-128,ngcc_ref_sig_09_dove_classic_128))

# DOVE / dove_classic_256 (256-bit)
ALGORITHM := DOVE
SRCDIR := Implementations/Digital_Signature-DOVE-x86-reference_implementation/API_PKC/Implementations/Reference_Implementation/DOVE_classic_ref
SRCS := gf256.c matrix.c uov/blas_matrix_ref.c auxfunc.c drng.c SIG_AlgorithmInstance.c
INC := -Isrc/dove_classic_256/uov
DEFS := -DDOVE_PARAM_SET=DOVE256
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE='"dove_classic_256"'
NOAUX := 
$(eval $(call ngcc_instance,sign-09,dove_classic_256,sig,256,sig-09-dove-classic-256,ngcc_ref_sig_09_dove_classic_256))

# DOVE / dove_pkc_skc_128 (128-bit)
ALGORITHM := DOVE
SRCDIR := Implementations/Digital_Signature-DOVE-x86-reference_implementation/API_PKC/Implementations/Reference_Implementation/DOVE_pkc_skc_ref
SRCS := gf256.c matrix.c uov/blas_matrix_ref.c auxfunc.c drng.c SIG_AlgorithmInstance.c
INC := -Isrc/dove_pkc_skc_128/uov
DEFS := -DDOVE_PARAM_SET=DOVE128
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE='"dove_pkc_skc_128"'
NOAUX := 
$(eval $(call ngcc_instance,sign-09,dove_pkc_skc_128,sig,128,sig-09-dove-pkc-skc-128,ngcc_ref_sig_09_dove_pkc_skc_128))

# DOVE / dove_pkc_skc_256 (256-bit)
ALGORITHM := DOVE
SRCDIR := Implementations/Digital_Signature-DOVE-x86-reference_implementation/API_PKC/Implementations/Reference_Implementation/DOVE_pkc_skc_ref
SRCS := gf256.c matrix.c uov/blas_matrix_ref.c auxfunc.c drng.c SIG_AlgorithmInstance.c
INC := -Isrc/dove_pkc_skc_256/uov
DEFS := -DDOVE_PARAM_SET=DOVE256
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE='"dove_pkc_skc_256"'
NOAUX := 
$(eval $(call ngcc_instance,sign-09,dove_pkc_skc_256,sig,256,sig-09-dove-pkc-skc-256,ngcc_ref_sig_09_dove_pkc_skc_256))

