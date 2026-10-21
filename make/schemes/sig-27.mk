# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# SQIsignTriangle / SQIsignTriangle_lvl1 (128-bit)
ALGORITHM := SQIsignTriangle
SRCDIR := Implementations/Reference_Implementation/SQIsignTriangle_lvl1
SRCS := SIG_AlgorithmInstance.c drng.c auxfunc.c sqisigntriangle/src/sqisign.c                    \
    sqisigntriangle/src/common/generic/fips202.c sqisigntriangle/src/common/generic/mem.c         \
    sqisigntriangle/src/common/generic/tools.c                                                    \
    sqisigntriangle/src/quaternion/ref/generic/intbig.c                                           \
    sqisigntriangle/src/quaternion/ref/generic/algebra.c                                          \
    sqisigntriangle/src/quaternion/ref/generic/ideal.c                                            \
    sqisigntriangle/src/quaternion/ref/generic/dim4.c                                             \
    sqisigntriangle/src/quaternion/ref/generic/dim2.c                                             \
    sqisigntriangle/src/quaternion/ref/generic/integers.c                                         \
    sqisigntriangle/src/quaternion/ref/generic/lattice.c                                          \
    sqisigntriangle/src/quaternion/ref/generic/lat_ball.c                                         \
    sqisigntriangle/src/quaternion/ref/generic/finit.c                                            \
    sqisigntriangle/src/quaternion/ref/generic/printer.c                                          \
    sqisigntriangle/src/quaternion/ref/generic/lll/l2.c                                           \
    sqisigntriangle/src/quaternion/ref/generic/lll/lll_verification.c                             \
    sqisigntriangle/src/quaternion/ref/generic/lll/lll_applications.c                             \
    sqisigntriangle/src/quaternion/ref/generic/rationals.c                                        \
    sqisigntriangle/src/quaternion/ref/generic/normeq.c                                           \
    sqisigntriangle/src/quaternion/ref/generic/qlapoti.c                                          \
    sqisigntriangle/src/quaternion/ref/generic/hnf/ibz_division.c                                 \
    sqisigntriangle/src/quaternion/ref/generic/hnf/hnf_internal.c                                 \
    sqisigntriangle/src/quaternion/ref/generic/hnf/hnf.c                                          \
    sqisigntriangle/src/quaternion/ref/generic/test/random_input_generation.c                     \
    sqisigntriangle/src/mp/ref/generic/mp.c sqisigntriangle/src/gf/ref/lvlx/fp.c                  \
    sqisigntriangle/src/gf/ref/lvlx/fp2.c sqisigntriangle/src/ec/ref/lvlx/ec.c                    \
    sqisigntriangle/src/ec/ref/lvlx/ec_jac.c sqisigntriangle/src/ec/ref/lvlx/xisog.c              \
    sqisigntriangle/src/ec/ref/lvlx/xeval.c sqisigntriangle/src/ec/ref/lvlx/isog_chains.c         \
    sqisigntriangle/src/ec/ref/lvlx/basis.c sqisigntriangle/src/ec/ref/lvlx/biextension.c         \
    sqisigntriangle/src/hd/ref/lvlx/hd.c sqisigntriangle/src/hd/ref/lvlx/theta_structure.c        \
    sqisigntriangle/src/hd/ref/lvlx/theta_isogenies.c                                             \
    sqisigntriangle/src/verification/ref/lvlx/common.c                                            \
    sqisigntriangle/src/verification/ref/lvlx/encode_verification.c                               \
    sqisigntriangle/src/verification/ref/lvlx/verify.c                                            \
    sqisigntriangle/src/id2iso/ref/lvlx/id2iso.c sqisigntriangle/src/id2iso/ref/lvlx/dim2id2iso.c \
    sqisigntriangle/src/signature/ref/lvlx/encode_signature.c                                     \
    sqisigntriangle/src/signature/ref/lvlx/keygen.c sqisigntriangle/src/signature/ref/lvlx/sign.c \
    sqisigntriangle/src/nistapi/lvl1/api.c sqisigntriangle/src/gf/ref/lvl1/fp_p5248_64.c          \
    sqisigntriangle/src/precomp/ref/lvl1/ec_params.c                                              \
    sqisigntriangle/src/precomp/ref/lvl1/e0_basis.c                                               \
    sqisigntriangle/src/precomp/ref/lvl1/hd_splitting_transforms.c                                \
    sqisigntriangle/src/precomp/ref/lvl1/torsion_constants.c                                      \
    sqisigntriangle/src/precomp/ref/lvl1/quaternion_data.c                                        \
    sqisigntriangle/src/precomp/ref/lvl1/endomorphism_action.c
