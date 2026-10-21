# NGCC uniform KAT harness

Every candidate folder (`sign-NN`, `kem-NN`, `kex-NN`, `hash-NN`) gets a
`Makefile` that builds each parameter set of the **reference implementation**
into a shared library `lib/lib<Instance>.so`, and `bin/ngcc_kat` loads any such
library, prints its metadata block and reproduces the official KAT files.

```
make -C api harness              # builds bin/ngcc_kat
make -C kex-07 -j8               # builds kex-07/lib/lib*.so
make -C kex-07 -j8 test          # runs the harness on each lib, results/summary.tsv
bin/ngcc_kat kex-07/lib/libNEV_AKE_512_769.so --meta-only
make -j16 all test               # everything, from the repository root
make manifest                    # hash the reference KATs into <candidate>/kat.sha256
make clean                       # remove every build output and the harness
```

## KAT manifests (kat.sha256)

The reference KAT files total 11 GB and are not kept in git. Instead each
candidate has a `kat.sha256` manifest with one line per reference file:

```
<sha256 hex> <strict|nolen>[:records] Aigis-enc1/KAT_KEM_Aigis-enc1.txt
```

The name is `<make label>/<reference file name>`, so instances whose KAT
files share a name but live in different directories (e.g. two hash backends)
get separate entries. The digest is over the canonical form of the file: CR and trailing blanks
stripped, empty lines dropped, lines joined by `\n`. Mode `nolen` also drops
every `X_Len = ...` line; it is chosen automatically for references that were
generated in template mode (`OUTPUT_BLANK_TEST_VECTORS 1`, blank `_Len`
values). `make test` passes `--kat-sha kat.sha256` whenever the manifest
exists, and the harness then compares the digest of what it generated against
the manifest, so a fresh clone can verify every candidate without the
reference files. Entries for the long hash vectors (`KAT_2_33`, `KAT_Loop`)
are included, so `--full`/`--loop` runs verify by hash too. Regenerate with
`make manifest` in a candidate folder (needs its Test_Vectors present) or
`make manifest` at the root.

## Repository layout and what git tracks

```
<id>/                 one folder per candidate (sign-NN, kem-NN, kex-NN, hash-NN),
                      holding the included reference sources, our Makefile,
                      kat.sha256 and (where needed) patches/
orig/<id>/orig.zip    optional original archive fetched by ../download.sh (untracked)
api/                  harness sources, link headers, generic make rules
bin/ngcc_kat          the harness binary (built, untracked)
tools/                reproducers for the reported defects (see tools/README.md)
```

The curated reference source trees and harness files are tracked. Original
archives, libraries, build outputs and results are ignored (see
`../.gitignore`).

## Files

| file | purpose |
|---|---|
| `api/link_common.h` | `ngcc_meta_common_t` header of the metadata block, type enum, `ngcc_meta()`/`ngcc_seed()` prototypes |
| `api/link_kem.h` `link_sig.h` `link_kex.h` `link_hash.h` | per-type metadata struct (sizes in bytes), function-pointer table and the official symbol names |
| `api/link_shim.c` | compiled into every library: defines `drng_algorithm`, `ngcc_meta()`, `ngcc_seed()`, `ngcc_random()` |
| `api/link_exports.map` | linker version script: only the official API, DRNG and `ngcc_*` symbols are exported |
| `api/link_rules.mk` + `link_finish.mk` | generic make rules included by every candidate Makefile |
| `api/drng.c` `auxfunc.c` | official ICCS DRNG and auxiliary hash functions, used when the instance directory has no copy |
| `api/ngcc_kat.c` | the master harness |

## Metadata block

`ngcc_meta()` returns a pointer to a struct that starts with
`ngcc_meta_common_t` (magic, ABI, type, candidate id, algorithm, instance,
variant, source dir, compiler, flags) followed by the type-specific sizes:

- kem: `pk_len sk_len ct_len ss_len`
- sig: `pk_len sk_len sn_len`
- kex: `passes pk_len sk_len sta_len stb_len ss_len total_msg_len`
- hash: `digest_bits digest_len`

Sizes are what the implementation itself claims through its
`*_get_*_len_bytes()` functions (or `DIGEST_BIT_LENGTH` for hashes).

## Harness

`ngcc_kat [--out-dir D] [--kat-dir D] [--kat-sha F] [--write-manifest F] [--kat-name N] [--count N] [--loop] [--full] [--timeout S] [--mem-limit MB] [--meta-only] [--quiet] lib.so`

