# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# SQIsign2D2 / SQISign2Dsquare-Level1-eff_compressed (128-bit)
ALGORITHM := SQIsign2D2
SRCDIR := Implementations/Reference_Implementation/SQISign2Dsquare-Level1-eff
SRCS := algebra.c auxfunc.c basis.c biextension.c dim2.c dim2id2iso.c dim4.c drng.c ec.c finit.c  \
    fp.c fp2.c hd.c id2iso.c ideal.c intbig.c integers.c isog_chains.c klptx.c lattice.c lll.c    \
    matkermod.c mp.c pack.c printer.c theta_isogenies.c theta_structure.c tools.c xeval.c xisog.c \
    ec_arithmetic/endomorphism_action.c ec_arithmetic/gf_constants.c                              \
    ec_arithmetic/quaternion_data.c ec_arithmetic/torsion_constants.c sqisign/keygen.c            \
    sqisign/sign.c sqisign/SIG_AlgorithmInstance.c ec_arithmetic/fp_p255.c
INC := -Isrc/SQISign2Dsquare-Level1-eff_compressed/sqisign                                        \
    -Isrc/SQISign2Dsquare-Level1-eff_compressed/ec_arithmetic/include
DEFS := -DNDEBUG -DCOMPRESSED=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sqisign/SIG_AlgorithmInstance.h"' -DNGCC_INSTANCE='"SQISign2Dsquare-Level1-eff_compressed"'
NOAUX := 
$(eval $(call ngcc_instance,sign-25,SQISign2Dsquare-Level1-eff_compressed,sig,128,sig-25-sqisign2dsq-l1-eff-compressed,ngcc_ref_sig_25_SQISign2Dsquare_Level1_eff_compressed))

# SQIsign2D2 / SQISign2Dsquare-Level1-eff_uncompressed (128-bit)
ALGORITHM := SQIsign2D2
SRCDIR := Implementations/Reference_Implementation/SQISign2Dsquare-Level1-eff
SRCS := algebra.c auxfunc.c basis.c biextension.c dim2.c dim2id2iso.c dim4.c drng.c ec.c finit.c  \
    fp.c fp2.c hd.c id2iso.c ideal.c intbig.c integers.c isog_chains.c klptx.c lattice.c lll.c    \
    matkermod.c mp.c pack.c printer.c theta_isogenies.c theta_structure.c tools.c xeval.c xisog.c \
    ec_arithmetic/endomorphism_action.c ec_arithmetic/gf_constants.c                              \
    ec_arithmetic/quaternion_data.c ec_arithmetic/torsion_constants.c sqisign/keygen.c            \
    sqisign/sign.c sqisign/SIG_AlgorithmInstance.c ec_arithmetic/fp_p255.c
INC := -Isrc/SQISign2Dsquare-Level1-eff_uncompressed/sqisign                                      \
    -Isrc/SQISign2Dsquare-Level1-eff_uncompressed/ec_arithmetic/include
DEFS := -DNDEBUG -DCOMPRESSED=0
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sqisign/SIG_AlgorithmInstance.h"' -DNGCC_INSTANCE='"SQISign2Dsquare-Level1-eff_uncompressed"'
NOAUX := 
$(eval $(call ngcc_instance,sign-25,SQISign2Dsquare-Level1-eff_uncompressed,sig,128,sig-25-sqisign2dsq-l1-eff-uncompressed,ngcc_ref_sig_25_SQISign2Dsquare_Level1_eff_uncompressed))

# SQIsign2D2 / SQISign2Dsquare-Level1-sec_compressed (128-bit)
ALGORITHM := SQIsign2D2
SRCDIR := Implementations/Reference_Implementation/SQISign2Dsquare-Level1-sec
SRCS := algebra.c auxfunc.c basis.c biextension.c dim2.c dim2id2iso.c dim4.c drng.c ec.c finit.c  \
    fp.c fp2.c hd.c id2iso.c ideal.c intbig.c integers.c isog_chains.c klptx.c lattice.c lll.c    \
    matkermod.c mp.c pack.c printer.c theta_isogenies.c theta_structure.c tools.c xeval.c xisog.c \
    ec_arithmetic/endomorphism_action.c ec_arithmetic/gf_constants.c                              \
    ec_arithmetic/quaternion_data.c ec_arithmetic/torsion_constants.c sqisign/keygen.c            \
    sqisign/sign.c sqisign/SIG_AlgorithmInstance.c ec_arithmetic/fp_p271.c