INC := -Isrc/SQIsignTriangle_lvl1/sqisigntriangle/include                                         \
    -Isrc/SQIsignTriangle_lvl1/sqisigntriangle/src/nistapi/lvl1                                   \
    -Isrc/SQIsignTriangle_lvl1/sqisigntriangle/src/common/generic/include                         \
    -Isrc/SQIsignTriangle_lvl1/sqisigntriangle/src/common/generic                                 \
    -Isrc/SQIsignTriangle_lvl1/sqisigntriangle/src/quaternion/ref/generic/include                 \
    -Isrc/SQIsignTriangle_lvl1/sqisigntriangle/src/quaternion/ref/generic/internal_quaternion_headers\
    -Isrc/SQIsignTriangle_lvl1/sqisigntriangle/src/quaternion/ref/generic                         \
    -Isrc/SQIsignTriangle_lvl1/sqisigntriangle/src/mp/ref/generic/include                         \
    -Isrc/SQIsignTriangle_lvl1/sqisigntriangle/src/gf/ref/include                                 \
    -Isrc/SQIsignTriangle_lvl1/sqisigntriangle/src/precomp/ref/lvl1/include                       \
    -Isrc/SQIsignTriangle_lvl1/sqisigntriangle/src/ec/ref/include                                 \
    -Isrc/SQIsignTriangle_lvl1/sqisigntriangle/src/hd/ref/include                                 \
    -Isrc/SQIsignTriangle_lvl1/sqisigntriangle/src/hd/ref/lvlx                                    \
    -Isrc/SQIsignTriangle_lvl1/sqisigntriangle/src/id2iso/ref/include                             \
    -Isrc/SQIsignTriangle_lvl1/sqisigntriangle/src/verification/ref/include                       \
    -Isrc/SQIsignTriangle_lvl1/sqisigntriangle/src/signature/ref/include
DEFS := -DSQISIGN_BUILD_TYPE_REF -DSQISIGN_GF_IMPL_REF -DENABLE_SIGN -DTARGET_AMD64 -DRADIX_64 -DHAVE_UINT128 -DTARGET_OS_UNIX -DSQISIGN_TEST_REPS=10 -DSQISIGN_VARIANT=lvl1
INSTANCE_CFLAGS := -funroll-loops
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-27,SQIsignTriangle_lvl1,sig,128,sig-27-sqisigntriangle-lvl1,ngcc_ref_sig_27_SQIsignTriangle_lvl1))

