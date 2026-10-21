/*
 * NGCC link shim: compiled into every candidate library.
 *
 * Provides the metadata block, the shared `drng_algorithm` DRNG context that
 * the candidate code references via `extern DRNG_ctx drng_algorithm;`, and
 * the ngcc_seed()/ngcc_random() entry points used by the harness.
 *
 * Compile-time configuration (set by the per-candidate Makefile):
 *   -DNGCC_BUILD_KEM | -DNGCC_BUILD_SIG | -DNGCC_BUILD_KEX | -DNGCC_BUILD_HASH
 *   -DNGCC_ID="kex-07"            candidate folder id
 *   -DNGCC_ALG="NEV-AKE"          algorithm name
 *   -DNGCC_VARIANT="Reference_Implementation"
 *   -DNGCC_SRCDIR="..."           instance source dir (relative)
 *   -DNGCC_FLAGS="-O2"            flags used for the candidate sources
 *   -DNGCC_INSTANCE="name"        optional; default: ALGORITHM_INSTANCE from
 *                                 the candidate's *_AlgorithmInstance.h
 *   -DNGCC_INSTANCE_HEADER=<file> optional; header to include instead of the
 *                                 standard *_AlgorithmInstance.h
 *   -DNGCC_DIGEST_BITS=N          hash only, optional; default DIGEST_BIT_LENGTH
 *   -DNGCC_NO_DRNG                do not define drng_algorithm (candidate
 *                                 defines it itself)
 */
#include <string.h>
#include "drng.h"

#if defined(NGCC_BUILD_KEM)
#  include "link_kem.h"
#  ifndef NGCC_INSTANCE_HEADER
#    define NGCC_INSTANCE_HEADER "KEM_AlgorithmInstance.h"
#  endif
#elif defined(NGCC_BUILD_SIG)
#  include "link_sig.h"
#  ifndef NGCC_INSTANCE_HEADER
#    define NGCC_INSTANCE_HEADER "SIG_AlgorithmInstance.h"
#  endif
#elif defined(NGCC_BUILD_KEX)
#  include "link_kex.h"
#  ifndef NGCC_INSTANCE_HEADER
#    define NGCC_INSTANCE_HEADER "KEX_AlgorithmInstance.h"
#  endif
#elif defined(NGCC_BUILD_HASH)
#  include "link_hash.h"
#  ifndef NGCC_INSTANCE_HEADER
#    define NGCC_INSTANCE_HEADER "CryptHash_AlgorithmInstance.h"
#  endif
#else
#  error "define one of NGCC_BUILD_KEM, NGCC_BUILD_SIG, NGCC_BUILD_KEX, NGCC_BUILD_HASH"
#endif

#ifndef NGCC_INSTANCE_HEADER_NONE
#  include NGCC_INSTANCE_HEADER
#endif

#ifndef NGCC_INSTANCE
#  ifdef ALGORITHM_INSTANCE
#    define NGCC_INSTANCE ALGORITHM_INSTANCE
#  else
#    error "NGCC_INSTANCE not given and ALGORITHM_INSTANCE not defined by header"
#  endif
#endif
#ifndef NGCC_ID
#  define NGCC_ID "unknown"
#endif
#ifndef NGCC_ALG
#  define NGCC_ALG "unknown"
#endif
#ifndef NGCC_VARIANT
#  define NGCC_VARIANT "Reference_Implementation"
#endif
#ifndef NGCC_SRCDIR
#  define NGCC_SRCDIR ""
#endif
#ifndef NGCC_FLAGS
#  define NGCC_FLAGS ""
#endif

#if defined(__clang__)
#  define NGCC_COMPILER "clang " __clang_version__
#elif defined(__GNUC__)
#  define NGCC_COMPILER "gcc " __VERSION__
#else
#  define NGCC_COMPILER "unknown"
#endif

/* The DRNG context shared with the candidate code. */
#ifndef NGCC_NO_DRNG
DRNG_ctx drng_algorithm;
#else
extern DRNG_ctx drng_algorithm;
#endif

