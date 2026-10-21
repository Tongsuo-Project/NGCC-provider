# Frozen author ref sources and parameters; edit explicitly when updating a proposal.

# ReSolveD-alpha / ReSolveD-alpha-160f (160-bit)
ALGORITHM := ReSolveD-alpha
SRCDIR := Implementations/Reference_Implementation/ReSolveD-alpha-160f
SRCS := bavc.c drng.c enc.c prg.c fields.c instances.c quicksilver.c rsd.c random_oracle.c tccr.c \
    universal_hashing.c utils.c voleith_impl.c vole.c auxfunc.c SIG_AlgorithmInstance.c
INC := 
DEFS := -DXOF_PSEUDO
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-21,ReSolveD-alpha-160f,sig,160,sig-21-resolved-alpha-160f,ngcc_ref_sig_21_ReSolveD_alpha_160f))

# ReSolveD-alpha / ReSolveD-alpha-160s (160-bit)
ALGORITHM := ReSolveD-alpha
SRCDIR := Implementations/Reference_Implementation/ReSolveD-alpha-160s
SRCS := bavc.c drng.c enc.c prg.c fields.c instances.c quicksilver.c rsd.c random_oracle.c tccr.c \
    universal_hashing.c utils.c voleith_impl.c vole.c auxfunc.c SIG_AlgorithmInstance.c
INC := 
DEFS := -DXOF_PSEUDO
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-21,ReSolveD-alpha-160s,sig,160,sig-21-resolved-alpha-160s,ngcc_ref_sig_21_ReSolveD_alpha_160s))

# ReSolveD-alpha / ReSolveD-alpha-256f (256-bit)
ALGORITHM := ReSolveD-alpha
SRCDIR := Implementations/Reference_Implementation/ReSolveD-alpha-256f
SRCS := bavc.c drng.c enc.c prg.c fields.c instances.c quicksilver.c rsd.c random_oracle.c tccr.c \
    universal_hashing.c utils.c voleith_impl.c vole.c auxfunc.c SIG_AlgorithmInstance.c
INC := 
DEFS := -DXOF_PSEUDO
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-21,ReSolveD-alpha-256f,sig,256,sig-21-resolved-alpha-256f,ngcc_ref_sig_21_ReSolveD_alpha_256f))

# ReSolveD-alpha / ReSolveD-alpha-256s (256-bit)
ALGORITHM := ReSolveD-alpha
SRCDIR := Implementations/Reference_Implementation/ReSolveD-alpha-256s
SRCS := bavc.c drng.c enc.c prg.c fields.c instances.c quicksilver.c rsd.c random_oracle.c tccr.c \
    universal_hashing.c utils.c voleith_impl.c vole.c auxfunc.c SIG_AlgorithmInstance.c
INC := 
DEFS := -DXOF_PSEUDO
INSTANCE_CFLAGS := -std=c99
SHIMDEFS := 
NOAUX := 
$(eval $(call ngcc_instance,sign-21,ReSolveD-alpha-256s,sig,256,sig-21-resolved-alpha-256s,ngcc_ref_sig_21_ReSolveD_alpha_256s))

