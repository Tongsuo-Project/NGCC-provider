# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# Lynxer / Lynxer-160f (160-bit)
ALGORITHM := Lynxer
SRCDIR := Implementations/Reference_Implementation/Lynxer-160f
SRCS := bavc.c drng.c enc.c prg.c fields.c instances.c lynx_matrices.c quicksilver.c owf.c        \
    random_oracle.c tccr.c universal_hashing.c utils.c voleith_impl.c vole.c auxfunc.c            \
    SIG_AlgorithmInstance.c
INC := 
DEFS := -DXOF_PSEUDO
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-14,Lynxer-160f,sig,160,sig-14-lynxer-160f,ngcc_ref_sig_14_Lynxer_160f))

# Lynxer / Lynxer-160s (160-bit)
ALGORITHM := Lynxer
SRCDIR := Implementations/Reference_Implementation/Lynxer-160s
SRCS := bavc.c drng.c enc.c prg.c fields.c instances.c lynx_matrices.c quicksilver.c owf.c        \
    random_oracle.c tccr.c universal_hashing.c utils.c voleith_impl.c vole.c auxfunc.c            \
    SIG_AlgorithmInstance.c
INC := 
DEFS := -DXOF_PSEUDO
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-14,Lynxer-160s,sig,160,sig-14-lynxer-160s,ngcc_ref_sig_14_Lynxer_160s))

# Lynxer / Lynxer-256f (256-bit)
ALGORITHM := Lynxer
SRCDIR := Implementations/Reference_Implementation/Lynxer-256f
SRCS := bavc.c drng.c enc.c prg.c fields.c instances.c lynx_matrices.c quicksilver.c owf.c        \
    random_oracle.c tccr.c universal_hashing.c utils.c voleith_impl.c vole.c auxfunc.c            \
    SIG_AlgorithmInstance.c
INC := 
DEFS := -DXOF_PSEUDO
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-14,Lynxer-256f,sig,256,sig-14-lynxer-256f,ngcc_ref_sig_14_Lynxer_256f))

# Lynxer / Lynxer-256s (256-bit)
ALGORITHM := Lynxer
SRCDIR := Implementations/Reference_Implementation/Lynxer-256s
SRCS := bavc.c drng.c enc.c prg.c fields.c instances.c lynx_matrices.c quicksilver.c owf.c        \
    random_oracle.c tccr.c universal_hashing.c utils.c voleith_impl.c vole.c auxfunc.c            \
    SIG_AlgorithmInstance.c
INC := 
DEFS := -DXOF_PSEUDO
INSTANCE_CFLAGS := 
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-14,Lynxer-256s,sig,256,sig-14-lynxer-256s,ngcc_ref_sig_14_Lynxer_256s))

