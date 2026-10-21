# Diagnostic targets only. These instances are never linked into ngcc.so.

ifeq ($(CANDIDATE),kem-04/bag_piglet_128)
# BAG-Piglet / bag_piglet_128 (128-bit)
ALGORITHM := BAG-Piglet
SRCDIR := Implementations/Reference_Implementation/bag_piglet128
SRCS := src/common/aes256ctr.c src/common/augabidulin.c src/common/ccakem.c src/common/ccapke.c   \
    src/common/gabidulin.c src/common/parsing.c src/common/q_polynomial.c src/common/tools.c      \
    src/ffi/ffi_elt.c src/ffi/ffi_field.c src/ffi/ffi_vec.c src/rbc/rbc_elt.c src/rbc/rbc_poly.c  \
    src/rbc/rbc_qre.c src/rbc/rbc_vec.c src/rbc/rbc_vspace.c src/scheme/bag_piglet.c              \
    src/kat/KEM_AlgorithmInstance.c lib/api_pkc/auxfunc.c lib/api_pkc/drng.c                      \
    lib/randombytes/randombytes.c
INC := -Isrc/bag_piglet_128/src/common -Isrc/bag_piglet_128/src/scheme                            \
    -Isrc/bag_piglet_128/src/ffi -Isrc/bag_piglet_128/src/rbc -Isrc/bag_piglet_128/src/kat        \
    -Isrc/bag_piglet_128/lib/api_pkc -Isrc/bag_piglet_128/lib/randombytes
DEFS := -DBAG_PIGLET_KAT=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"src/kat/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-04,bag_piglet_128,kem,128,kem-04-bag-piglet-128,ngcc_ref_kem_04_bag_piglet_128))

endif

ifeq ($(CANDIDATE),kem-04/bag_piglet_256)
# BAG-Piglet / bag_piglet_256 (256-bit)
ALGORITHM := BAG-Piglet
SRCDIR := Implementations/Reference_Implementation/bag_piglet256
SRCS := src/common/aes256ctr.c src/common/augabidulin.c src/common/ccakem.c src/common/ccapke.c   \
    src/common/gabidulin.c src/common/parsing.c src/common/q_polynomial.c src/common/tools.c      \
    src/ffi/ffi_elt.c src/ffi/ffi_field.c src/ffi/ffi_vec.c src/rbc/rbc_elt.c src/rbc/rbc_poly.c  \
    src/rbc/rbc_qre.c src/rbc/rbc_vec.c src/rbc/rbc_vspace.c src/scheme/bag_piglet.c              \
    src/kat/KEM_AlgorithmInstance.c lib/api_pkc/auxfunc.c lib/api_pkc/drng.c                      \
    lib/randombytes/randombytes.c
INC := -Isrc/bag_piglet_256/src/common -Isrc/bag_piglet_256/src/scheme                            \
    -Isrc/bag_piglet_256/src/ffi -Isrc/bag_piglet_256/src/rbc -Isrc/bag_piglet_256/src/kat        \
    -Isrc/bag_piglet_256/lib/api_pkc -Isrc/bag_piglet_256/lib/randombytes
DEFS := -DBAG_PIGLET_KAT=1
INSTANCE_CFLAGS := 
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"src/kat/KEM_AlgorithmInstance.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-04,bag_piglet_256,kem,256,kem-04-bag-piglet-256,ngcc_ref_kem_04_bag_piglet_256))

endif

ifeq ($(CANDIDATE),kem-06/BRA-128)
# BRA / BRA-128 (128-bit)
ALGORITHM := BRA
SRCDIR := Implementations/Reference_Implementation/BRA-128
SRCS := src/augmented_gabidulin.c src/bra.c src/kem.c src/parsing.c src/qpoly.c                   \
    lib/random_source/random_source.c lib/XKCP/KeccakHash.c lib/XKCP/KeccakHashtimes4.c           \
    lib/XKCP/KeccakSponge.c lib/XKCP/KeccakSpongetimes4.c lib/XKCP/SimpleFIPS202.c                \
    lib/XKCP/opt64/KeccakP-1600-opt64.c lib/XKCP/opt64/KeccakP-1600-times4-on1.c drng.c auxfunc.c \
    KEM_BRA-128.c src/rbc-67/rbc_elt.c src/rbc-67/rbc_vec.c src/rbc-67/rbc_poly.c                 \
    src/rbc-67/rbc_qre.c src/rbc-67/rbc_vspace.c
