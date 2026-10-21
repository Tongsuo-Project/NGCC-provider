*********************************Instructions**********************************

The following files SHALL NOT be modified:

drng.c                      Source file of Deterministic Random Number Generator

drng.h                      Header file of Deterministic Random Number Generator

auxfunc.c                   Source file of auxiliary functions

auxfunc.h                   Header file of auxiliary functions

KAT_SIG.c                   Source file for generating test vector files of 
                            digital signature (SIG) scheme

KAT_KEM.c                   Source file for generating test vector files of 
                            key encapsulation mechanism (KEM) scheme

KAT_KEX.c                   Source file for generating test vector files of 
                            key exchange (KEX) protocol

The following files NEED to be modified:

SIG_AlgorithmInstance.c     Source file of SIG scheme (demo)

SIG_AlgorithmInstance.h     Header file of SIG scheme (programming interface)

KEM_AlgorithmInstance.c     Source file of KEM scheme (demo)

KEM_AlgorithmInstance.h     Header file of KEM scheme (programming interface)

KEX_AlgorithmInstance.c     Source file of KEX protocol (demo)

KEX_AlgorithmInstance.h     Header file of KEX protocol (programming interface)

**************************************Use**************************************

1.  Modify the provided file "XXX_AlgorithmInstance.h":
    a.  Set the macro "OUTPUT_BLANK_TEST_VECTORS" as 0 to set generating mode.
    b.  Set the macro "ALGORITHM_INSTANCE" as the submitted algorithm instance 
        name.

2.  Modify the provided file "XXX_AlgorithmInstance.c" to implement the 
    functions of the submitted algorithm.

3.  Compile and execute to generate test vector files.

*************************************Notes*************************************

1.  The submitted algorithm shall use the provided programming interface in 
    "XXX_AlgorithmInstance.h".    
2.  This program assumes a little-endian byte order for multi-byte values. 
    Behavior is undefined on systems with a different byte order.
3.  This program requires compilation with compilers supporting the 
    ISO/IEC 9899:1999 (C99) standard or later.
4.  For non-byte-aligned data operations (e.g., DRNG outputs), the most
	significant bit (MSB) first convention applies to partial-byte read/write
    operations.
    Example: the 11-bit non-byte-aligned output "10101100001"(bin) is stored 
    in memory as "AC 20"(hex):
                     MSB     ------->     LSB         
        Address+0  :  1  0  1  0  1  1  0  0  (0xAC)
        Address+1  :  0  0  1  0  0  0  0  0  (0x20, with only 3 valid bits)
5.  The cryptographic hash algorithm and eXtendable-Output Function (XOF) in 
    implementations shall use the auxiliary functions. These auxiliary functions
    are only used to verify the correctness of implementations and preliminarily
    evaluate performance, without considering security. In the subsequent 
    evaluation rounds, these will be replaced with new auxiliary functions based
    on new cryptographic hash algorithms.
6.  The files of KEX are only suitable for 2-pass and 3-pass key exchange 
    protocols. If the submitted key exchange protocol requires more passes, 
    modify "KAT_KEX.c" and refer to the given programming interface to implement
    those passes.

****************************Additional Usage Notes*****************************

1.  Overview

    This directory contains the portable SM3-based implementation of ZEN for
    the 128-bit security level.

    Implementation family: SM3
    Security level: 128-bit security level
    Symmetric primitive: SM3
    Vectorization: Portable C

    Please refer to "params.h" and the local parameter files for the exact
    parameter definitions used by this instance.

2.  Build

    To clean previous build artifacts and build the KEM test programs, run:

        make clean
        make KAT_KEM
        make SPEED_KEM

    The command "make clean" removes previous build artifacts.
    The command "make KAT_KEM" builds the known-answer-test executable.
    The command "make SPEED_KEM" builds the speed-test executable.

3.  Run

    To run the known-answer test, execute:

        ./KAT_KEM

    The program "./KAT_KEM" is used for known-answer tests and for generating
    or verifying KEM test vectors.

    To run the speed test, execute:

        ./SPEED_KEM

    The program "./SPEED_KEM" is used for speed testing of KEM key generation,
    encapsulation, and decapsulation.

4.  Notes

    The source files, local headers, and parameter files are the authoritative
    references for this implementation. If this README and the source code
    differ, the source code and parameter files should be followed.

    This is a portable C implementation and does not require AVX2-specific
    instructions.

    End of additional usage notes.

