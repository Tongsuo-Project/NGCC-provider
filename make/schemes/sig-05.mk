# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Chinith / sm4th_d3_128f_loose (128-bit)
ALGORITHM := Chinith
SRCDIR := Implementations/Reference_Implementation/sm4th_d3_128f_loose
SRCS := auxfunc.c bavc.c compat.c drng.c fields.c params.c prg.c random_oracle.c sig_impl.c       \
    universal_hashing.c utils.c vole.c owf.c sm4_witness.c sm4th_sm4_128.c utils_sm4/sm4_affine.c \
    utils_sm4/sm4_core.c utils_sm4/sm4_ecb.c utils_sm4/sm4_sbox.c sig_timing.c                    \
    sm4th_d3_128f_loose.c sm4th_d3_128f_loose_AlgorithmInstance.c
INC := -Isrc/sm4th_d3_128f_loose/utils_sm4
DEFS := -DNDEBUG -DSM4_SBOX_TABLE
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sm4th_d3_128f_loose_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,sign-05,sm4th_d3_128f_loose,sig,128,sig-05-sm4th-d3-128f-loose,ngcc_ref_sig_05_sm4th_d3_128f_loose))

# Chinith / sm4th_d3_128f_tight (128-bit)
ALGORITHM := Chinith
SRCDIR := Implementations/Reference_Implementation/sm4th_d3_128f_tight
SRCS := auxfunc.c bavc.c compat.c drng.c fields.c params.c prg.c random_oracle.c sig_impl.c       \
    universal_hashing.c utils.c vole.c owf.c sm4_witness.c sm4th_sm4_128.c utils_sm4/sm4_affine.c \
    utils_sm4/sm4_core.c utils_sm4/sm4_ecb.c utils_sm4/sm4_sbox.c sig_timing.c                    \
    utils_ballet/Ballet256256_coreE.c sm4th_d3_128f_tight.c                                       \
    sm4th_d3_128f_tight_AlgorithmInstance.c
INC := -Isrc/sm4th_d3_128f_tight/utils_sm4 -Isrc/sm4th_d3_128f_tight/utils_ballet
DEFS := -DNDEBUG -DSM4_SBOX_TABLE -DUSE_BALLET=1 -DHAVE_BALLET_CORE=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sm4th_d3_128f_tight_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,sign-05,sm4th_d3_128f_tight,sig,128,sig-05-sm4th-d3-128f-tight,ngcc_ref_sig_05_sm4th_d3_128f_tight))

# Chinith / sm4th_d3_128s_loose (128-bit)
ALGORITHM := Chinith
SRCDIR := Implementations/Reference_Implementation/sm4th_d3_128s_loose
SRCS := auxfunc.c bavc.c compat.c drng.c fields.c params.c prg.c random_oracle.c sig_impl.c       \
    universal_hashing.c utils.c vole.c owf.c sm4_witness.c sm4th_sm4_128.c utils_sm4/sm4_affine.c \
    utils_sm4/sm4_core.c utils_sm4/sm4_ecb.c utils_sm4/sm4_sbox.c sig_timing.c                    \
    sm4th_d3_128s_loose.c sm4th_d3_128s_loose_AlgorithmInstance.c
INC := -Isrc/sm4th_d3_128s_loose/utils_sm4
DEFS := -DNDEBUG -DSM4_SBOX_TABLE
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sm4th_d3_128s_loose_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,sign-05,sm4th_d3_128s_loose,sig,128,sig-05-sm4th-d3-128s-loose,ngcc_ref_sig_05_sm4th_d3_128s_loose))

# Chinith / sm4th_d3_128s_tight (128-bit)
ALGORITHM := Chinith
SRCDIR := Implementations/Reference_Implementation/sm4th_d3_128s_tight
SRCS := auxfunc.c bavc.c compat.c drng.c fields.c params.c prg.c random_oracle.c sig_impl.c       \
    universal_hashing.c utils.c vole.c owf.c sm4_witness.c sm4th_sm4_128.c utils_sm4/sm4_affine.c \
    utils_sm4/sm4_core.c utils_sm4/sm4_ecb.c utils_sm4/sm4_sbox.c sig_timing.c                    \
    utils_ballet/Ballet256256_coreE.c sm4th_d3_128s_tight.c                                       \
    sm4th_d3_128s_tight_AlgorithmInstance.c
INC := -Isrc/sm4th_d3_128s_tight/utils_sm4 -Isrc/sm4th_d3_128s_tight/utils_ballet
DEFS := -DNDEBUG -DSM4_SBOX_TABLE -DUSE_BALLET=1 -DHAVE_BALLET_CORE=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sm4th_d3_128s_tight_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,sign-05,sm4th_d3_128s_tight,sig,128,sig-05-sm4th-d3-128s-tight,ngcc_ref_sig_05_sm4th_d3_128s_tight))

