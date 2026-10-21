# Diagnostic targets only. These instances are never linked into ngcc.so.

ifeq ($(CANDIDATE),kem-12/CTL-257-512)
# CTL / CTL-257-512 (128-bit)
ALGORITHM := CTL
SRCDIR := Implementations and Test_Vectors/KEM-CTL-x86-Reference_Implementation/crypto_kem/Implementations/CTL-257-512
SRCS := api_257_512.c codec.c fft.c fnr.c kem257.c kem769.c kem3329.c ntru_utils.c ng_fxp.c       \
    ng_mp31.c ng_ntru.c ng_poly.c ng_zint31.c sha3.c modqp.c prng.c auxfunc.c drng.c              \
    ntru_solver.c keygen.c KEM_AlgorithmInstance.c
INC :=
DEFS := -DNTRUGEN_PREFIX=ctlng
INSTANCE_CFLAGS :=
SHIMDEFS :=
NOAUX :=
$(eval $(call ngcc_instance,kem-12,CTL-257-512,kem,128,kem-12-ctl-257-512,ngcc_ref_kem_12_CTL_257_512))

endif

ifeq ($(CANDIDATE),kem-12/CTL-769-1024)
# CTL / CTL-769-1024 (256-bit)
ALGORITHM := CTL
SRCDIR := Implementations and Test_Vectors/KEM-CTL-x86-Reference_Implementation/crypto_kem/Implementations/CTL-769-1024
SRCS := api_769_1024.c codec.c fft.c fnr.c kem257.c kem769.c kem3329.c ntru_utils.c ng_fxp.c      \
    ng_mp31.c ng_ntru.c ng_poly.c ng_zint31.c sha3.c modqp.c prng.c auxfunc.c drng.c              \
    ntru_solver.c keygen.c KEM_AlgorithmInstance.c
INC :=
DEFS := -DNTRUGEN_PREFIX=ctlng
INSTANCE_CFLAGS :=
SHIMDEFS :=
NOAUX :=
$(eval $(call ngcc_instance,kem-12,CTL-769-1024,kem,256,kem-12-ctl-769-1024,ngcc_ref_kem_12_CTL_769_1024))

endif

ifeq ($(CANDIDATE),sign-15/lwrdsa128)
# MORNING-ATLAS / lwrdsa128 (128-bit)
ALGORITHM := MORNING-ATLAS
SRCDIR := Implementation/Reference_Implementation/lwrdsa128
SRCS := SIG_lwrdsa128.c auxfunc.c drng.c packing.c poly.c polyvec.c rounding.c
INC := 
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-15,lwrdsa128,sig,128,sig-15-lwrdsa128,ngcc_ref_sig_15_lwrdsa128))

endif

ifeq ($(CANDIDATE),sign-15/lwrdsa192)
# MORNING-ATLAS / lwrdsa192 (192-bit)
ALGORITHM := MORNING-ATLAS
SRCDIR := Implementation/Reference_Implementation/lwrdsa192
SRCS := SIG_lwrdsa192.c auxfunc.c drng.c packing.c poly.c polyvec.c rounding.c
INC := 
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-15,lwrdsa192,sig,192,sig-15-lwrdsa192,ngcc_ref_sig_15_lwrdsa192))

endif

ifeq ($(CANDIDATE),sign-15/lwrdsa256)
# MORNING-ATLAS / lwrdsa256 (256-bit)
ALGORITHM := MORNING-ATLAS
SRCDIR := Implementation/Reference_Implementation/lwrdsa256
SRCS := SIG_lwrdsa256.c auxfunc.c drng.c packing.c poly.c polyvec.c rounding.c
INC := 
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-15,lwrdsa256,sig,256,sig-15-lwrdsa256,ngcc_ref_sig_15_lwrdsa256))

endif

ifeq ($(CANDIDATE),sign-18/Origami-128)
# Origami / Origami-128 (128-bit)
ALGORITHM := Origami
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Origami-128
SRCS := SIG_AlgorithmInstance.c aes.c auxfunc.c drng.c origami_ref.c symmetric_iccs.c
INC := 
DEFS := -DORIGAMI_OPT=REF
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-18,Origami-128,sig,128,sig-18-origami-128,ngcc_ref_sig_18_Origami_128))

endif

ifeq ($(CANDIDATE),sign-18/Origami-256)
# Origami / Origami-256 (256-bit)
ALGORITHM := Origami
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Origami-256
SRCS := SIG_AlgorithmInstance.c aes.c auxfunc.c drng.c origami_ref.c symmetric_iccs.c
INC := 
DEFS := -DORIGAMI_OPT=REF
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-18,Origami-256,sig,256,sig-18-origami-256,ngcc_ref_sig_18_Origami_256))

endif

ifeq ($(CANDIDATE),sign-26/SQIsign2D-lvl2)
# SQIsign2D-push1/2 / SQIsign2D-lvl2 (192-bit)
ALGORITHM := SQIsign2D-push1/2
SRCDIR := Implementations/sqisign2d_lvl2
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
    src/sqisigndim2/ref/sqisigndim2x/doublepath.c src/gf/ref/lvl2/fp_191118.c                     \
    src/precomp/ref/lvl2/gf_constants.c src/precomp/ref/lvl2/torsion_constants.c                  \
    src/precomp/ref/lvl2/quaternion_data.c src/precomp/ref/lvl2/endomorphism_action.c             \
    src/precomp/ref/lvl2/klpt_constants.c
INC := -Isrc/SQIsign2D-lvl2/src/iccs -Isrc/SQIsign2D-lvl2/include                                 \
    -Isrc/SQIsign2D-lvl2/src/common/generic/include                                               \
    -Isrc/SQIsign2D-lvl2/src/intbig/ref/generic/include                                           \
    -Isrc/SQIsign2D-lvl2/src/quaternion/ref/generic/include                                       \
    -Isrc/SQIsign2D-lvl2/src/precomp/ref/lvl2/include -Isrc/SQIsign2D-lvl2/src/klpt/ref/include   \
    -Isrc/SQIsign2D-lvl2/src/gf/ref/include -Isrc/SQIsign2D-lvl2/src/ec/ref/include               \
    -Isrc/SQIsign2D-lvl2/src/hd/ref/include -Isrc/SQIsign2D-lvl2/src/id2iso/ref/include           \
    -Isrc/SQIsign2D-lvl2/src/dim2id2iso/ref/include                                               \
    -Isrc/SQIsign2D-lvl2/src/sqisigndim2/ref/include
DEFS := -DNDEBUG -DTARGET_AMD64 -DRADIX_64 -DTARGET_OS_UNIX -DSQISIGN_VARIANT=lvl2 -DOUTPUT_BLANK_TEST_VECTORS=0 -DALGORITHM_INSTANCE='"SQIsign2D-lvl2"'
INSTANCE_CFLAGS := -funroll-loops
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-26,SQIsign2D-lvl2,sig,192,sig-26-sqisign2d-lvl2,ngcc_ref_sig_26_SQIsign2D_lvl2))

endif
