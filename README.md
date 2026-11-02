# NGCC-provider

NGCC (Next-Generation Cryptography Competition) is a global call for next-generation commercial cryptography algorithms, launched in 2025 by the Institute of Commercial Cryptography Standards (ICCS). The [notice on collecting next-generation public-key algorithm proposals](https://www.niccs.org.cn/symmbzyjy/tzgg/pc/content/1976155884915003392/content_1976155884915003392.html) officially published the submission requirements and evaluation criteria for public-key cryptographic algorithms and started accepting public-key algorithm proposals.

The public-key portion (KEM + signature) comprises 74 schemes and 306 parameter sets. This provider integrates 71 schemes and 220 parameter sets (excluding the inefficient 384/512-bit security categories). See the list in [NGCC_PROPOSALS_SUMMARY.md](NGCC_PROPOSALS_SUMMARY.md).

NGCC-provider adapts NGCC proposal reference and author-supplied optimized implementations to the Tongsuo provider interface, providing KEM, signature, signature-key encoding/decoding, and TLS 1.3 capabilities. Proposal algorithm sources are compiled into a single `ngcc.so`; the provider load name is `ngcc`.

Author reference implementations live in the [NGCC-candidates](https://github.com/Tongsuo-Project/NGCC-candidates) repository. This repository mounts that tree as a git submodule at `ngcc/candidates` and does not modify any author algorithm source.

## Disclaimer

**This repository is for testing only and must not be used in production.**

Scheme code comes from implementations submitted by NGCC proposal teams. This repository only performs interface adaptation and makes no guarantees about cryptographic security or implementation security. Passing tests does not mean a scheme is secure.

Many submissions still use a placeholder for the underlying hash algorithm, to be replaced later; so we estimate the test computation costs as only provisional. However, under current parameters, the test communication costs are fixed.

## Documentation

- [Proposal summary](NGCC_PROPOSALS_SUMMARY.md): integrated and unintegrated parameter sets, EVP/TLS call names, source provenance, test results, and limitations.
- [Usage](USAGE.md): C API and command-line examples, including KEM, sign/verify, certificate issuance, and TLS 1.3.
- [Author optimized implementations](make/IMPLEMENTATIONS.md): source import, architecture selection, CPU requirements, compatibility fallbacks, and validation.

## Build

Building requires Perl, make, a C toolchain, and the Tongsuo library.

### Install Tongsuo

Clone the source from [Tongsuo](https://github.com/Tongsuo-Project/Tongsuo). Tongsuo also requires Perl 5 (including the `Text::Template` module), make, and a C compiler.

```sh
git clone https://github.com/Tongsuo-Project/Tongsuo
cd Tongsuo
./config --prefix=$HOME/opt/tongsuo
make -j6
make install
```


### Clone and initialize the reference-implementation repository:

```sh
git submodule update --init
./config OPENSSL_PREFIX=$HOME/opt/tongsuo
make -j6
```

`./config --list` lists targets; `./config --list-algorithms` lists names. Configuration accepts `CC=...`, `LD=...`, `CFLAGS=...`, `CPPFLAGS=...`, `LDFLAGS=...`, `GMP_STATIC=...`, and similar parameters. The current system is detected by default; cross compilation requires an explicit target plus a matching toolchain and dependency libraries.

The output is `ngcc.so`.

### Author optimized implementations

Reference implementations remain the default. To select the proposal teams' own
x86-64 or AArch64 backends, configure and build directly:

```sh
./config --implementation=optimized OPENSSL_PREFIX=$HOME/opt/tongsuo
make -j6
make print-implementations
```

`./config` verifies the optimized source checksums and automatically imports any
missing files from the pinned submission ZIPs, using Perl core modules. No Python
or separate import command is needed. When all files are present, the archives
are unnecessary. To use an archive location other than the repository's parent,
pass `--with-optimized-archives=/path/to/archive-root` to the same configure
command. Archive paths are recorded in `make/optimized-origins.json`.

Configuration verifies archive and member SHA-256 values before installing
missing files and refuses to overwrite changed sources. A failed import leaves
the existing generated configuration unchanged. No network downloads are performed.

Selection depends on the configured target, not runtime CPU detection. The CPU
must support the selected author's instruction set, including AVX-512 or SVE
where specified. `configdata.json` records each selected source directory and its
requirements, or the reason for using the reference implementation. Use
`--implementation=ref` to return to references.

#### C++ backends

Four signature schemes ship their optimized backends as a C++ template codebase
derived from FAEST: Galas (`sign-12`), Lynxer (`sign-14`), ReSolveD-alpha
(`sign-21`) and SYDO (`sign-28`) — 16 parameter sets on `linux-x86_64`. These
encode the parameter set as a template argument, including a `std::pair` used as
a non-type template parameter, so the compiler must support C++20 structural
types. GCC 12 or later and Clang 14 or later work; GCC 11, the default on
Ubuntu 22.04, does not. Pass a suitable C++ compiler with `CXX`:

```sh
CXX=clang++-19 ./config --implementation=optimized OPENSSL_PREFIX=$HOME/opt/tongsuo
make -j6
```

No extra `CXXFLAGS` are required. Clang generates faster code for these four
schemes: on a Xeon Platinum 8575C, signing was 1.4x to 2.3x faster with Clang 19
than with GCC 14 at identical flags.

`make/implementations.json` records the source list for every optimized entry;
entries whose `SRCS` contains a C++ source file are the ones listed above.
`config` does not yet probe the compiler, so an inadequate compiler surfaces as a
compilation error during `make` rather than as a configure-time fallback.

After building, `make test-implementations` builds separate reference instances
and tests all eight reference/optimized key-generation, operation, and
verification combinations. It also checks signature rejection after altering the
message. See [implementation notes](make/IMPLEMENTATIONS.md) for known exclusions
and the distinction between compilation and runtime validation. Python is required
only for the optional test/report tools.

ZEN and DARTS use the author's v2.0 revisions. DARTS's AVX2 and NEON backends
pass the updated reference/optimized interoperability checks. ZEN v2.0 supplies
only x86 AVX2 optimization; ARM targets use its new reference implementation.

### GMP

The following schemes depend on GMP and are disabled by default. To include them in the build, you must specify a GMP path:

- QIMEN-PIKE: 2 parameter sets, `kem-31-ngcc-1`, `kem-31-ngcc-2`.
- SQISign2Dsquare: 12 parameter sets, `sig-25-sqisign2dsq-*`.
- SQIsign2D-push1/2: 2 parameter sets, `sig-26-sqisign2d-lvl1`, `sig-26-sqisign2d-lvl3`.
- SQIsignTriangle: 3 parameter sets, `sig-27-sqisigntriangle-lvl1`, `sig-27-sqisigntriangle-lvl2`, `sig-27-sqisigntriangle-lvl5`.

```sh
./config --with-gmp-source=/path/to/gmp-source
```

The GMP source directory must contain `configure` and must not already be configured; building GMP also requires `m4`.
After configuration, NGCC-provider's `make` builds a PIC static library in `build/gmp/` and links it into `ngcc.so` without modifying the GMP source directory. If you already have a PIC static GMP, you can point to it with `GMP_ROOT=/path/to/gmp-install`; that option is mutually exclusive with the source option. Each configure run must pass the GMP options you need. For cross compilation, set the GMP target with `GMP_HOST=<target-triplet>`.

## Test

Tests cover EVP, TLS, certificates, and interoperability. The actual run set and skipped items follow the [proposal summary](NGCC_PROPOSALS_SUMMARY.md#test-and-source-snapshots). By default, parameter sets whose historical full-case wall-clock time exceeded 1 second are skipped. `FULL_TEST=1` runs all enabled parameters; Python 3.10+ is required.

These are this repository's own correctness acceptance tests. For usage examples, see [Usage](USAGE.md).


```sh
make verify-sources
make test JOBS=6 TEST_TIMEOUT=1800
```