INC := -Isrc/BRA-128/src -Isrc/BRA-128/src/rbc-67 -Isrc/BRA-128/lib/random_source                 \
    -Isrc/BRA-128/lib/XKCP -Isrc/BRA-128/lib/XKCP/opt64 -Isrc/BRA-128/lib/wrapper
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_BRA-128.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-06,BRA-128,kem,128,kem-06-bra-128,ngcc_ref_kem_06_BRA_128))

endif

ifeq ($(CANDIDATE),kem-06/BRA-256)
# BRA / BRA-256 (256-bit)
ALGORITHM := BRA
SRCDIR := Implementations/Reference_Implementation/BRA-256
SRCS := src/augmented_gabidulin.c src/bra.c src/kem.c src/parsing.c src/qpoly.c                   \
    lib/random_source/random_source.c lib/XKCP/KeccakHash.c lib/XKCP/KeccakHashtimes4.c           \
    lib/XKCP/KeccakSponge.c lib/XKCP/KeccakSpongetimes4.c lib/XKCP/SimpleFIPS202.c                \
    lib/XKCP/opt64/KeccakP-1600-opt64.c lib/XKCP/opt64/KeccakP-1600-times4-on1.c drng.c auxfunc.c \
    KEM_BRA-256.c src/rbc-83/rbc_elt.c src/rbc-83/rbc_vec.c src/rbc-83/rbc_poly.c                 \
    src/rbc-83/rbc_qre.c src/rbc-83/rbc_vspace.c
INC := -Isrc/BRA-256/src -Isrc/BRA-256/src/rbc-83 -Isrc/BRA-256/lib/random_source                 \
    -Isrc/BRA-256/lib/XKCP -Isrc/BRA-256/lib/XKCP/opt64 -Isrc/BRA-256/lib/wrapper
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_BRA-256.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-06,BRA-256,kem,256,kem-06-bra-256,ngcc_ref_kem_06_BRA_256))

endif

ifeq ($(CANDIDATE),kem-07/BRQC-128)
# BRQC / BRQC-128 (128-bit)
ALGORITHM := BRQC
SRCDIR := Implementations/Reference_Implementation/BRQC-128
SRCS := src/brqc.c src/gabidulin.c src/kem.c src/parsing.c src/qpoly.c                            \
    lib/random_source/random_source.c lib/XKCP/KeccakHash.c lib/XKCP/KeccakHashtimes4.c           \
    lib/XKCP/KeccakSponge.c lib/XKCP/KeccakSpongetimes4.c lib/XKCP/SimpleFIPS202.c                \
    lib/XKCP/opt64/KeccakP-1600-opt64.c lib/XKCP/opt64/KeccakP-1600-times4-on1.c drng.c auxfunc.c \
    KEM_BRQC-128.c src/rbc-127/rbc_elt.c src/rbc-127/rbc_vec.c src/rbc-127/rbc_poly.c             \
    src/rbc-127/rbc_qre.c src/rbc-127/rbc_vspace.c
INC := -Isrc/BRQC-128/src -Isrc/BRQC-128/src/rbc-127 -Isrc/BRQC-128/lib/random_source             \
    -Isrc/BRQC-128/lib/XKCP -Isrc/BRQC-128/lib/XKCP/opt64 -Isrc/BRQC-128/lib/wrapper
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_BRQC-128.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-07,BRQC-128,kem,128,kem-07-brqc-128,ngcc_ref_kem_07_BRQC_128))

endif

