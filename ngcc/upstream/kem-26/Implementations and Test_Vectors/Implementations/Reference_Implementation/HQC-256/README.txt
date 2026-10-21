NSS-HQC implementation instance

This directory follows the API_PKC KEM interface and uses the official auxfunc,
drng, and KAT_KEM files unchanged.

Implemented protocol components:

- public API and KAT integration;
- parameter-derived sizes;
- deterministic XOF-based domain separation;
- seed-based fixed-weight sampling;
- binary ring multiplication by sparse vectors;
- dither derivation from salt || pk || "dither";
- block quantization and decompression;
- concatenated RS over GF(256) and duplicated RM(1,7) code layer;
- FO-style re-encryption check with constant-time shared-secret selection.
- selftest target covering ring, sampling, quantization, and dither plumbing.

Review note: the code layer now follows the NSS_HQC_tex_v4 structure with
GF(256) arithmetic, RS encoding, duplicated RM(1,7) inner decoding, reliability
thresholding, and an algebraic RS errors-and-erasures reference decoder. The
algorithm group still needs to confirm the final NSS-HQC-128 message-length
interpretation because v4 lists k1 = 16 while kappa = 256.

The optimized implementation is currently a byte-identical portable baseline.
Future AVX2/PCLMULQDQ replacements must preserve KAT equality with the reference
implementation.
