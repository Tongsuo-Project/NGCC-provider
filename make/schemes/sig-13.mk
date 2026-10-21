# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# GreatWall / GreatWall128f (128-bit)
ALGORITHM := GreatWall
SRCDIR := Implementations/Reference_Implementation/GreatWall128f
SRCS := SIG_AlgorithmInstance.c drng.c api.c aes.c aes_impl.c block.c block_impl.c faest.c        \
    greatwall.c greatwall_impl.c hash.c KeccakHash.c KeccakP-1600-reference.c KeccakSponge.c      \
    owf_proof.c polynomials.c polynomials_impl.c prgs.c quicksilver.c small_vole.c transpose.c    \
    universal_hash.c util.c vector_com.c vole_check.c vole_commit.c randomness_kat_fixed.c        \
    auxfunc.c greatwall-128-matrix.c
INC := 
DEFS := -DGREATWALL_HASH_BACKEND_SM3
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-13,GreatWall128f,sig,128,sig-13-greatwall128f,ngcc_ref_sig_13_GreatWall128f))

# GreatWall / GreatWall128s (128-bit)
ALGORITHM := GreatWall
SRCDIR := Implementations/Reference_Implementation/GreatWall128s
SRCS := SIG_AlgorithmInstance.c drng.c api.c aes.c aes_impl.c block.c block_impl.c faest.c        \
    greatwall.c greatwall_impl.c hash.c KeccakHash.c KeccakP-1600-reference.c KeccakSponge.c      \
    owf_proof.c polynomials.c polynomials_impl.c prgs.c quicksilver.c small_vole.c transpose.c    \
    universal_hash.c util.c vector_com.c vole_check.c vole_commit.c randomness_kat_fixed.c        \
    auxfunc.c greatwall-128-matrix.c
INC := 
DEFS := -DGREATWALL_HASH_BACKEND_SM3
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-13,GreatWall128s,sig,128,sig-13-greatwall128s,ngcc_ref_sig_13_GreatWall128s))

# GreatWall / GreatWall160f (160-bit)
ALGORITHM := GreatWall
SRCDIR := Implementations/Reference_Implementation/GreatWall160f
SRCS := aes.c aes_impl.c api.c faest.c gf163_proof.c hash.c KeccakHash.c KeccakP-1600-reference.c \
    KeccakSponge.c owf_proof.c quicksilver.c small_vole.c transpose.c universal_hash.c util.c     \
    vector_com.c vole_check.c vole_commit.c drng.c SIG_AlgorithmInstance.c randomness_kat_fixed.c \
    auxfunc.c greatwall-160-matrix.c
INC := 
DEFS := -DGREATWALL_HASH_BACKEND_SM3
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-13,GreatWall160f,sig,160,sig-13-greatwall160f,ngcc_ref_sig_13_GreatWall160f))

# GreatWall / GreatWall160s (160-bit)
ALGORITHM := GreatWall
SRCDIR := Implementations/Reference_Implementation/GreatWall160s
SRCS := aes.c aes_impl.c api.c faest.c gf163_proof.c hash.c KeccakHash.c KeccakP-1600-reference.c \
    KeccakSponge.c owf_proof.c quicksilver.c small_vole.c transpose.c universal_hash.c util.c     \
    vector_com.c vole_check.c vole_commit.c drng.c SIG_AlgorithmInstance.c randomness_kat_fixed.c \
    auxfunc.c greatwall-160-matrix.c
INC := 
DEFS := -DGREATWALL_HASH_BACKEND_SM3
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-13,GreatWall160s,sig,160,sig-13-greatwall160s,ngcc_ref_sig_13_GreatWall160s))

# GreatWall / GreatWall192f (192-bit)
ALGORITHM := GreatWall
SRCDIR := Implementations/Reference_Implementation/GreatWall192f
SRCS := SIG_AlgorithmInstance.c drng.c api.c aes.c aes_impl.c block.c block_impl.c faest.c        \
    greatwall.c greatwall_impl.c hash.c KeccakHash.c KeccakP-1600-reference.c KeccakSponge.c      \
    owf_proof.c polynomials.c polynomials_impl.c prgs.c quicksilver.c small_vole.c transpose.c    \
    universal_hash.c util.c vector_com.c vole_check.c vole_commit.c randomness_kat_fixed.c        \
    auxfunc.c greatwall-192-matrix.c
INC := 
DEFS := -DGREATWALL_HASH_BACKEND_SM3
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-13,GreatWall192f,sig,192,sig-13-greatwall192f,ngcc_ref_sig_13_GreatWall192f))

# GreatWall / GreatWall192s (192-bit)
ALGORITHM := GreatWall
SRCDIR := Implementations/Reference_Implementation/GreatWall192s
SRCS := SIG_AlgorithmInstance.c drng.c api.c aes.c aes_impl.c block.c block_impl.c faest.c        \
    greatwall.c greatwall_impl.c hash.c KeccakHash.c KeccakP-1600-reference.c KeccakSponge.c      \
    owf_proof.c polynomials.c polynomials_impl.c prgs.c quicksilver.c small_vole.c transpose.c    \
    universal_hash.c util.c vector_com.c vole_check.c vole_commit.c randomness_kat_fixed.c        \
    auxfunc.c greatwall-192-matrix.c
INC := 
DEFS := -DGREATWALL_HASH_BACKEND_SM3
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-13,GreatWall192s,sig,192,sig-13-greatwall192s,ngcc_ref_sig_13_GreatWall192s))

# GreatWall / GreatWall256f (256-bit)
ALGORITHM := GreatWall
SRCDIR := Implementations/Reference_Implementation/GreatWall256f
SRCS := SIG_AlgorithmInstance.c drng.c api.c aes.c aes_impl.c block.c block_impl.c faest.c        \
    greatwall.c greatwall_impl.c hash.c KeccakHash.c KeccakP-1600-reference.c KeccakSponge.c      \
    owf_proof.c polynomials.c polynomials_impl.c prgs.c quicksilver.c small_vole.c transpose.c    \
    universal_hash.c util.c vector_com.c vole_check.c vole_commit.c randomness_kat_fixed.c        \
    auxfunc.c greatwall-256-matrix.c
INC := 
DEFS := -DGREATWALL_HASH_BACKEND_SM3
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-13,GreatWall256f,sig,256,sig-13-greatwall256f,ngcc_ref_sig_13_GreatWall256f))

# GreatWall / GreatWall256s (256-bit)
ALGORITHM := GreatWall
SRCDIR := Implementations/Reference_Implementation/GreatWall256s
SRCS := SIG_AlgorithmInstance.c drng.c api.c aes.c aes_impl.c block.c block_impl.c faest.c        \
    greatwall.c greatwall_impl.c hash.c KeccakHash.c KeccakP-1600-reference.c KeccakSponge.c      \
    owf_proof.c polynomials.c polynomials_impl.c prgs.c quicksilver.c small_vole.c transpose.c    \
    universal_hash.c util.c vector_com.c vole_check.c vole_commit.c randomness_kat_fixed.c        \
    auxfunc.c greatwall-256-matrix.c
INC := 
DEFS := -DGREATWALL_HASH_BACKEND_SM3
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-13,GreatWall256s,sig,256,sig-13-greatwall256s,ngcc_ref_sig_13_GreatWall256s))