INC := -Isrc/SQISign2Dsquare-Level1-sec_compressed/sqisign                                        \
    -Isrc/SQISign2Dsquare-Level1-sec_compressed/ec_arithmetic/include
DEFS := -DNDEBUG -DCOMPRESSED=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sqisign/SIG_AlgorithmInstance.h"' -DNGCC_INSTANCE='"SQISign2Dsquare-Level1-sec_compressed"'
NOAUX := 
$(eval $(call ngcc_instance,sign-25,SQISign2Dsquare-Level1-sec_compressed,sig,128,sig-25-sqisign2dsq-l1-sec-compressed,ngcc_ref_sig_25_SQISign2Dsquare_Level1_sec_compressed))

# SQIsign2D2 / SQISign2Dsquare-Level1-sec_uncompressed (128-bit)
ALGORITHM := SQIsign2D2
SRCDIR := Implementations/Reference_Implementation/SQISign2Dsquare-Level1-sec
SRCS := algebra.c auxfunc.c basis.c biextension.c dim2.c dim2id2iso.c dim4.c drng.c ec.c finit.c  \
    fp.c fp2.c hd.c id2iso.c ideal.c intbig.c integers.c isog_chains.c klptx.c lattice.c lll.c    \
    matkermod.c mp.c pack.c printer.c theta_isogenies.c theta_structure.c tools.c xeval.c xisog.c \
    ec_arithmetic/endomorphism_action.c ec_arithmetic/gf_constants.c                              \
    ec_arithmetic/quaternion_data.c ec_arithmetic/torsion_constants.c sqisign/keygen.c            \
    sqisign/sign.c sqisign/SIG_AlgorithmInstance.c ec_arithmetic/fp_p271.c
INC := -Isrc/SQISign2Dsquare-Level1-sec_uncompressed/sqisign                                      \
    -Isrc/SQISign2Dsquare-Level1-sec_uncompressed/ec_arithmetic/include
DEFS := -DNDEBUG -DCOMPRESSED=0
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sqisign/SIG_AlgorithmInstance.h"' -DNGCC_INSTANCE='"SQISign2Dsquare-Level1-sec_uncompressed"'
NOAUX := 
$(eval $(call ngcc_instance,sign-25,SQISign2Dsquare-Level1-sec_uncompressed,sig,128,sig-25-sqisign2dsq-l1-sec-uncompressed,ngcc_ref_sig_25_SQISign2Dsquare_Level1_sec_uncompressed))

# SQIsign2D2 / SQISign2Dsquare-Level2-eff_compressed (160-bit)
ALGORITHM := SQIsign2D2
SRCDIR := Implementations/Reference_Implementation/SQISign2Dsquare-Level2-eff
SRCS := algebra.c auxfunc.c basis.c biextension.c dim2.c dim2id2iso.c dim4.c drng.c ec.c finit.c  \
    fp.c fp2.c hd.c id2iso.c ideal.c intbig.c integers.c isog_chains.c klptx.c lattice.c lll.c    \
    matkermod.c mp.c pack.c printer.c theta_isogenies.c theta_structure.c tools.c xeval.c xisog.c \
    ec_arithmetic/endomorphism_action.c ec_arithmetic/gf_constants.c                              \
    ec_arithmetic/quaternion_data.c ec_arithmetic/torsion_constants.c sqisign/keygen.c            \
    sqisign/sign.c sqisign/SIG_AlgorithmInstance.c ec_arithmetic/fp_p320.c
INC := -Isrc/SQISign2Dsquare-Level2-eff_compressed/sqisign                                        \
    -Isrc/SQISign2Dsquare-Level2-eff_compressed/ec_arithmetic/include
