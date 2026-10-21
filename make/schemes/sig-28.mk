# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# SYDO / sydo_160f (160-bit)
ALGORITHM := SYDO
SRCDIR := Implementations/Reference_Implementation/sydo_160f
SRCS := SIG_AlgorithmInstance.c src/aes.c src/bavc.c src/compat.c src/fields.c src/randomness.c   \
    src/sydo.c src/field.c src/hash.c src/macs.c src/primitives.c src/quicksilver.c src/rsd.c     \
    src/vole_check.c src/universal_hashing.c src/utils.c src/vole.c lib/auxfunc.c lib/drng.c      \
    lib/blake2/ref/blake2b-ref.c
INC := -Isrc/sydo_160f/src -Isrc/sydo_160f/lib -Isrc/sydo_160f/lib/blake2/ref
DEFS := -DNDEBUG -DHAVE_SYS_RANDOM_H -DHAVE_GETRANDOM -DSYDO_REF_NGCC
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-28,sydo_160f,sig,160,sig-28-sydo-160f,ngcc_ref_sig_28_sydo_160f))

# SYDO / sydo_160s (160-bit)
ALGORITHM := SYDO
SRCDIR := Implementations/Reference_Implementation/sydo_160s
SRCS := SIG_AlgorithmInstance.c src/aes.c src/bavc.c src/compat.c src/fields.c src/randomness.c   \
    src/sydo.c src/field.c src/hash.c src/macs.c src/primitives.c src/quicksilver.c src/rsd.c     \
    src/vole_check.c src/universal_hashing.c src/utils.c src/vole.c lib/auxfunc.c lib/drng.c      \
    lib/blake2/ref/blake2b-ref.c
INC := -Isrc/sydo_160s/src -Isrc/sydo_160s/lib -Isrc/sydo_160s/lib/blake2/ref
DEFS := -DNDEBUG -DHAVE_SYS_RANDOM_H -DHAVE_GETRANDOM -DSYDO_REF_NGCC
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-28,sydo_160s,sig,160,sig-28-sydo-160s,ngcc_ref_sig_28_sydo_160s))

# SYDO / sydo_256f (256-bit)
ALGORITHM := SYDO
SRCDIR := Implementations/Reference_Implementation/sydo_256f
SRCS := SIG_AlgorithmInstance.c src/aes.c src/bavc.c src/compat.c src/fields.c src/randomness.c   \
    src/sydo.c src/field.c src/hash.c src/macs.c src/primitives.c src/quicksilver.c src/rsd.c     \
    src/vole_check.c src/universal_hashing.c src/utils.c src/vole.c lib/auxfunc.c lib/drng.c      \
    lib/blake2/ref/blake2b-ref.c
INC := -Isrc/sydo_256f/src -Isrc/sydo_256f/lib -Isrc/sydo_256f/lib/blake2/ref
DEFS := -DNDEBUG -DHAVE_SYS_RANDOM_H -DHAVE_GETRANDOM -DSYDO_REF_NGCC
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-28,sydo_256f,sig,256,sig-28-sydo-256f,ngcc_ref_sig_28_sydo_256f))

# SYDO / sydo_256s (256-bit)
ALGORITHM := SYDO
SRCDIR := Implementations/Reference_Implementation/sydo_256s
SRCS := SIG_AlgorithmInstance.c src/aes.c src/bavc.c src/compat.c src/fields.c src/randomness.c   \
    src/sydo.c src/field.c src/hash.c src/macs.c src/primitives.c src/quicksilver.c src/rsd.c     \
    src/vole_check.c src/universal_hashing.c src/utils.c src/vole.c lib/auxfunc.c lib/drng.c      \
    lib/blake2/ref/blake2b-ref.c
INC := -Isrc/sydo_256s/src -Isrc/sydo_256s/lib -Isrc/sydo_256s/lib/blake2/ref
DEFS := -DNDEBUG -DHAVE_SYS_RANDOM_H -DHAVE_GETRANDOM -DSYDO_REF_NGCC
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-28,sydo_256s,sig,256,sig-28-sydo-256s,ngcc_ref_sig_28_sydo_256s))

