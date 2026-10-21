/*
 * NGCC uniform link interface: common metadata block.
 *
 * Every candidate is built into a shared library (lib<Instance>.so) that
 * exports, in addition to the official ICCS API functions for its type:
 *
 *   const ngcc_meta_t *ngcc_meta(void);
 *       Returns the metadata block. The block starts with ngcc_meta_common_t
 *       and, depending on `type`, is really an ngcc_meta_kem_t,
 *       ngcc_meta_sig_t, ngcc_meta_kex_t or ngcc_meta_hash_t (see
 *       link_kem.h, link_sig.h, link_kex.h, link_hash.h). Size fields are
 *       filled on first call by querying the candidate's *_get_*_len_bytes()
 *       functions, so they reflect what the implementation claims.
 *
 *   int ngcc_seed(const unsigned char *seed, unsigned long long seed_len_bytes);
 *       (Re)initialises the library's internal `drng_algorithm` DRNG context,
 *       which the candidate code uses for all randomness. This is the same
 *       call the official KAT_*.c drivers make before each test.
 *
 *   int ngcc_random(unsigned char *out, unsigned long long out_len_bits);
 *       Draws from the internal DRNG (debug aid, mirrors get_random_number).
 *
 * The library is linked with api/link_exports.map so only these symbols, the
 * official API functions and the DRNG entry points are visible.
 */
#ifndef NGCC_LINK_COMMON_H
#define NGCC_LINK_COMMON_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NGCC_META_MAGIC 0x4347474EU /* "NGGC" little-endian: 'N','G','G','C' */
#define NGCC_LINK_ABI   1

typedef enum {
    NGCC_TYPE_UNKNOWN = 0,
    NGCC_TYPE_KEM  = 1, /* key encapsulation mechanism  (link_kem.h)  */
    NGCC_TYPE_SIG  = 2, /* digital signature            (link_sig.h)  */
    NGCC_TYPE_KEX  = 3, /* key exchange protocol        (link_kex.h)  */
    NGCC_TYPE_HASH = 4  /* cryptographic hash           (link_hash.h) */
} ngcc_type_t;

/* Common header of every metadata block. All strings are NUL-terminated. */
typedef struct ngcc_meta_common {
    uint32_t magic;        /* NGCC_META_MAGIC                              */
    uint32_t abi;          /* NGCC_LINK_ABI                                */
    uint32_t type;         /* ngcc_type_t                                  */
    uint32_t struct_size;  /* sizeof the full type-specific struct         */
    char     id[16];       /* candidate folder id, e.g. "kex-07"           */
    char     algorithm[64];/* algorithm name as listed on niccs.org.cn     */
    char     instance[64]; /* ALGORITHM_INSTANCE from the candidate header */
    char     variant[32];  /* e.g. "Reference_Implementation"              */
    char     source_dir[256]; /* instance source dir, relative to the candidate folder */
    char     compiler[64]; /* compiler and version string                  */
    char     build_flags[128]; /* optimisation / feature flags used        */
} ngcc_meta_common_t;

typedef ngcc_meta_common_t ngcc_meta_t;

/* Symbols exported by every candidate library. */
const ngcc_meta_t *ngcc_meta(void);
int ngcc_seed(const unsigned char *seed, unsigned long long seed_len_bytes);
int ngcc_random(unsigned char *out, unsigned long long out_len_bits);

/* Function-pointer names used by the harness when resolving with dlsym(). */
#define NGCC_SYM_META   "ngcc_meta"
#define NGCC_SYM_SEED   "ngcc_seed"
#define NGCC_SYM_RANDOM "ngcc_random"

static inline const char *ngcc_type_name(uint32_t t)
{
    switch (t) {
    case NGCC_TYPE_KEM:  return "kem";
    case NGCC_TYPE_SIG:  return "sig";
    case NGCC_TYPE_KEX:  return "kex";
    case NGCC_TYPE_HASH: return "hash";
    default:             return "unknown";
    }
}

#ifdef __cplusplus
}
#endif
#endif /* NGCC_LINK_COMMON_H */
