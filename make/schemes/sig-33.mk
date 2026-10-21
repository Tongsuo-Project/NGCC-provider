# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# VDOO / vdoo_128 (128-bit)
ALGORITHM := VDOO
SRCDIR := Implementation/Reference_Implementation/vdoo_128
SRCS := auxfunc.c blas_comm.c drng.c parallel_matrix_op.c rng.c utils.c api.c                     \
    SIG_AlgorithmInstance.c vdoo_keypair.c vdoo_sign.c vdoo_verif.c
INC := 
DEFS := -DVDOO_128
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-33,vdoo_128,sig,128,sig-33-vdoo-128,ngcc_ref_sig_33_vdoo_128))

# VDOO / vdoo_256 (256-bit)
ALGORITHM := VDOO
SRCDIR := Implementation/Reference_Implementation/vdoo_256
SRCS := auxfunc.c blas_comm.c drng.c parallel_matrix_op.c rng.c utils.c api.c                     \
    SIG_AlgorithmInstance.c vdoo_keypair.c vdoo_sign.c vdoo_verif.c
INC := 
DEFS := -DVDOO_256
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-33,vdoo_256,sig,256,sig-33-vdoo-256,ngcc_ref_sig_33_vdoo_256))