# SQIsignTriangle / SQIsignTriangle_lvl2 (160-bit)
ALGORITHM := SQIsignTriangle
SRCDIR := Implementations/Reference_Implementation/SQIsignTriangle_lvl2
SRCS := SIG_AlgorithmInstance.c drng.c auxfunc.c sqisigntriangle/src/sqisign.c                    \
    sqisigntriangle/src/common/generic/fips202.c sqisigntriangle/src/common/generic/mem.c         \
    sqisigntriangle/src/common/generic/tools.c                                                    \
    sqisigntriangle/src/quaternion/ref/generic/intbig.c                                           \
    sqisigntriangle/src/quaternion/ref/generic/algebra.c                                          \
    sqisigntriangle/src/quaternion/ref/generic/ideal.c                                            \
    sqisigntriangle/src/quaternion/ref/generic/dim4.c                                             \
    sqisigntriangle/src/quaternion/ref/generic/dim2.c                                             \
    sqisigntriangle/src/quaternion/ref/generic/integers.c                                         \
    sqisigntriangle/src/quaternion/ref/generic/lattice.c                                          \
    sqisigntriangle/src/quaternion/ref/generic/lat_ball.c                                         \
    sqisigntriangle/src/quaternion/ref/generic/finit.c                                            \
    sqisigntriangle/src/quaternion/ref/generic/printer.c                                          \
    sqisigntriangle/src/quaternion/ref/generic/lll/l2.c                                           \
    sqisigntriangle/src/quaternion/ref/generic/lll/lll_verification.c                             \
    sqisigntriangle/src/quaternion/ref/generic/lll/lll_applications.c                             \
    sqisigntriangle/src/quaternion/ref/generic/rationals.c                                        \
    sqisigntriangle/src/quaternion/ref/generic/normeq.c                                           \
    sqisigntriangle/src/quaternion/ref/generic/qlapoti.c                                          \
    sqisigntriangle/src/quaternion/ref/generic/hnf/ibz_division.c                                 \
    sqisigntriangle/src/quaternion/ref/generic/hnf/hnf_internal.c                                 \
    sqisigntriangle/src/quaternion/ref/generic/hnf/hnf.c                                          \
    sqisigntriangle/src/quaternion/ref/generic/test/random_input_generation.c                     \
    sqisigntriangle/src/mp/ref/generic/mp.c sqisigntriangle/src/gf/ref/lvlx/fp.c                  \
    sqisigntriangle/src/gf/ref/lvlx/fp2.c sqisigntriangle/src/ec/ref/lvlx/ec.c                    \
    sqisigntriangle/src/ec/ref/lvlx/ec_jac.c sqisigntriangle/src/ec/ref/lvlx/xisog.c              \
    sqisigntriangle/src/ec/ref/lvlx/xeval.c sqisigntriangle/src/ec/ref/lvlx/isog_chains.c         \
    sqisigntriangle/src/ec/ref/lvlx/basis.c sqisigntriangle/src/ec/ref/lvlx/biextension.c         \
    sqisigntriangle/src/hd/ref/lvlx/hd.c sqisigntriangle/src/hd/ref/lvlx/theta_structure.c        \
    sqisigntriangle/src/hd/ref/lvlx/theta_isogenies.c                                             \
    sqisigntriangle/src/verification/ref/lvlx/common.c                                            \
    sqisigntriangle/src/verification/ref/lvlx/encode_verification.c                               \
    sqisigntriangle/src/verification/ref/lvlx/verify.c                                            \
    sqisigntriangle/src/id2iso/ref/lvlx/id2iso.c sqisigntriangle/src/id2iso/ref/lvlx/dim2id2iso.c \
    sqisigntriangle/src/signature/ref/lvlx/encode_signature.c                                     \
    sqisigntriangle/src/signature/ref/lvlx/keygen.c sqisigntriangle/src/signature/ref/lvlx/sign.c \
    sqisigntriangle/src/nistapi/lvl2/api.c sqisigntriangle/src/gf/ref/lvl2/fp_p9309_64.c          \
    sqisigntriangle/src/precomp/ref/lvl2/ec_params.c                                              \
    sqisigntriangle/src/precomp/ref/lvl2/e0_basis.c                                               \
    sqisigntriangle/src/precomp/ref/lvl2/hd_splitting_transforms.c                                \
    sqisigntriangle/src/precomp/ref/lvl2/torsion_constants.c                                      \
    sqisigntriangle/src/precomp/ref/lvl2/quaternion_data.c                                        \
    sqisigntriangle/src/precomp/ref/lvl2/endomorphism_action.c
INC := -Isrc/SQIsignTriangle_lvl2/sqisigntriangle/include                                         \
    -Isrc/SQIsignTriangle_lvl2/sqisigntriangle/src/nistapi/lvl2                                   \
    -Isrc/SQIsignTriangle_lvl2/sqisigntriangle/src/common/generic/include                         \
    -Isrc/SQIsignTriangle_lvl2/sqisigntriangle/src/common/generic                                 \
    -Isrc/SQIsignTriangle_lvl2/sqisigntriangle/src/quaternion/ref/generic/include                 \
    -Isrc/SQIsignTriangle_lvl2/sqisigntriangle/src/quaternion/ref/generic/internal_quaternion_headers\
    -Isrc/SQIsignTriangle_lvl2/sqisigntriangle/src/quaternion/ref/generic                         \
    -Isrc/SQIsignTriangle_lvl2/sqisigntriangle/src/mp/ref/generic/include                         \
    -Isrc/SQIsignTriangle_lvl2/sqisigntriangle/src/gf/ref/include                                 \
    -Isrc/SQIsignTriangle_lvl2/sqisigntriangle/src/precomp/ref/lvl2/include                       \
    -Isrc/SQIsignTriangle_lvl2/sqisigntriangle/src/ec/ref/include                                 \
    -Isrc/SQIsignTriangle_lvl2/sqisigntriangle/src/hd/ref/include                                 \
    -Isrc/SQIsignTriangle_lvl2/sqisigntriangle/src/hd/ref/lvlx                                    \
    -Isrc/SQIsignTriangle_lvl2/sqisigntriangle/src/id2iso/ref/include                             \
    -Isrc/SQIsignTriangle_lvl2/sqisigntriangle/src/verification/ref/include                       \
    -Isrc/SQIsignTriangle_lvl2/sqisigntriangle/src/signature/ref/include
