# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Phoenix / Phoenix-SM3-128f (128-bit)
ALGORITHM := Phoenix
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Phoenix-SM3-128f
SRCS := address.c counter.c merkle.c octopus.c randombytes.c sign.c tfors.c utils.c utilsx1.c     \
    gwots.c gwotsx1.c SIG_AlgorithmInstance.c drng.c auxfunc.c hash_sm3.c sm3_drng.c              \
    thash_sm3_simple.c
INC := 
DEFS := -DPARAMS=phoenix-sm3-128f -DALLOW_DEEP_TREES -DSUBMISSION_DRNG -DPARAMNAME='"Phoenix-SM3-128f"'
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-19,Phoenix-SM3-128f,sig,128,sig-19-phoenix-sm3-128f,ngcc_ref_sig_19_Phoenix_SM3_128f))

# Phoenix / Phoenix-SM3-128s (128-bit)
ALGORITHM := Phoenix
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Phoenix-SM3-128s
SRCS := address.c counter.c merkle.c octopus.c randombytes.c sign.c tfors.c utils.c utilsx1.c     \
    gwots.c gwotsx1.c SIG_AlgorithmInstance.c drng.c auxfunc.c hash_sm3.c sm3_drng.c              \
    thash_sm3_simple.c
INC := 
DEFS := -DPARAMS=phoenix-sm3-128s -DALLOW_DEEP_TREES -DSUBMISSION_DRNG -DPARAMNAME='"Phoenix-SM3-128s"'
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-19,Phoenix-SM3-128s,sig,128,sig-19-phoenix-sm3-128s,ngcc_ref_sig_19_Phoenix_SM3_128s))

# Phoenix / Phoenix-SM3-192f (192-bit)
ALGORITHM := Phoenix
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Phoenix-SM3-192f
SRCS := address.c counter.c merkle.c octopus.c randombytes.c sign.c tfors.c utils.c utilsx1.c     \
    gwots.c gwotsx1.c SIG_AlgorithmInstance.c drng.c auxfunc.c hash_sm3.c sm3_drng.c              \
    thash_sm3_simple.c
INC := 
DEFS := -DPARAMS=phoenix-sm3-192f -DALLOW_DEEP_TREES -DSUBMISSION_DRNG -DPARAMNAME='"Phoenix-SM3-192f"'
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-19,Phoenix-SM3-192f,sig,192,sig-19-phoenix-sm3-192f,ngcc_ref_sig_19_Phoenix_SM3_192f))

# Phoenix / Phoenix-SM3-192s (192-bit)
ALGORITHM := Phoenix
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Phoenix-SM3-192s
SRCS := address.c counter.c merkle.c octopus.c randombytes.c sign.c tfors.c utils.c utilsx1.c     \
    gwots.c gwotsx1.c SIG_AlgorithmInstance.c drng.c auxfunc.c hash_sm3.c sm3_drng.c              \
    thash_sm3_simple.c
INC := 
DEFS := -DPARAMS=phoenix-sm3-192s -DALLOW_DEEP_TREES -DSUBMISSION_DRNG -DPARAMNAME='"Phoenix-SM3-192s"'
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-19,Phoenix-SM3-192s,sig,192,sig-19-phoenix-sm3-192s,ngcc_ref_sig_19_Phoenix_SM3_192s))

# Phoenix / Phoenix-SM3-256f (256-bit)
ALGORITHM := Phoenix
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Phoenix-SM3-256f
SRCS := address.c counter.c merkle.c octopus.c randombytes.c sign.c tfors.c utils.c utilsx1.c     \
    gwots.c gwotsx1.c SIG_AlgorithmInstance.c drng.c auxfunc.c hash_sm3.c sm3_drng.c              \
    thash_sm3_simple.c
INC := 
DEFS := -DPARAMS=phoenix-sm3-256f -DALLOW_DEEP_TREES -DSUBMISSION_DRNG -DPARAMNAME='"Phoenix-SM3-256f"'
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-19,Phoenix-SM3-256f,sig,256,sig-19-phoenix-sm3-256f,ngcc_ref_sig_19_Phoenix_SM3_256f))

# Phoenix / Phoenix-SM3-256s (256-bit)
ALGORITHM := Phoenix
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/Phoenix-SM3-256s
SRCS := address.c counter.c merkle.c octopus.c randombytes.c sign.c tfors.c utils.c utilsx1.c     \
    gwots.c gwotsx1.c SIG_AlgorithmInstance.c drng.c auxfunc.c hash_sm3.c sm3_drng.c              \
    thash_sm3_simple.c
INC := 
DEFS := -DPARAMS=phoenix-sm3-256s -DALLOW_DEEP_TREES -DSUBMISSION_DRNG -DPARAMNAME='"Phoenix-SM3-256s"'
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-19,Phoenix-SM3-256s,sig,256,sig-19-phoenix-sm3-256s,ngcc_ref_sig_19_Phoenix_SM3_256s))