ifeq ($(CANDIDATE),kem-07/BRQC-256)
# BRQC / BRQC-256 (256-bit)
ALGORITHM := BRQC
SRCDIR := Implementations/Reference_Implementation/BRQC-256
SRCS := src/brqc.c src/gabidulin.c src/kem.c src/parsing.c src/qpoly.c                            \
    lib/random_source/random_source.c lib/XKCP/KeccakHash.c lib/XKCP/KeccakHashtimes4.c           \
    lib/XKCP/KeccakSponge.c lib/XKCP/KeccakSpongetimes4.c lib/XKCP/SimpleFIPS202.c                \
    lib/XKCP/opt64/KeccakP-1600-opt64.c lib/XKCP/opt64/KeccakP-1600-times4-on1.c drng.c auxfunc.c \
    KEM_BRQC-256.c src/rbc-163/rbc_elt.c src/rbc-163/rbc_vec.c src/rbc-163/rbc_poly.c             \
    src/rbc-163/rbc_qre.c src/rbc-163/rbc_vspace.c
INC := -Isrc/BRQC-256/src -Isrc/BRQC-256/src/rbc-163 -Isrc/BRQC-256/lib/random_source             \
    -Isrc/BRQC-256/lib/XKCP -Isrc/BRQC-256/lib/XKCP/opt64 -Isrc/BRQC-256/lib/wrapper
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_BRQC-256.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-07,BRQC-256,kem,256,kem-07-brqc-256,ngcc_ref_kem_07_BRQC_256))

endif

ifeq ($(CANDIDATE),kem-10/CMultiURAG-128)
# C-Multi-UR-AG / CMultiURAG-128 (128-bit)
ALGORITHM := C-Multi-UR-AG
SRCDIR := Implementations/Reference_Implementation/CMultiURAG-128
SRCS := KEM_CMultiURAG-128.c drng.c auxfunc.c lib/random_source/random_source.c                   \
    lib/XKCP/KeccakHash.c lib/XKCP/KeccakHashtimes4.c lib/XKCP/KeccakSponge.c                     \
    lib/XKCP/KeccakSpongetimes4.c lib/XKCP/SimpleFIPS202.c lib/XKCP/opt64/KeccakP-1600-opt64.c    \
    lib/XKCP/opt64/KeccakP-1600-times4-on1.c src/rbc-79/rbc_elt.c src/rbc-79/rbc_vec.c            \
    src/rbc-79/rbc_poly.c src/rbc-79/rbc_qre.c src/rbc-79/rbc_vspace.c src/rbc-79/rbc_mat.c       \
    src/parsing.c src/qpoly.c src/augmented_gabidulin.c src/cmultiurag.c src/kem.c
INC := -Isrc/CMultiURAG-128/src -Isrc/CMultiURAG-128/src/rbc-79                                   \
    -Isrc/CMultiURAG-128/lib/random_source -Isrc/CMultiURAG-128/lib/XKCP                          \
    -Isrc/CMultiURAG-128/lib/XKCP/opt64 -Isrc/CMultiURAG-128/lib/wrapper
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_CMultiURAG-128.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-10,CMultiURAG-128,kem,128,kem-10-cmultiurag-128,ngcc_ref_kem_10_CMultiURAG_128))

endif

ifeq ($(CANDIDATE),kem-10/CMultiURAG-256)
# C-Multi-UR-AG / CMultiURAG-256 (256-bit)
ALGORITHM := C-Multi-UR-AG
SRCDIR := Implementations/Reference_Implementation/CMultiURAG-256
SRCS := KEM_CMultiURAG-256.c drng.c auxfunc.c lib/random_source/random_source.c                   \
    lib/XKCP/KeccakHash.c lib/XKCP/KeccakHashtimes4.c lib/XKCP/KeccakSponge.c                     \
    lib/XKCP/KeccakSpongetimes4.c lib/XKCP/SimpleFIPS202.c lib/XKCP/opt64/KeccakP-1600-opt64.c    \
    lib/XKCP/opt64/KeccakP-1600-times4-on1.c src/rbc-127/rbc_elt.c src/rbc-127/rbc_vec.c          \
    src/rbc-127/rbc_poly.c src/rbc-127/rbc_qre.c src/rbc-127/rbc_vspace.c src/rbc-127/rbc_mat.c   \
    src/parsing.c src/qpoly.c src/augmented_gabidulin.c src/cmultiurag.c src/kem.c
