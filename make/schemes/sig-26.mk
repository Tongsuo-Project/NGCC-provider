# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# SQIsign2D-push1/2 / SQIsign2D-lvl1 (128-bit)
ALGORITHM := SQIsign2D-push1/2
SRCDIR := Implementations/sqisign2d_lvl1
SRCS := src/sqisign.c src/iccs/SIG_AlgorithmInstance.c src/iccs/drng.c src/iccs/auxfunc.c         \
    src/iccs/randombytes_iccs.c src/iccs/xof_iccs.c src/common/generic/mem.c                      \
    src/common/generic/tools.c src/intbig/ref/generic/intbig.c                                    \
    src/quaternion/ref/generic/algebra.c src/quaternion/ref/generic/ideal.c                       \
    src/quaternion/ref/generic/dim4.c src/quaternion/ref/generic/dim2.c                           \
    src/quaternion/ref/generic/integers.c src/quaternion/ref/generic/lattice.c                    \
    src/quaternion/ref/generic/finit.c src/quaternion/ref/generic/printer.c                       \
    src/quaternion/ref/generic/lll.c src/quaternion/ref/generic/matkermod.c                       \
    src/klpt/ref/klptx/tools.c src/gf/ref/gfx/fp2.c src/gf/ref/gfx/fp.c src/gf/ref/gfx/mp.c       \
    src/ec/ref/ecx/ec.c src/ec/ref/ecx/kps.c src/ec/ref/ecx/xisog.c src/ec/ref/ecx/xeval.c        \
    src/ec/ref/ecx/isog_chains.c src/ec/ref/ecx/basis.c src/ec/ref/ecx/biextension.c              \
    src/id2iso/ref/id2isox/id2iso.c src/hd/ref/hdx/hd.c src/hd/ref/hdx/theta_structure.c          \
    src/hd/ref/hdx/theta_isogenies.c src/dim2id2iso/ref/dim2id2isox/dim2id2iso.c                  \
    src/sqisigndim2/ref/sqisigndim2x/sign.c src/sqisigndim2/ref/sqisigndim2x/keygen.c             \
    src/sqisigndim2/ref/sqisigndim2x/doublepath.c src/gf/ref/lvl1/fp_13178.c                      \
    src/precomp/ref/lvl1/gf_constants.c src/precomp/ref/lvl1/torsion_constants.c                  \
    src/precomp/ref/lvl1/quaternion_data.c src/precomp/ref/lvl1/endomorphism_action.c             \
    src/precomp/ref/lvl1/klpt_constants.c
INC := -Isrc/SQIsign2D-lvl1/src/iccs -Isrc/SQIsign2D-lvl1/include                                 \
    -Isrc/SQIsign2D-lvl1/src/common/generic/include                                               \
    -Isrc/SQIsign2D-lvl1/src/intbig/ref/generic/include                                           \
    -Isrc/SQIsign2D-lvl1/src/quaternion/ref/generic/include                                       \
    -Isrc/SQIsign2D-lvl1/src/precomp/ref/lvl1/include -Isrc/SQIsign2D-lvl1/src/klpt/ref/include   \
    -Isrc/SQIsign2D-lvl1/src/gf/ref/include -Isrc/SQIsign2D-lvl1/src/ec/ref/include               \
    -Isrc/SQIsign2D-lvl1/src/hd/ref/include -Isrc/SQIsign2D-lvl1/src/id2iso/ref/include           \
    -Isrc/SQIsign2D-lvl1/src/dim2id2iso/ref/include                                               \
    -Isrc/SQIsign2D-lvl1/src/sqisigndim2/ref/include
DEFS := -DNDEBUG -DTARGET_AMD64 -DRADIX_64 -DTARGET_OS_UNIX -DSQISIGN_VARIANT=lvl1 -DOUTPUT_BLANK_TEST_VECTORS=0 -DALGORITHM_INSTANCE='"SQIsign2D-lvl1"'
INSTANCE_CFLAGS := -funroll-loops
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-26,SQIsign2D-lvl1,sig,128,sig-26-sqisign2d-lvl1,ngcc_ref_sig_26_SQIsign2D_lvl1))