DEFS := -DSQISIGN_BUILD_TYPE_REF -DSQISIGN_GF_IMPL_REF -DENABLE_SIGN -DTARGET_AMD64 -DRADIX_64 -DHAVE_UINT128 -DTARGET_OS_UNIX -DSQISIGN_TEST_REPS=10 -DSQISIGN_VARIANT=lvl2
INSTANCE_CFLAGS := -funroll-loops
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-27,SQIsignTriangle_lvl2,sig,160,sig-27-sqisigntriangle-lvl2,ngcc_ref_sig_27_SQIsignTriangle_lvl2))

# SQIsignTriangle / SQIsignTriangle_lvl5 (256-bit)
ALGORITHM := SQIsignTriangle
SRCDIR := Implementations/Reference_Implementation/SQIsignTriangle_lvl5
SRCS := SIG_AlgorithmInstance.c drng.c auxfunc.c sqisigntriangle/src/sqisign.c                    \
    sqisigntriangle/src/common/generic/fips202.c sqisigntriangle/src/common/generic/mem.c         \
    sqisigntriangle/src/common/generic/tools.c                                                    \
    sqisigntriangle/src/quaternion/ref/generic/intbig.c                                           \
    sqisigntriangle/src/quaternion/ref/generic/algebra.c                                          \
    sqisigntriangle/src/quaternion/ref/generic/ideal.c                                            \
    sqisigntriangle/src/quaternion/ref/generic/dim4.c                                             \
    sqisigntriangle/src/quaternion/ref/generic/dim2.c                                             \
    sqisigntriangle/src/quaternion/ref/generic/integers.c                                         \
    sqisigntriangle/src/quaternion/ref/generic/lattice.c                                          \
    sqisigntriangle/src/quaternion/ref/generic/lat_ball.c                                         \
    sqisigntriangle/src/quaternion/ref/generic/finit.c                                            \
    sqisigntriangle/src/quaternion/ref/generic/printer.c                                          \
    sqisigntriangle/src/quaternion/ref/generic/lll/l2.c                                           \
    sqisigntriangle/src/quaternion/ref/generic/lll/lll_verification.c                             \
    sqisigntriangle/src/quaternion/ref/generic/lll/lll_applications.c                             \
    sqisigntriangle/src/quaternion/ref/generic/rationals.c                                        \
    sqisigntriangle/src/quaternion/ref/generic/normeq.c                                           \
    sqisigntriangle/src/quaternion/ref/generic/qlapoti.c                                          \
    sqisigntriangle/src/quaternion/ref/generic/hnf/ibz_division.c                                 \
    sqisigntriangle/src/quaternion/ref/generic/hnf/hnf_internal.c                                 \
    sqisigntriangle/src/quaternion/ref/generic/hnf/hnf.c                                          \
    sqisigntriangle/src/quaternion/ref/generic/test/random_input_generation.c                     \
    sqisigntriangle/src/mp/ref/generic/mp.c sqisigntriangle/src/gf/ref/lvlx/fp.c                  \
    sqisigntriangle/src/gf/ref/lvlx/fp2.c sqisigntriangle/src/ec/ref/lvlx/ec.c                    \
    sqisigntriangle/src/ec/ref/lvlx/ec_jac.c sqisigntriangle/src/ec/ref/lvlx/xisog.c              \
    sqisigntriangle/src/ec/ref/lvlx/xeval.c sqisigntriangle/src/ec/ref/lvlx/isog_chains.c         \
    sqisigntriangle/src/ec/ref/lvlx/basis.c sqisigntriangle/src/ec/ref/lvlx/biextension.c         \
    sqisigntriangle/src/hd/ref/lvlx/hd.c sqisigntriangle/src/hd/ref/lvlx/theta_structure.c        \
    sqisigntriangle/src/hd/ref/lvlx/theta_isogenies.c                                             \
    sqisigntriangle/src/verification/ref/lvlx/common.c                                            \
    sqisigntriangle/src/verification/ref/lvlx/encode_verification.c                               \
    sqisigntriangle/src/verification/ref/lvlx/verify.c                                            \
    sqisigntriangle/src/id2iso/ref/lvlx/id2iso.c sqisigntriangle/src/id2iso/ref/lvlx/dim2id2iso.c \
    sqisigntriangle/src/signature/ref/lvlx/encode_signature.c                                     \
    sqisigntriangle/src/signature/ref/lvlx/keygen.c sqisigntriangle/src/signature/ref/lvlx/sign.c \
    sqisigntriangle/src/nistapi/lvl5/api.c sqisigntriangle/src/gf/ref/lvl5/fp_p27500_64.c         \
    sqisigntriangle/src/precomp/ref/lvl5/ec_params.c                                              \
    sqisigntriangle/src/precomp/ref/lvl5/e0_basis.c                                               \
    sqisigntriangle/src/precomp/ref/lvl5/hd_splitting_transforms.c                                \
    sqisigntriangle/src/precomp/ref/lvl5/torsion_constants.c                                      \
    sqisigntriangle/src/precomp/ref/lvl5/quaternion_data.c                                        \
    sqisigntriangle/src/precomp/ref/lvl5/endomorphism_action.c