# Chinith / sm4th_em_d2_128f_loose (128-bit)
ALGORITHM := Chinith
SRCDIR := Implementations/Reference_Implementation/sm4th_em_d2_128f_loose
SRCS := auxfunc.c bavc.c compat.c drng.c fields.c params.c prg.c random_oracle.c sig_impl.c       \
    universal_hashing.c utils.c vole.c owf.c sm4_witness.c sm4th_sm4_128.c utils_sm4/sm4_affine.c \
    utils_sm4/sm4_core.c utils_sm4/sm4_ecb.c utils_sm4/sm4_sbox.c sm4th_em_d2_128f_loose.c        \
    sm4th_em_d2_128f_loose_AlgorithmInstance.c
INC := -Isrc/sm4th_em_d2_128f_loose/utils_sm4
DEFS := -DNDEBUG -DSM4_SBOX_TABLE
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sm4th_em_d2_128f_loose_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,sign-05,sm4th_em_d2_128f_loose,sig,128,sig-05-sm4th-em-d2-128f-loose,ngcc_ref_sig_05_sm4th_em_d2_128f_loose))

# Chinith / sm4th_em_d2_128f_tight (128-bit)
ALGORITHM := Chinith
SRCDIR := Implementations/Reference_Implementation/sm4th_em_d2_128f_tight
SRCS := auxfunc.c bavc.c compat.c drng.c fields.c params.c prg.c random_oracle.c sig_impl.c       \
    universal_hashing.c utils.c vole.c owf.c sm4_witness.c sm4th_sm4_128.c utils_sm4/sm4_affine.c \
    utils_sm4/sm4_core.c utils_sm4/sm4_ecb.c utils_sm4/sm4_sbox.c                                 \
    utils_ballet/Ballet256256_coreE.c sm4th_em_d2_128f_tight.c                                    \
    sm4th_em_d2_128f_tight_AlgorithmInstance.c
INC := -Isrc/sm4th_em_d2_128f_tight/utils_sm4 -Isrc/sm4th_em_d2_128f_tight/utils_ballet
DEFS := -DNDEBUG -DSM4_SBOX_TABLE -DUSE_BALLET=1 -DHAVE_BALLET_CORE=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sm4th_em_d2_128f_tight_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,sign-05,sm4th_em_d2_128f_tight,sig,128,sig-05-sm4th-em-d2-128f-tight,ngcc_ref_sig_05_sm4th_em_d2_128f_tight))

# Chinith / sm4th_em_d2_128s_loose (128-bit)
ALGORITHM := Chinith
SRCDIR := Implementations/Reference_Implementation/sm4th_em_d2_128s_loose
SRCS := auxfunc.c bavc.c compat.c drng.c fields.c params.c prg.c random_oracle.c sig_impl.c       \
    universal_hashing.c utils.c vole.c owf.c sm4_witness.c sm4th_sm4_128.c utils_sm4/sm4_affine.c \
    utils_sm4/sm4_core.c utils_sm4/sm4_ecb.c utils_sm4/sm4_sbox.c sm4th_em_d2_128s_loose.c        \
    sm4th_em_d2_128s_loose_AlgorithmInstance.c
INC := -Isrc/sm4th_em_d2_128s_loose/utils_sm4
DEFS := -DNDEBUG -DSM4_SBOX_TABLE
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sm4th_em_d2_128s_loose_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,sign-05,sm4th_em_d2_128s_loose,sig,128,sig-05-sm4th-em-d2-128s-loose,ngcc_ref_sig_05_sm4th_em_d2_128s_loose))

# Chinith / sm4th_em_d2_128s_tight (128-bit)
ALGORITHM := Chinith
SRCDIR := Implementations/Reference_Implementation/sm4th_em_d2_128s_tight
SRCS := auxfunc.c bavc.c compat.c drng.c fields.c params.c prg.c random_oracle.c sig_impl.c       \
    universal_hashing.c utils.c vole.c owf.c sm4_witness.c sm4th_sm4_128.c utils_sm4/sm4_affine.c \
    utils_sm4/sm4_core.c utils_sm4/sm4_ecb.c utils_sm4/sm4_sbox.c                                 \
    utils_ballet/Ballet256256_coreE.c sm4th_em_d2_128s_tight.c                                    \
    sm4th_em_d2_128s_tight_AlgorithmInstance.c
INC := -Isrc/sm4th_em_d2_128s_tight/utils_sm4 -Isrc/sm4th_em_d2_128s_tight/utils_ballet
DEFS := -DNDEBUG -DSM4_SBOX_TABLE -DUSE_BALLET=1 -DHAVE_BALLET_CORE=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sm4th_em_d2_128s_tight_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,sign-05,sm4th_em_d2_128s_tight,sig,128,sig-05-sm4th-em-d2-128s-tight,ngcc_ref_sig_05_sm4th_em_d2_128s_tight))