# SQIsign2D-push1/2 / SQIsign2D-lvl3 (256-bit)
ALGORITHM := SQIsign2D-push1/2
SRCDIR := Implementations/sqisign2d_lvl3
SRCS := src/sqisign.c src/iccs/SIG_AlgorithmInstance.c src/iccs/drng.c src/iccs/auxfunc.c         \
    src/iccs/randombytes_iccs.c src/iccs/xof_iccs.c src/common/generic/mem.c                      \
    src/common/generic/tools.c src/intbig/ref/generic/intbig.c                                    \
    src/quaternion/ref/generic/algebra.c src/quaternion/ref/generic/ideal.c                       \
    src/quaternion/ref/generic/dim4.c src/quaternion/ref/generic/dim2.c                           \
    src/quaternion/ref/generic/integers.c src/quaternion/ref/generic/lattice.c                    \
    src/quaternion/ref/generic/finit.c src/quaternion/ref/generic/printer.c                       \
    src/quaternion/ref/generic/lll.c src/quaternion/ref/generic/matkermod.c                       \
    src/klpt/ref/klptx/tools.c src/gf/ref/gfx/fp2.c src/gf/ref/gfx/fp.c src/gf/ref/gfx/mp.c       \
    src/ec/ref/ecx/ec.c src/ec/ref/ecx/kps.c src/ec/ref/ecx/xisog.c src/ec/ref/ecx/xeval.c        \
    src/ec/ref/ecx/isog_chains.c src/ec/ref/ecx/basis.c src/ec/ref/ecx/biextension.c              \
    src/id2iso/ref/id2isox/id2iso.c src/hd/ref/hdx/hd.c src/hd/ref/hdx/theta_structure.c          \
    src/hd/ref/hdx/theta_isogenies.c src/dim2id2iso/ref/dim2id2isox/dim2id2iso.c                  \
    src/sqisigndim2/ref/sqisigndim2x/sign.c src/sqisigndim2/ref/sqisigndim2x/keygen.c             \
    src/sqisigndim2/ref/sqisigndim2x/doublepath.c src/gf/ref/lvl3/fp_263156.c                     \
    src/precomp/ref/lvl3/gf_constants.c src/precomp/ref/lvl3/torsion_constants.c                  \
    src/precomp/ref/lvl3/quaternion_data.c src/precomp/ref/lvl3/endomorphism_action.c             \
    src/precomp/ref/lvl3/klpt_constants.c
INC := -Isrc/SQIsign2D-lvl3/src/iccs -Isrc/SQIsign2D-lvl3/include                                 \
    -Isrc/SQIsign2D-lvl3/src/common/generic/include                                               \
    -Isrc/SQIsign2D-lvl3/src/intbig/ref/generic/include                                           \
    -Isrc/SQIsign2D-lvl3/src/quaternion/ref/generic/include                                       \
    -Isrc/SQIsign2D-lvl3/src/precomp/ref/lvl3/include -Isrc/SQIsign2D-lvl3/src/klpt/ref/include   \
    -Isrc/SQIsign2D-lvl3/src/gf/ref/include -Isrc/SQIsign2D-lvl3/src/ec/ref/include               \
    -Isrc/SQIsign2D-lvl3/src/hd/ref/include -Isrc/SQIsign2D-lvl3/src/id2iso/ref/include           \
    -Isrc/SQIsign2D-lvl3/src/dim2id2iso/ref/include                                               \
    -Isrc/SQIsign2D-lvl3/src/sqisigndim2/ref/include
DEFS := -DNDEBUG -DTARGET_AMD64 -DRADIX_64 -DTARGET_OS_UNIX -DSQISIGN_VARIANT=lvl3 -DOUTPUT_BLANK_TEST_VECTORS=0 -DALGORITHM_INSTANCE='"SQIsign2D-lvl3"'
INSTANCE_CFLAGS := -funroll-loops
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-26,SQIsign2D-lvl3,sig,256,sig-26-sqisign2d-lvl3,ngcc_ref_sig_26_SQIsign2D_lvl3))