DEFS := -DNDEBUG -DCOMPRESSED=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sqisign/SIG_AlgorithmInstance.h"' -DNGCC_INSTANCE='"SQISign2Dsquare-Level2-eff_compressed"'
NOAUX := 
$(eval $(call ngcc_instance,sign-25,SQISign2Dsquare-Level2-eff_compressed,sig,160,sig-25-sqisign2dsq-l2-eff-compressed,ngcc_ref_sig_25_SQISign2Dsquare_Level2_eff_compressed))

# SQIsign2D2 / SQISign2Dsquare-Level2-eff_uncompressed (160-bit)
ALGORITHM := SQIsign2D2
SRCDIR := Implementations/Reference_Implementation/SQISign2Dsquare-Level2-eff
SRCS := algebra.c auxfunc.c basis.c biextension.c dim2.c dim2id2iso.c dim4.c drng.c ec.c finit.c  \
    fp.c fp2.c hd.c id2iso.c ideal.c intbig.c integers.c isog_chains.c klptx.c lattice.c lll.c    \
    matkermod.c mp.c pack.c printer.c theta_isogenies.c theta_structure.c tools.c xeval.c xisog.c \
    ec_arithmetic/endomorphism_action.c ec_arithmetic/gf_constants.c                              \
    ec_arithmetic/quaternion_data.c ec_arithmetic/torsion_constants.c sqisign/keygen.c            \
    sqisign/sign.c sqisign/SIG_AlgorithmInstance.c ec_arithmetic/fp_p320.c
INC := -Isrc/SQISign2Dsquare-Level2-eff_uncompressed/sqisign                                      \
    -Isrc/SQISign2Dsquare-Level2-eff_uncompressed/ec_arithmetic/include
DEFS := -DNDEBUG -DCOMPRESSED=0
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sqisign/SIG_AlgorithmInstance.h"' -DNGCC_INSTANCE='"SQISign2Dsquare-Level2-eff_uncompressed"'
NOAUX := 
$(eval $(call ngcc_instance,sign-25,SQISign2Dsquare-Level2-eff_uncompressed,sig,160,sig-25-sqisign2dsq-l2-eff-uncompressed,ngcc_ref_sig_25_SQISign2Dsquare_Level2_eff_uncompressed))

# SQIsign2D2 / SQISign2Dsquare-Level2-sec_compressed (160-bit)
ALGORITHM := SQIsign2D2
SRCDIR := Implementations/Reference_Implementation/SQISign2Dsquare-Level2-sec
SRCS := algebra.c auxfunc.c basis.c biextension.c dim2.c dim2id2iso.c dim4.c drng.c ec.c finit.c  \
    fp.c fp2.c hd.c id2iso.c ideal.c intbig.c integers.c isog_chains.c klptx.c lattice.c lll.c    \
    matkermod.c mp.c pack.c printer.c theta_isogenies.c theta_structure.c tools.c xeval.c xisog.c \
    ec_arithmetic/endomorphism_action.c ec_arithmetic/gf_constants.c                              \
    ec_arithmetic/quaternion_data.c ec_arithmetic/torsion_constants.c sqisign/keygen.c            \
    sqisign/sign.c sqisign/SIG_AlgorithmInstance.c ec_arithmetic/fp_p332.c
INC := -Isrc/SQISign2Dsquare-Level2-sec_compressed/sqisign                                        \
    -Isrc/SQISign2Dsquare-Level2-sec_compressed/ec_arithmetic/include
DEFS := -DNDEBUG -DCOMPRESSED=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sqisign/SIG_AlgorithmInstance.h"' -DNGCC_INSTANCE='"SQISign2Dsquare-Level2-sec_compressed"'
NOAUX := 
$(eval $(call ngcc_instance,sign-25,SQISign2Dsquare-Level2-sec_compressed,sig,160,sig-25-sqisign2dsq-l2-sec-compressed,ngcc_ref_sig_25_SQISign2Dsquare_Level2_sec_compressed))

