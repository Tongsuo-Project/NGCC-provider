# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# QIMEN-PIKE / NGCC-1 (128-bit)
ALGORITHM := QIMEN-PIKE
SRCDIR := Implementations/Implementations
SRCS := src/ngcc/common/KEM_AlgorithmInstance.c src/ngcc/common/drng.c src/ngcc/common/auxfunc.c  \
    src/common/generic/aes_c.c src/common/generic/fips202.c src/common/generic/pike_hash.c        \
    src/common/generic/tools.c src/intbig/ref/generic/intbig.c                                    \
    src/quaternion/ref/generic/algebra.c src/quaternion/ref/generic/ideal.c                       \
    src/quaternion/ref/generic/dim4.c src/quaternion/ref/generic/dim2.c                           \
    src/quaternion/ref/generic/integers.c src/quaternion/ref/generic/lattice.c                    \
    src/quaternion/ref/generic/finit.c src/quaternion/ref/generic/printer.c                       \
    src/quaternion/ref/generic/lll.c src/quaternion/ref/generic/matkermod.c                       \
    src/klpt/ref/klptx/tools.c src/gf/ref/gfx/fp2.c src/gf/ref/gfx/mp.c src/ec/ref/pike_ecx/ec.c  \
    src/ec/ref/ecx/kps.c src/ec/ref/ecx/xisog.c src/ec/ref/ecx/xeval.c                            \
    src/ec/ref/ecx/isog_chains.c src/ec/ref/pike_ecx/basis.c src/ec/ref/ecx/biextension.c         \
    src/hd/ref/hdx/hd.c src/hd/ref/hdx/theta_structure.c src/hd/ref/hdx/theta_isogenies.c         \
    src/id2iso/ref/id2isox/id2iso.c src/pike/ref/pikex_compressed/pike_compressed.c               \
    src/gf/ref/NGCC_1/fp_p479.c src/precomp/ref/NGCC_1/gf_constants.c                             \
    src/precomp/ref/NGCC_1/torsion_constants.c src/precomp/ref/NGCC_1/endomorphism_action.c       \
    src/precomp/ref/NGCC_1/klpt_constants.c src/precomp/ref/NGCC_1/quaternion_data.c
INC := -Isrc/NGCC-1/src/ngcc/common -Isrc/NGCC-1/include -Isrc/NGCC-1/src/common/generic/include  \
    -Isrc/NGCC-1/src/intbig/ref/generic/include -Isrc/NGCC-1/src/quaternion/ref/generic/include   \
    -Isrc/NGCC-1/src/precomp/ref/NGCC_1/include -Isrc/NGCC-1/src/klpt/ref/include                 \
    -Isrc/NGCC-1/src/gf/ref/include -Isrc/NGCC-1/src/ec/ref/include                               \
    -Isrc/NGCC-1/src/hd/ref/include -Isrc/NGCC-1/src/id2iso/ref/include                           \
    -Isrc/NGCC-1/src/pike/ref/include -Isrc/NGCC-1/src/pike/ref/pikex_compressed/include
DEFS := -DALGORITHM_INSTANCE='"NGCC-1"' -DNGCC_PIKE_LEVEL=1 -DOUTPUT_BLANK_TEST_VECTORS=0 -DPIKE_XOF_BACKEND=2 -DTARGET_AMD64 -DRADIX_64 -DTARGET_OS_UNIX
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-31,NGCC-1,kem,128,kem-31-ngcc-1,ngcc_ref_kem_31_NGCC_1))

# QIMEN-PIKE / NGCC-2 (256-bit)
ALGORITHM := QIMEN-PIKE
SRCDIR := Implementations/Implementations
SRCS := src/ngcc/common/KEM_AlgorithmInstance.c src/ngcc/common/drng.c src/ngcc/common/auxfunc.c  \
    src/common/generic/aes_c.c src/common/generic/fips202.c src/common/generic/pike_hash.c        \
    src/common/generic/tools.c src/intbig/ref/generic/intbig.c                                    \
    src/quaternion/ref/generic/algebra.c src/quaternion/ref/generic/ideal.c                       \
    src/quaternion/ref/generic/dim4.c src/quaternion/ref/generic/dim2.c                           \
    src/quaternion/ref/generic/integers.c src/quaternion/ref/generic/lattice.c                    \
    src/quaternion/ref/generic/finit.c src/quaternion/ref/generic/printer.c                       \
    src/quaternion/ref/generic/lll.c src/quaternion/ref/generic/matkermod.c                       \
    src/klpt/ref/klptx/tools.c src/gf/ref/gfx/fp2.c src/gf/ref/gfx/mp.c src/ec/ref/pike_ecx/ec.c  \
    src/ec/ref/ecx/kps.c src/ec/ref/ecx/xisog.c src/ec/ref/ecx/xeval.c                            \
    src/ec/ref/ecx/isog_chains.c src/ec/ref/pike_ecx/basis.c src/ec/ref/ecx/biextension.c         \
    src/hd/ref/hdx/hd.c src/hd/ref/hdx/theta_structure.c src/hd/ref/hdx/theta_isogenies.c         \
    src/id2iso/ref/id2isox/id2iso.c src/pike/ref/pikex_compressed/pike_compressed.c               \
    src/gf/ref/NGCC_2/fp_p765.c src/precomp/ref/NGCC_2/gf_constants.c                             \
    src/precomp/ref/NGCC_2/torsion_constants.c src/precomp/ref/NGCC_2/endomorphism_action.c       \
    src/precomp/ref/NGCC_2/klpt_constants.c src/precomp/ref/NGCC_2/quaternion_data.c
INC := -Isrc/NGCC-2/src/ngcc/common -Isrc/NGCC-2/include -Isrc/NGCC-2/src/common/generic/include  \
    -Isrc/NGCC-2/src/intbig/ref/generic/include -Isrc/NGCC-2/src/quaternion/ref/generic/include   \
    -Isrc/NGCC-2/src/precomp/ref/NGCC_2/include -Isrc/NGCC-2/src/klpt/ref/include                 \
    -Isrc/NGCC-2/src/gf/ref/include -Isrc/NGCC-2/src/ec/ref/include                               \
    -Isrc/NGCC-2/src/hd/ref/include -Isrc/NGCC-2/src/id2iso/ref/include                           \
    -Isrc/NGCC-2/src/pike/ref/include -Isrc/NGCC-2/src/pike/ref/pikex_compressed/include
DEFS := -DALGORITHM_INSTANCE='"NGCC-2"' -DNGCC_PIKE_LEVEL=2 -DOUTPUT_BLANK_TEST_VECTORS=0 -DPIKE_XOF_BACKEND=2 -DTARGET_AMD64 -DRADIX_64 -DTARGET_OS_UNIX
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,kem-31,NGCC-2,kem,256,kem-31-ngcc-2,ngcc_ref_kem_31_NGCC_2))