It replays the official `KAT_KEM.c` / `KAT_SIG.c` / `KAT_KEX.c` /
`KAT_CryptHash.c` drivers exactly (same seed schedule, same text format) and
compares line by line with the submitted `KAT_<TYPE>_<instance>.txt`, ignoring
CR/LF differences. Every buffer handed to the candidate has 64 guard bytes on
each side; a write outside the claimed length is reported as `OVERFLOW`. For
signatures it additionally checks that a flipped message bit is rejected.
Hash runs `KAT_2_12` and `KAT_2_23` by default; `--loop` adds the
million-iteration loop (minutes on reference code) and `--full` adds the
2^33-bit message (1 GiB).

The last stdout line is always `RESULT <id> <instance> <STATUS> [detail]` with
STATUS one of PASS, MISMATCH, NOKAT, CRYPTOFAIL, OVERFLOW, LOADFAIL, TIMEOUT,
CRASH; the exit code is the index in that list.

## Writing a candidate Makefile

```make
NGCC_ID   := kem-01
NGCC_TYPE := kem                # kem | sig | kex | hash
NGCC_ALG  := Aigis-Enc+
include ../api/link_rules.mk

REF := Implementations/Reference_Implementation
NGCC_KAT_DIR := Test_Vectors    # dir holding KAT_KEM_<instance>.txt

# defaults: every *.c *.cpp *.S directly in the dir, minus driver/benchmark
# files (KAT_*, main*, test*, bench*, speed*, cpucycles*, ...); api/drng.c and
# api/auxfunc.c are added when the dir has no own copy
$(eval $(call ngcc_instance,CHAMP-512,$(REF)/CHAMP-512))

# overrides go BEFORE the ngcc_instance call for that label
SRCS_Aigis-enc1 := cbd.c ... kat_test/KEM_AlgorithmInstance.c kat_test/drng.c kat_test/rng.c
INC_Aigis-enc1  := -Isrc/Aigis-enc1/kat_test
DEFS_Aigis-enc1 := -DPARAMS=1 -DUSE_NICCS_API
SHIMDEFS_Aigis-enc1 := -DNGCC_INSTANCE_HEADER='"kat_test/KEM_AlgorithmInstance.h"'
$(eval $(call ngcc_instance,Aigis-enc1,$(REF)/Aigis-Enc+-I))

include ../api/link_finish.mk
```

All overrides: `SRCS_ INC_ DEFS_ CFLAGS_ CXXFLAGS_ LDLIBS_ SHIMDEFS_ NOAUX_
NODRNG_ KATDIR_ KATNAME_ TESTFLAGS_` (see the header of `link_rules.mk`).
For very slow reference code set `TESTFLAGS_<label> := --count 3`; `make
manifest` then records a digest over the first 3 records (`strict:3`) and
`make test` verifies exactly that prefix.
Source paths in `SRCS_` are relative to `src/<label>`, which is a symlink to
the instance directory (so directory names with spaces are never a problem).
`SHIMDEFS_` options: `-DNGCC_INSTANCE="name"` when the header does not define
`ALGORITHM_INSTANCE`, `-DNGCC_INSTANCE_HEADER='"x.h"'` when it is not named
`<TYPE>_AlgorithmInstance.h`, `-DNGCC_DIGEST_BITS=N` for hashes without
`DIGEST_BIT_LENGTH`, `-DNGCC_NO_DRNG` when the candidate defines
`drng_algorithm` itself.

Rules that every candidate Makefile follows:

1. Build only from source. Never link shipped `.o`/`.a`/`.so`, never run shipped
   binaries, never invoke the candidate's own Makefile, CMake, meson or scripts.
   Read them for the source list and defines only.
2. Reference implementation only; one library per parameter set. Label each
   instance so that `lib<label>.so` is unambiguous.
3. Randomness must come from `drng_algorithm`. If the candidate has a
   `randombytes.c` reading `/dev/urandom` and a `rng.c` wrapping the DRNG, use
   the latter. A KAT `MISMATCH` is the usual symptom of getting this wrong.
4. Do not edit submission files. If a source needs a fix, put a patched copy
   under `<candidate>/patches/` and reference it from `SRCS_` with `../../patches/...`
   (paths are relative to `src/<label>`), documenting why in the Makefile.
5. Record any external library (`-lgmp`, `-lcrypto`) in `LDLIBS_`.