INC := -Isrc/CMultiURAG-256/src -Isrc/CMultiURAG-256/src/rbc-127                                  \
    -Isrc/CMultiURAG-256/lib/random_source -Isrc/CMultiURAG-256/lib/XKCP                          \
    -Isrc/CMultiURAG-256/lib/XKCP/opt64 -Isrc/CMultiURAG-256/lib/wrapper
DEFS := 
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"KEM_CMultiURAG-256.h"'
NOAUX := 
$(eval $(call ngcc_instance,kem-10,CMultiURAG-256,kem,256,kem-10-cmultiurag-256,ngcc_ref_kem_10_CMultiURAG_256))

endif

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

ifeq ($(CANDIDATE),sign-07/CS-128)
# CS / CS-128 (128-bit)
ALGORITHM := CS
SRCDIR := Implementations/Reference_Implementation/CS-128
SRCS := SIG_AlgorithmInstance.c auxfunc.c bits-hints.c conversion.c cs.c drng.c encodings.c ntt.c \
    sampling.c
INC := 
DEFS := 
INSTANCE_CFLAGS := -fcommon -w
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"API_PKC/Implementations/Reference_Implementation/AlgorithmInstance/SIG_AlgorithmInstance.h"' -DNGCC_INSTANCE='"CS_128"' -DNGCC_NO_DRNG
NOAUX := 
$(eval $(call ngcc_instance,sign-07,CS-128,sig,128,sig-07-cs-128,ngcc_ref_sig_07_CS_128))

endif

ifeq ($(CANDIDATE),sign-07/CS-256)
# CS / CS-256 (256-bit)
ALGORITHM := CS
SRCDIR := Implementations/Reference_Implementation/CS-256
SRCS := SIG_AlgorithmInstance.c auxfunc.c bits-hints.c conversion.c cs.c drng.c encodings.c ntt.c \
    sampling.c
INC := 
DEFS := 
INSTANCE_CFLAGS := -fcommon -w
SHIMDEFS := -DNGCC_INSTANCE_HEADER='"API_PKC/Implementations/Reference_Implementation/AlgorithmInstance/SIG_AlgorithmInstance.h"' -DNGCC_INSTANCE='"CS_256"' -DNGCC_NO_DRNG
NOAUX := 
$(eval $(call ngcc_instance,sign-07,CS-256,sig,256,sig-07-cs-256,ngcc_ref_sig_07_CS_256))

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

ifeq ($(CANDIDATE),sign-32/UVW-128)
# UVW signature / UVW-128 (128-bit)
ALGORITHM := UVW signature
SRCDIR := Implementations/Reference_Implementation/UVW-128
SRCS := SIG_AlgorithmInstance.c uvw.c gauss.c auxfunc.c drng.c fq_arithmetic/vf3.c                \
    fq_arithmetic/mf3.c fq_arithmetic/dp.c
INC := 
DEFS := -DUSE_API_PKC
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-32,UVW-128,sig,128,sig-32-uvw-128,ngcc_ref_sig_32_UVW_128))

endif

ifeq ($(CANDIDATE),sign-32/UVW-256)
# UVW signature / UVW-256 (256-bit)
ALGORITHM := UVW signature
SRCDIR := Implementations/Reference_Implementation/UVW-256
SRCS := SIG_AlgorithmInstance.c uvw.c gauss.c auxfunc.c drng.c fq_arithmetic/vf3.c                \
    fq_arithmetic/mf3.c fq_arithmetic/dp.c
INC := 
DEFS := -DUSE_API_PKC
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-32,UVW-256,sig,256,sig-32-uvw-256,ngcc_ref_sig_32_UVW_256))

endif

