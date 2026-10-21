# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Galas / Galas-160F (160-bit)
ALGORITHM := Galas
SRCDIR := Implementations/Reference_Implementation/Galas-160F
SRCS := galas/gf2n.c galas/owf.c galas/bf.c galas/xof.c galas/bavc.c galas/vole.c                 \
    galas/instances.c galas/universal_hashing.c galas/vole_check_ref.c galas/qs_proof_ref.c       \
    galas/quicksilver.c galas/qs_gadgets.c galas/random_oracle.c galas/sign_helpers.c             \
    galas/challenge.c galas/qs_constraint.c galas/qs_full_constraints.c ngcc/auxfunc.c            \
    ngcc/drng.c ngcc/SIG_AlgorithmInstance.c
INC := -Isrc/Galas-160F/galas -Isrc/Galas-160F/ngcc
DEFS := -DGALAS_INSTANCE=GALAS_160F -DGALAS_INSTANCE_NAME='"Galas-160F"'
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"ngcc/SIG_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,sign-12,Galas-160F,sig,160,sig-12-galas-160f,ngcc_ref_sig_12_Galas_160F))

# Galas / Galas-160S (160-bit)
ALGORITHM := Galas
SRCDIR := Implementations/Reference_Implementation/Galas-160S
SRCS := galas/gf2n.c galas/owf.c galas/bf.c galas/xof.c galas/bavc.c galas/vole.c                 \
    galas/instances.c galas/universal_hashing.c galas/vole_check_ref.c galas/qs_proof_ref.c       \
    galas/quicksilver.c galas/qs_gadgets.c galas/random_oracle.c galas/sign_helpers.c             \
    galas/challenge.c galas/qs_constraint.c galas/qs_full_constraints.c ngcc/auxfunc.c            \
    ngcc/drng.c ngcc/SIG_AlgorithmInstance.c
INC := -Isrc/Galas-160S/galas -Isrc/Galas-160S/ngcc
DEFS := -DGALAS_INSTANCE=GALAS_160S -DGALAS_INSTANCE_NAME='"Galas-160S"'
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"ngcc/SIG_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,sign-12,Galas-160S,sig,160,sig-12-galas-160s,ngcc_ref_sig_12_Galas_160S))

# Galas / Galas-256F (256-bit)
ALGORITHM := Galas
SRCDIR := Implementations/Reference_Implementation/Galas-256F
SRCS := galas/gf2n.c galas/owf.c galas/bf.c galas/xof.c galas/bavc.c galas/vole.c                 \
    galas/instances.c galas/universal_hashing.c galas/vole_check_ref.c galas/qs_proof_ref.c       \
    galas/quicksilver.c galas/qs_gadgets.c galas/random_oracle.c galas/sign_helpers.c             \
    galas/challenge.c galas/qs_constraint.c galas/qs_full_constraints.c ngcc/auxfunc.c            \
    ngcc/drng.c ngcc/SIG_AlgorithmInstance.c
INC := -Isrc/Galas-256F/galas -Isrc/Galas-256F/ngcc
DEFS := -DGALAS_INSTANCE=GALAS_256F -DGALAS_INSTANCE_NAME='"Galas-256F"'
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"ngcc/SIG_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,sign-12,Galas-256F,sig,256,sig-12-galas-256f,ngcc_ref_sig_12_Galas_256F))

# Galas / Galas-256S (256-bit)
ALGORITHM := Galas
SRCDIR := Implementations/Reference_Implementation/Galas-256S
SRCS := galas/gf2n.c galas/owf.c galas/bf.c galas/xof.c galas/bavc.c galas/vole.c                 \
    galas/instances.c galas/universal_hashing.c galas/vole_check_ref.c galas/qs_proof_ref.c       \
    galas/quicksilver.c galas/qs_gadgets.c galas/random_oracle.c galas/sign_helpers.c             \
    galas/challenge.c galas/qs_constraint.c galas/qs_full_constraints.c ngcc/auxfunc.c            \
    ngcc/drng.c ngcc/SIG_AlgorithmInstance.c
INC := -Isrc/Galas-256S/galas -Isrc/Galas-256S/ngcc
DEFS := -DGALAS_INSTANCE=GALAS_256S -DGALAS_INSTANCE_NAME='"Galas-256S"'
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"ngcc/SIG_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,sign-12,Galas-256S,sig,256,sig-12-galas-256s,ngcc_ref_sig_12_Galas_256S))

