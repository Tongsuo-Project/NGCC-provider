# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Qing Luan / QingLuan-128 (128-bit)
ALGORITHM := Qing Luan
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/QingLuan-128
SRCS := src/fq_arith.c src/rsdp.c src/restr.c src/mpc.c src/keygen.c src/sign.c src/verify.c      \
    api_pkc/drng.c api_pkc/auxfunc.c api_pkc/SIG_AlgorithmInstance.c api_pkc/utils_adapter.c      \
    api_pkc/hash_adapter.c
INC := -Isrc/QingLuan-128/api_pkc -Isrc/QingLuan-128/include
DEFS := -DQINGLUAN_128
INSTANCE_CFLAGS := -std=c11
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-20,QingLuan-128,sig,128,sig-20-qingluan-128,ngcc_ref_sig_20_QingLuan_128))

# Qing Luan / QingLuan-256 (256-bit)
ALGORITHM := Qing Luan
SRCDIR := Implementations and Test_Vectors/Implementations/Reference_Implementation/QingLuan-256
SRCS := src/fq_arith.c src/rsdp.c src/restr.c src/mpc.c src/keygen.c src/sign.c src/verify.c      \
    api_pkc/drng.c api_pkc/auxfunc.c api_pkc/SIG_AlgorithmInstance.c api_pkc/utils_adapter.c      \
    api_pkc/hash_adapter.c
INC := -Isrc/QingLuan-256/api_pkc -Isrc/QingLuan-256/include
DEFS := -DQINGLUAN_256
INSTANCE_CFLAGS := -std=c11
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-20,QingLuan-256,sig,256,sig-20-qingluan-256,ngcc_ref_sig_20_QingLuan_256))