int ngcc_seed(const unsigned char *seed, unsigned long long seed_len_bytes)
{
    return init_random_number(&drng_algorithm, seed, seed_len_bytes);
}

int ngcc_random(unsigned char *out, unsigned long long out_len_bits)
{
    return get_random_number(&drng_algorithm, out, out_len_bits);
}

/* The complete flag string; ngcc_meta().build_flags holds only its first 127 bytes. */
const char *ngcc_build_flags(void) { return NGCC_FLAGS; }

static void ngcc_strcpy(char *dst, size_t n, const char *src)
{
    size_t l = strlen(src);
    if (l >= n) l = n - 1;
    memcpy(dst, src, l);
    dst[l] = '\0';
}

#if defined(NGCC_BUILD_KEM)
static ngcc_meta_kem_t meta;
#elif defined(NGCC_BUILD_SIG)
static ngcc_meta_sig_t meta;
#elif defined(NGCC_BUILD_KEX)
static ngcc_meta_kex_t meta;
#else
static ngcc_meta_hash_t meta;
#endif

const ngcc_meta_t *ngcc_meta(void)
{
    if (meta.h.magic == NGCC_META_MAGIC)
        return &meta.h;

    memset(&meta, 0, sizeof meta);
    meta.h.abi = NGCC_LINK_ABI;
    meta.h.struct_size = (uint32_t)sizeof meta;
    ngcc_strcpy(meta.h.id, sizeof meta.h.id, NGCC_ID);
    ngcc_strcpy(meta.h.algorithm, sizeof meta.h.algorithm, NGCC_ALG);
    ngcc_strcpy(meta.h.instance, sizeof meta.h.instance, NGCC_INSTANCE);
    ngcc_strcpy(meta.h.variant, sizeof meta.h.variant, NGCC_VARIANT);
    ngcc_strcpy(meta.h.source_dir, sizeof meta.h.source_dir, NGCC_SRCDIR);
    ngcc_strcpy(meta.h.compiler, sizeof meta.h.compiler, NGCC_COMPILER);
    ngcc_strcpy(meta.h.build_flags, sizeof meta.h.build_flags, NGCC_FLAGS);

#if defined(NGCC_BUILD_KEM)
    meta.h.type = NGCC_TYPE_KEM;
    meta.pk_len = kem_get_pk_len_bytes();
    meta.sk_len = kem_get_sk_len_bytes();
    meta.ct_len = kem_get_ct_len_bytes();
    meta.ss_len = kem_get_ss_len_bytes();
#elif defined(NGCC_BUILD_SIG)
    meta.h.type = NGCC_TYPE_SIG;
    meta.pk_len = sig_get_pk_len_bytes();
    meta.sk_len = sig_get_sk_len_bytes();
    meta.sn_len = sig_get_sn_len_bytes();
#elif defined(NGCC_BUILD_KEX)
    meta.h.type = NGCC_TYPE_KEX;
    meta.passes        = kex_get_passes_num();
    meta.pk_len        = kex_get_pk_len_bytes();
    meta.sk_len        = kex_get_sk_len_bytes();
    meta.sta_len       = kex_get_sta_len_bytes();
    meta.stb_len       = kex_get_stb_len_bytes();
    meta.ss_len        = kex_get_ss_len_bytes();
    meta.total_msg_len = kex_get_total_msg_len_bytes();
#else
    meta.h.type = NGCC_TYPE_HASH;
#  ifdef NGCC_DIGEST_BITS
    meta.digest_bits = NGCC_DIGEST_BITS;
#  elif defined(DIGEST_BIT_LENGTH)
    meta.digest_bits = DIGEST_BIT_LENGTH;
#  else
#    error "hash: NGCC_DIGEST_BITS not given and DIGEST_BIT_LENGTH not defined"
#  endif
    meta.digest_len = meta.digest_bits / 8;
#endif
    meta.h.magic = NGCC_META_MAGIC;
    return &meta.h;
}