INC := -Isrc/SQIsignTriangle_lvl5/sqisigntriangle/include                                         \
    -Isrc/SQIsignTriangle_lvl5/sqisigntriangle/src/nistapi/lvl5                                   \
    -Isrc/SQIsignTriangle_lvl5/sqisigntriangle/src/common/generic/include                         \
    -Isrc/SQIsignTriangle_lvl5/sqisigntriangle/src/common/generic                                 \
    -Isrc/SQIsignTriangle_lvl5/sqisigntriangle/src/quaternion/ref/generic/include                 \
    -Isrc/SQIsignTriangle_lvl5/sqisigntriangle/src/quaternion/ref/generic/internal_quaternion_headers\
    -Isrc/SQIsignTriangle_lvl5/sqisigntriangle/src/quaternion/ref/generic                         \
    -Isrc/SQIsignTriangle_lvl5/sqisigntriangle/src/mp/ref/generic/include                         \
    -Isrc/SQIsignTriangle_lvl5/sqisigntriangle/src/gf/ref/include                                 \
    -Isrc/SQIsignTriangle_lvl5/sqisigntriangle/src/precomp/ref/lvl5/include                       \
    -Isrc/SQIsignTriangle_lvl5/sqisigntriangle/src/ec/ref/include                                 \
    -Isrc/SQIsignTriangle_lvl5/sqisigntriangle/src/hd/ref/include                                 \
    -Isrc/SQIsignTriangle_lvl5/sqisigntriangle/src/hd/ref/lvlx                                    \
    -Isrc/SQIsignTriangle_lvl5/sqisigntriangle/src/id2iso/ref/include                             \
    -Isrc/SQIsignTriangle_lvl5/sqisigntriangle/src/verification/ref/include                       \
    -Isrc/SQIsignTriangle_lvl5/sqisigntriangle/src/signature/ref/include
DEFS := -DSQISIGN_BUILD_TYPE_REF -DSQISIGN_GF_IMPL_REF -DENABLE_SIGN -DTARGET_AMD64 -DRADIX_64 -DHAVE_UINT128 -DTARGET_OS_UNIX -DSQISIGN_TEST_REPS=10 -DSQISIGN_VARIANT=lvl5
INSTANCE_CFLAGS := -funroll-loops
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-27,SQIsignTriangle_lvl5,sig,256,sig-27-sqisigntriangle-lvl5,ngcc_ref_sig_27_SQIsignTriangle_lvl5))

