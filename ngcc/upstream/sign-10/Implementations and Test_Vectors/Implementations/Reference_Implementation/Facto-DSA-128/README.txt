Facto-DSA reference implementation, instance Facto-DSA-128

Files:
  SIG_AlgorithmInstance.h  NGCC SIG API declarations and instance parameters.
  SIG_AlgorithmInstance.c  ISO C implementation of keygen/sign/verify.
  KAT_SIG.c                ICCS KAT generator for signatures.
  auxfunc.c/.h             ICCS auxiliary SM3/pseudoXOF functions.
  drng.c/.h                ICCS deterministic RNG.
  Makefile                 Build and KAT generation script.

Build:
  make

Generate the KAT file:
  make run

The generated file is output/KAT_SIG_Facto-DSA-128.txt.
This implementation is single-threaded and uses no platform-specific instructions.

Build the lib .so library file

make libfactodsa128-ref.so