# Author optimized implementations

`implementations.json` selects only implementations shipped in the pinned NGCC
submission archives. The provider does not implement new cryptographic kernels.
The registered EVP names, parameter sets and OIDs remain unchanged. ZEN and DARTS
use the author's v2.0 revisions; validation of the earlier revisions does not
establish compatibility or correctness of these new versions.

## Sources and reproducibility

The existing `ngcc/metadata/sources.sha256` protects the reference sources.
`optimized-sources.sha256` covers 11,848 unchanged files from 66 pinned archives.
`optimized-origins.json` maps every imported path to its ZIP member, archive
SHA-256, archive location and source URL. Some proposals share common sources;
these are imported together with their optimized directories. Imported files also
include implementations that cannot currently be selected, for the reasons below.

ZEN and DARTS exactly match the candidate repository's `main` revision
`c09784c0f4655bf5a57e124b74e5325581fd0599`: ZEN's 2026-10-30 v2.0 archive and
DARTS author commit `98687cf6910c1409d188464c9ceff69cbf409cc4`. Their 630 tracked
implementation files were compared byte-for-byte with those author archives.
The archives are bundled in the candidate repository, and their locations are
recorded in `optimized-origins.json`. No files from the superseded ZEN ARM/SVE
backend are mixed into the new revision.

Initialize the `ngcc/candidates` submodule, then select optimized implementations
through the ordinary configuration entry point:

```sh
./config --implementation=optimized OPENSSL_PREFIX=/path/to/tongsuo
make -j6
```

Configuration checks installed sources first. Missing files are imported
automatically by `make/optimized-sources.pl`, using Perl core modules, with no
Python dependency or external extraction command. Already complete source trees
do not require the original ZIPs. Otherwise, the default archive root is the
provider's parent directory; override it on the same command with
`--with-optimized-archives=/path/to/project_nsmprovider`. For example, a recorded
archive path can be `ngcc_provider/ngcc/metadata/kem-01-revision.zip`.

Importing is idempotent and rejects a mismatched archive, a changed member or a
changed destination. All missing members are staged and checked before they are
installed. An import failure does not rewrite the existing Makefile, config
header or `configdata.json`. No author algorithm files are patched. The source
aliases used by make live under `build/sources`, outside the author tree.

`make verify-sources` checks references and, when optimized instances are selected,
the optimized manifest. `make verify-optimized-sources` checks that manifest
explicitly. A reference-only build does not require the extra imports.

## Selection and requirements

`./config --implementation=ref` is the default. `--implementation=optimized`
selects the matching author backend for each enabled parameter set. The four
targets have these catalog capacities, including GMP-dependent parameters:

- `linux-x86_64`: 191 parameter sets from 61 proposals.
- `linux-aarch64`: 56 parameter sets from 21 proposals.
- `darwin64-x86_64-cc`: 129 parameter sets from 42 proposals.
- `darwin64-arm64-cc`: 51 parameter sets from 19 proposals.

These counts include optimized C, intrinsics and assembly; they are not assembly
counts or performance claims. Existing default exclusions and GMP selection still
apply. `configdata.json` records the exact source list, flags, instruction-set
requirements and fallback reasons. `make print-implementations` reports the actual
selected variants. Objects are separated by target, parameter and variant under
`build/ngcc`, including when building references for interoperability tests.

Instance linking also isolates tentative (`COMMON`) globals such as the authors'
random-generator state. On Darwin, `ld -r -d` allocates them before `nmedit`
localizes symbols. On ELF, `objcopy` gives remaining common symbols an
instance-specific prefix before localizing ordinary definitions. Each link checks
the resulting symbol list; the final provider still exports only
`OSSL_provider_init`. This prevents unrelated proposals from sharing a same-named
global variable.

This is a build-time selection, without a new provider CPU dispatcher. Run the
result only on a CPU supporting the selected flags. In particular, TRIKE's
optimized backend needs AVX-512/VPCLMULQDQ; ARM backends using SVE cannot run on
Apple Silicon. Compiler flags, including the complete `INSTANCE_CFLAGS`, are the
authoritative instruction-set requirements. Do not copy a module to a weaker CPU.

Galas, SYDO, Lynxer and ReSolveD-alpha use author C++ sources. Select a compiler
supporting the required C++23 features with `CXX`; the C++23 validation used LLVM
19. Older Apple Clang versions may reject the authors' template code. `CXXFLAGS`
is available for additional compiler options. The normal platform C++ runtime is
linked when needed.

The small files in `make/compat` handle platform headers, allocation and API
linkage. Galas calls the author's C++ API through a C ABI adapter. Darwin's
allocation adapter handles the author's four-byte-aligned 160-bit block types.
The CS header adapter resolves an unused external declaration conflicting with
the author's internal definition. These adapters contain no cryptographic
implementation. Compiler options also select the authors' GNU inline convention
and map their byte-swap intrinsic spelling to the compiler intrinsic.