# SQIsign2D2 / SQISign2Dsquare-Level2-sec_uncompressed (160-bit)
ALGORITHM := SQIsign2D2
SRCDIR := Implementations/Reference_Implementation/SQISign2Dsquare-Level2-sec
SRCS := algebra.c auxfunc.c basis.c biextension.c dim2.c dim2id2iso.c dim4.c drng.c ec.c finit.c  \
    fp.c fp2.c hd.c id2iso.c ideal.c intbig.c integers.c isog_chains.c klptx.c lattice.c lll.c    \
    matkermod.c mp.c pack.c printer.c theta_isogenies.c theta_structure.c tools.c xeval.c xisog.c \
    ec_arithmetic/endomorphism_action.c ec_arithmetic/gf_constants.c                              \
    ec_arithmetic/quaternion_data.c ec_arithmetic/torsion_constants.c sqisign/keygen.c            \
    sqisign/sign.c sqisign/SIG_AlgorithmInstance.c ec_arithmetic/fp_p332.c
INC := -Isrc/SQISign2Dsquare-Level2-sec_uncompressed/sqisign                                      \
    -Isrc/SQISign2Dsquare-Level2-sec_uncompressed/ec_arithmetic/include
DEFS := -DNDEBUG -DCOMPRESSED=0
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sqisign/SIG_AlgorithmInstance.h"' -DNGCC_INSTANCE='"SQISign2Dsquare-Level2-sec_uncompressed"'
NOAUX := 
$(eval $(call ngcc_instance,sign-25,SQISign2Dsquare-Level2-sec_uncompressed,sig,160,sig-25-sqisign2dsq-l2-sec-uncompressed,ngcc_ref_sig_25_SQISign2Dsquare_Level2_sec_uncompressed))

# SQIsign2D2 / SQISign2Dsquare-Level3-eff_compressed (256-bit)
ALGORITHM := SQIsign2D2
SRCDIR := Implementations/Reference_Implementation/SQISign2Dsquare-Level3-eff
SRCS := algebra.c auxfunc.c basis.c biextension.c dim2.c dim2id2iso.c dim4.c drng.c ec.c finit.c  \
    fp.c fp2.c hd.c id2iso.c ideal.c intbig.c integers.c isog_chains.c klptx.c lattice.c lll.c    \
    matkermod.c mp.c pack.c printer.c theta_isogenies.c theta_structure.c tools.c xeval.c xisog.c \
    ec_arithmetic/endomorphism_action.c ec_arithmetic/gf_constants.c                              \
    ec_arithmetic/quaternion_data.c ec_arithmetic/torsion_constants.c sqisign/keygen.c            \
    sqisign/sign.c sqisign/SIG_AlgorithmInstance.c ec_arithmetic/fp_p511.c
INC := -Isrc/SQISign2Dsquare-Level3-eff_compressed/sqisign                                        \
    -Isrc/SQISign2Dsquare-Level3-eff_compressed/ec_arithmetic/include
DEFS := -DNDEBUG -DCOMPRESSED=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sqisign/SIG_AlgorithmInstance.h"' -DNGCC_INSTANCE='"SQISign2Dsquare-Level3-eff_compressed"'
NOAUX := 
$(eval $(call ngcc_instance,sign-25,SQISign2Dsquare-Level3-eff_compressed,sig,256,sig-25-sqisign2dsq-l3-eff-compressed,ngcc_ref_sig_25_SQISign2Dsquare_Level3_eff_compressed))

# SQIsign2D2 / SQISign2Dsquare-Level3-eff_uncompressed (256-bit)
ALGORITHM := SQIsign2D2
SRCDIR := Implementations/Reference_Implementation/SQISign2Dsquare-Level3-eff
SRCS := algebra.c auxfunc.c basis.c biextension.c dim2.c dim2id2iso.c dim4.c drng.c ec.c finit.c  \
    fp.c fp2.c hd.c id2iso.c ideal.c intbig.c integers.c isog_chains.c klptx.c lattice.c lll.c    \
    matkermod.c mp.c pack.c printer.c theta_isogenies.c theta_structure.c tools.c xeval.c xisog.c \
    ec_arithmetic/endomorphism_action.c ec_arithmetic/gf_constants.c                              \
    ec_arithmetic/quaternion_data.c ec_arithmetic/torsion_constants.c sqisign/keygen.c            \
    sqisign/sign.c sqisign/SIG_AlgorithmInstance.c ec_arithmetic/fp_p511.c
