BW_KEM_C128 Reference Implementation Bundle

This directory is the self-contained ISO C reference implementation bundle for
the BW_KEM_C128 KEM instance.

File summary

  KEM_AlgorithmInstance.h/.c
      ICCS KEM programming interface and BW_KEM_C128 wrapper implementation

  auxfunc.c/.h
      ICCS auxiliary hash and XOF helper functions (bundled unchanged)

  drng.c/.h
      ICCS deterministic random number generator (bundled unchanged)

  KAT_KEM.c
      ICCS KAT driver for key encapsulation mechanisms (bundled unchanged)

  params.h, kem.*, indcpa.*, BWcoding.*, poly.*, polyvec.*
      BW_KEM_C128 reference implementation core

  cbd.*, ntt.*, reduce.*, verify.*, randombytes.*
      Arithmetic, sampling, and support modules required by the reference core

  symmetric.h, symmetric-iccs.c
      BW_KEM_C128 symmetric abstraction and ICCS auxiliary-function backend

Build and KAT generation

  make
      Build the local KAT executable

  make kat
      Generate the KAT file and copy it to:
      ../../../Test_Vector/KAT_KEM_BW_KEM_C128.txt

  make clean
      Remove local build artifacts

Important notes

1.  This bundled implementation is self-contained and does not depend on files
    outside submission/.
2.  The bundled implementation uses the ICCS programming interface in
    KEM_AlgorithmInstance.h.
3.  The cryptographic hash and XOF calls used by the bundled implementation are
    routed through ICCS auxiliary functions in auxfunc.c/.h.
4.  This program assumes a little-endian byte order for multi-byte values.
5.  This program requires compilation with a C99-compatible compiler or later.