## Compatibility fallbacks

An optimized source directory alone is insufficient to replace an existing EVP
algorithm. The catalog retains a reference implementation when necessary:

- BIKE-MLThre and QUBE: the optimized key/shared-secret sizes differ from the
  registered reference ABI.
- PolarLAC and Phoenix on x86: cross-implementation tests reject reference
  ciphertexts or signatures. DARTS v2.0 passes the reference/optimized tests on
  both macOS AArch64 and x86-64 (Rosetta), so both author backends are enabled.
- ZEN v2.0 supplies only x86 AVX2 optimization. AArch64 falls back to the new
  reference implementation; the former ARM/SVE backend belonged to the old
  submission and is removed. The supplied x86 assembly uses ELF, so macOS also
  uses the reference implementation.
- Rudraksh2-256: the supplied assembly passes five arguments to a four-argument
  macro; Clang rejects that unmodified source.
- UVW: the supplied signature wrapper accepts an altered message. Both the
  reference baseline and optimized implementation fail this rejection check;
  optimized substitution is disabled. This does not fix the pre-existing
  reference behavior.
- Platform-specific ELF assembly, SVE requirements or the absence of a matching
  backend prevent selection on other targets. The reason is recorded per entry.

These fallbacks avoid silently changing existing wire formats or editing the
authors' cryptographic code. No result here establishes cryptographic security.

## AArch64 assembly in the pinned submissions

Three of the 75 currently pinned submissions contain independent AArch64 assembly
source: DKEM, Lynxer and ReSolveD-alpha. The latter two bundle a Keccak/NEON component;
that is not an entire AArch64 signature backend. Counting the inline assembly in
SQIsignTriangle's ARM AES random-generator component gives four submissions.
Cortex-M assembly and C intrinsics are excluded from this count.
The earlier count of four independent-assembly proposals included the superseded
ZEN submission; the ZEN v2.0 archive has no AArch64 implementation.

DKEM supplies Apple symbol handling. Its `DKE_USE_AARCH64_NATIVE` path is enabled
on both ARM targets, with the actual NTT/multiplication assembly in the object
list. Both DKEM-128 and DKEM-256 passed all eight reference/optimized combinations
and EVP tests on the local Apple Silicon machine. The bundled
Lynxer/ReSolveD-alpha Keccak assembly cannot be directly linked as supplied on
Mach-O and is not selected for the existing SM3 signature registrations.

## Validation

```sh
make verify-sources
python3 cc_test/ngcc_configure.py
make test-implementations JOBS=6 TEST_TIMEOUT=1800
```

Python is used only by optional test/report tools. Configuration, source import,
checksum verification and compilation do not invoke Python.

The interoperability helper links reference and optimized instances into separate
modules, checks ABI sizes, and exercises all eight combinations of key producer,
encapsulator/signer and decapsulator/verifier. Signatures also undergo an altered
message rejection check. Reports include object hashes and per-instance logs in
`test-results/implementation-interop.json`.

Runtime tests on macOS ARM and macOS x86 (Rosetta) are distinct from the Linux
cross-compilation checks. Linux object construction and symbol isolation do not
establish Linux runtime interoperability. AVX-512 paths require testing
on suitable hardware. See `implementation-validation.json` for the validation
snapshot and its limits. Existing reference-only TLS benchmark results must not
be presented as optimized-backend measurements.

The v2.0 revision validation passed 128 configuration checks and imported all 399
ZEN/DARTS optimized files from their two real archives with only Perl on PATH.
DARTS-128/256 passed all eight reference/optimized combinations on both macOS
ARM64 and x86-64 (Rosetta). The final 210-instance ARM64 module passed 53 EVP
cases (all 51 selected optimizations plus both ZEN references), six focused
ZEN/DARTS TLS/certificate cases, concurrency/lifecycle tests and the export audit.

Linux checks freshly compiled both reference and selected implementations for
the four revised parameters on both architectures. Aggregate links cover all
191 x86-64 and 56 AArch64 selected optimizations; unchanged objects are reused
only after their hashes and recipes match the earlier validation snapshot.
These are compilation and link checks, not Linux execution. The earlier snapshot
is retained in `implementation-validation-before-v2.json` with its original
source and module hashes.

The candidate repository's broad legacy `provenance/sources.sha256` also lists
two missing, untracked Octarine-512 test executables inherited from `main`.
They are outside the provider manifests and remain unchanged; all 9,702 provider
reference files and 11,848 optimized-manifest files pass checksum verification.