INC := -Isrc/SQISign2Dsquare-Level3-eff_uncompressed/sqisign                                      \
    -Isrc/SQISign2Dsquare-Level3-eff_uncompressed/ec_arithmetic/include
DEFS := -DNDEBUG -DCOMPRESSED=0
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sqisign/SIG_AlgorithmInstance.h"' -DNGCC_INSTANCE='"SQISign2Dsquare-Level3-eff_uncompressed"'
NOAUX := 
$(eval $(call ngcc_instance,sign-25,SQISign2Dsquare-Level3-eff_uncompressed,sig,256,sig-25-sqisign2dsq-l3-eff-uncompressed,ngcc_ref_sig_25_SQISign2Dsquare_Level3_eff_uncompressed))

# SQIsign2D2 / SQISign2Dsquare-Level3-sec_compressed (256-bit)
ALGORITHM := SQIsign2D2
SRCDIR := Implementations/Reference_Implementation/SQISign2Dsquare-Level3-sec
SRCS := algebra.c auxfunc.c basis.c biextension.c dim2.c dim2id2iso.c dim4.c drng.c ec.c finit.c  \
    fp.c fp2.c hd.c id2iso.c ideal.c intbig.c integers.c isog_chains.c klptx.c lattice.c lll.c    \
    matkermod.c mp.c pack.c printer.c theta_isogenies.c theta_structure.c tools.c xeval.c xisog.c \
    ec_arithmetic/endomorphism_action.c ec_arithmetic/gf_constants.c                              \
    ec_arithmetic/quaternion_data.c ec_arithmetic/torsion_constants.c sqisign/keygen.c            \
    sqisign/sign.c sqisign/SIG_AlgorithmInstance.c ec_arithmetic/fp_p523.c
INC := -Isrc/SQISign2Dsquare-Level3-sec_compressed/sqisign                                        \
    -Isrc/SQISign2Dsquare-Level3-sec_compressed/ec_arithmetic/include
DEFS := -DNDEBUG -DCOMPRESSED=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sqisign/SIG_AlgorithmInstance.h"' -DNGCC_INSTANCE='"SQISign2Dsquare-Level3-sec_compressed"'
NOAUX := 
$(eval $(call ngcc_instance,sign-25,SQISign2Dsquare-Level3-sec_compressed,sig,256,sig-25-sqisign2dsq-l3-sec-compressed,ngcc_ref_sig_25_SQISign2Dsquare_Level3_sec_compressed))

# SQIsign2D2 / SQISign2Dsquare-Level3-sec_uncompressed (256-bit)
ALGORITHM := SQIsign2D2
SRCDIR := Implementations/Reference_Implementation/SQISign2Dsquare-Level3-sec
SRCS := algebra.c auxfunc.c basis.c biextension.c dim2.c dim2id2iso.c dim4.c drng.c ec.c finit.c  \
    fp.c fp2.c hd.c id2iso.c ideal.c intbig.c integers.c isog_chains.c klptx.c lattice.c lll.c    \
    matkermod.c mp.c pack.c printer.c theta_isogenies.c theta_structure.c tools.c xeval.c xisog.c \
    ec_arithmetic/endomorphism_action.c ec_arithmetic/gf_constants.c                              \
    ec_arithmetic/quaternion_data.c ec_arithmetic/torsion_constants.c sqisign/keygen.c            \
    sqisign/sign.c sqisign/SIG_AlgorithmInstance.c ec_arithmetic/fp_p523.c
INC := -Isrc/SQISign2Dsquare-Level3-sec_uncompressed/sqisign                                      \
    -Isrc/SQISign2Dsquare-Level3-sec_uncompressed/ec_arithmetic/include
DEFS := -DNDEBUG -DCOMPRESSED=0
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"sqisign/SIG_AlgorithmInstance.h"' -DNGCC_INSTANCE='"SQISign2Dsquare-Level3-sec_uncompressed"'
NOAUX := 
$(eval $(call ngcc_instance,sign-25,SQISign2Dsquare-Level3-sec_uncompressed,sig,256,sig-25-sqisign2dsq-l3-sec-uncompressed,ngcc_ref_sig_25_SQISign2Dsquare_Level3_sec_uncompressed))