# Chinith / ublockith_d3_256f (256-bit)
ALGORITHM := Chinith
SRCDIR := Implementations/Reference_Implementation/ublockith_d3_256f
SRCS := auxfunc.c bavc.c compat.c drng.c fields.c params.c prg.c random_oracle.c sig_impl.c       \
    universal_hashing.c utils.c vole.c owf.c utils_ublock/sig_impl_ublock.c                       \
    utils_ublock/ublock_affine.c utils_ublock/ublock_constraints.c utils_ublock/ublock_core.c     \
    utils_ublock/ublock_ecb.c utils_ublock/ublock_witness.c utils_ublock/ublockith_ublock_256.c   \
    utils_ballet/Ballet256256_coreE.c ublockith_d3_256f.c ublockith_d3_256f_AlgorithmInstance.c
INC := -Isrc/ublockith_d3_256f/utils_ublock -Isrc/ublockith_d3_256f/utils_ballet
DEFS := -DNDEBUG -DUSE_BALLET=1 -DHAVE_BALLET_CORE=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"ublockith_d3_256f_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,sign-05,ublockith_d3_256f,sig,256,sig-05-ublockith-d3-256f,ngcc_ref_sig_05_ublockith_d3_256f))

# Chinith / ublockith_d3_256s (256-bit)
ALGORITHM := Chinith
SRCDIR := Implementations/Reference_Implementation/ublockith_d3_256s
SRCS := auxfunc.c bavc.c compat.c drng.c fields.c params.c prg.c random_oracle.c sig_impl.c       \
    universal_hashing.c utils.c vole.c owf.c utils_ublock/sig_impl_ublock.c                       \
    utils_ublock/ublock_affine.c utils_ublock/ublock_constraints.c utils_ublock/ublock_core.c     \
    utils_ublock/ublock_ecb.c utils_ublock/ublock_witness.c utils_ublock/ublockith_ublock_256.c   \
    utils_ballet/Ballet256256_coreE.c ublockith_d3_256s.c ublockith_d3_256s_AlgorithmInstance.c
INC := -Isrc/ublockith_d3_256s/utils_ublock -Isrc/ublockith_d3_256s/utils_ballet
DEFS := -DNDEBUG -DUSE_BALLET=1 -DHAVE_BALLET_CORE=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"ublockith_d3_256s_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,sign-05,ublockith_d3_256s,sig,256,sig-05-ublockith-d3-256s,ngcc_ref_sig_05_ublockith_d3_256s))

# Chinith / ublockith_em_d3_256f (256-bit)
ALGORITHM := Chinith
SRCDIR := Implementations/Reference_Implementation/ublockith_em_d3_256f
SRCS := auxfunc.c bavc.c compat.c drng.c fields.c params.c prg.c random_oracle.c sig_impl.c       \
    universal_hashing.c utils.c vole.c owf.c utils_ublock/sig_impl_ublock.c                       \
    utils_ublock/ublock_affine.c utils_ublock/ublock_constraints.c utils_ublock/ublock_core.c     \
    utils_ublock/ublock_ecb.c utils_ublock/ublock_witness.c utils_ublock/ublockith_ublock_256.c   \
    utils_ballet/Ballet256256_coreE.c ublockith_em_d3_256f.c                                      \
    ublockith_em_d3_256f_AlgorithmInstance.c
INC := -Isrc/ublockith_em_d3_256f/utils_ublock -Isrc/ublockith_em_d3_256f/utils_ballet
DEFS := -DNDEBUG -DUSE_BALLET=1 -DHAVE_BALLET_CORE=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"ublockith_em_d3_256f_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,sign-05,ublockith_em_d3_256f,sig,256,sig-05-ublockith-em-d3-256f,ngcc_ref_sig_05_ublockith_em_d3_256f))

# Chinith / ublockith_em_d3_256s (256-bit)
ALGORITHM := Chinith
SRCDIR := Implementations/Reference_Implementation/ublockith_em_d3_256s
SRCS := auxfunc.c bavc.c compat.c drng.c fields.c params.c prg.c random_oracle.c sig_impl.c       \
    universal_hashing.c utils.c vole.c owf.c utils_ublock/sig_impl_ublock.c                       \
    utils_ublock/ublock_affine.c utils_ublock/ublock_constraints.c utils_ublock/ublock_core.c     \
    utils_ublock/ublock_ecb.c utils_ublock/ublock_witness.c utils_ublock/ublockith_ublock_256.c   \
    utils_ballet/Ballet256256_coreE.c ublockith_em_d3_256s.c                                      \
    ublockith_em_d3_256s_AlgorithmInstance.c
INC := -Isrc/ublockith_em_d3_256s/utils_ublock -Isrc/ublockith_em_d3_256s/utils_ballet
DEFS := -DNDEBUG -DUSE_BALLET=1 -DHAVE_BALLET_CORE=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"ublockith_em_d3_256s_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,sign-05,ublockith_em_d3_256s,sig,256,sig-05-ublockith-em-d3-256s,ngcc_ref_sig_05_ublockith_em_d3_256s))

