/*
 * ngcc_kat: master KAT harness for NGCC candidate libraries.
 *
 * Usage: ngcc_kat [options] lib<Instance>.so
 *   --out-dir DIR    write generated KAT_<KIND>_<instance>.txt files here
 *   --kat-dir DIR    directory with the submitted KAT files to compare against
 *   --kat-name NAME  name part of the KAT file if it differs from the instance
 *   --kat-sha FILE   manifest of canonical SHA-256 digests of the reference KATs
 *                    (lines: <hex64> <strict|nolen>[:records] <filename>); takes precedence
 *                    over --kat-dir so the big reference files need not exist
 *   --write-manifest FILE  do not run KATs; hash the reference files for this
 *                    library found in --kat-dir and add/replace their lines in FILE
 *   --label L        key manifest entries as L/<filename> (the make instance
 *                    label), so instances whose KAT files share a name but live
 *                    in different directories do not collide
 *   --count N        number of KAT records for kem/sig/kex (default 10)
 *   --loop           hash: also run the 1,000,000-iteration loop test (slow)
 *   --full           hash: run everything incl. the 2^33-bit test (needs 1 GiB)
 *   --timeout SEC    abort after SEC seconds (default 900)
 *   --mem-limit MB   RLIMIT_AS in MiB (default: none)
 *   --meta-only      print metadata and exit
 *   --quiet          only print the RESULT line
 *
 * The harness loads the library with dlopen(RTLD_NOW|RTLD_LOCAL), resolves
 * ngcc_meta()/ngcc_seed() and the official API entry points, prints the
 * metadata block, then reproduces the official KAT_KEM.c / KAT_SIG.c /
 * KAT_KEX.c / KAT_CryptHash.c drivers exactly (same seed DRNG, same message
 * schedule, same output format). Every output buffer carries guard bytes that
 * are checked after each call so a candidate that writes past its claimed
 * length is reported instead of silently corrupting the heap.
 *
 * Exit codes / RESULT line:
 *   0 PASS      generated KAT matches the submitted KAT
 *   1 MISMATCH  generated KAT differs from the submitted KAT
 *   2 NOKAT     no submitted KAT found; generation succeeded
 *   3 CRYPTOFAIL  an API call returned an error or a self-check failed
 *   4 OVERFLOW  a call wrote past its claimed buffer length
 *   5 LOADFAIL  library or symbol could not be loaded
 *   6 TIMEOUT / 7 CRASH (signal)
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <stddef.h>

#include "drng.h"
#include "link_kem.h"
#include "link_sig.h"
#include "link_kex.h"
#include "link_hash.h"

#define SEED_LEN_BYTES 64
#define GUARD 64
#define GUARD_BYTE 0xA5

enum { RC_PASS = 0, RC_MISMATCH = 1, RC_NOKAT = 2, RC_CRYPTOFAIL = 3,
       RC_OVERFLOW = 4, RC_LOADFAIL = 5, RC_TIMEOUT = 6, RC_CRASH = 7 };

static const char *g_id = "?", *g_instance = "?";
static int g_quiet = 0;
static int g_count = 10, g_count_given = 0;
static int g_full = 0, g_loop = 0, g_meta_only = 0;
static const char *g_out_dir = NULL, *g_kat_dir = NULL, *g_kat_name = NULL;
static int g_timeout = 900;
static long g_mem_limit_mb = 0;
static long g_blank_len = 0;   /* reference _Len lines that were blank */
static int g_prefix_records = 0; /* file compare covered only this many records */

static void result(int rc, const char *fmt, ...)
{
    static const char *names[] = { "PASS", "MISMATCH", "NOKAT", "CRYPTOFAIL",
                                   "OVERFLOW", "LOADFAIL", "TIMEOUT", "CRASH" };
    va_list ap;
    fflush(stdout);
    printf("RESULT %s %s %s", g_id, g_instance, names[rc]);
    if (fmt) { printf(" "); va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); }
    printf("\n");
    fflush(stdout);
    exit(rc);
}

static void on_signal(int sig)
{
    char buf[256];
    int n = snprintf(buf, sizeof buf, "RESULT %s %s %s signal=%d\n", g_id, g_instance,
                     sig == SIGALRM ? "TIMEOUT" : "CRASH", sig);
    if (write(STDOUT_FILENO, buf, (size_t)n) < 0) { /* ignore */ }
    _exit(sig == SIGALRM ? RC_TIMEOUT : RC_CRASH);
}

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* ---------------- guarded buffers ---------------- */
typedef struct { unsigned char *p; size_t len; const char *name; } gbuf_t;

static gbuf_t gbuf(size_t len, const char *name)
{
    gbuf_t g;
    unsigned char *raw = calloc(len + 2 * GUARD, 1);
    if (!raw) result(RC_CRYPTOFAIL, "out of memory allocating %zu bytes for %s", len, name);
    memset(raw, GUARD_BYTE, GUARD);
    memset(raw + GUARD + len, GUARD_BYTE, GUARD);
    g.p = raw + GUARD; g.len = len; g.name = name;
    return g;
}

static void gcheck(const gbuf_t *g, const char *where)
{
    for (size_t i = 0; i < GUARD; i++) {
        if (g->p[(ptrdiff_t)i - (ptrdiff_t)GUARD] != GUARD_BYTE)
            result(RC_OVERFLOW, "%s wrote before buffer %s (claimed %zu bytes)", where, g->name, g->len);
        if (g->p[g->len + i] != GUARD_BYTE)
            result(RC_OVERFLOW, "%s wrote past buffer %s (claimed %zu bytes)", where, g->name, g->len);
    }
}

static void gcheck_len(const gbuf_t *g, unsigned long long returned, const char *where)
{
    if (returned > g->len)
        result(RC_OVERFLOW, "%s returned length %llu > claimed %zu for %s", where, returned, g->len, g->name);
}

/* ---------------- SHA-256 (for the KAT manifest) ---------------- */
typedef struct { uint32_t h[8]; uint64_t len; unsigned char buf[64]; size_t n; } sha256_t;
static const uint32_t K256[64] = {
0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2 };
#define ROR(x,n) (((x) >> (n)) | ((x) << (32 - (n))))
static void sha256_block(sha256_t *c, const unsigned char *p)
{
    uint32_t w[64], a, b, cc, d, e, f, g, h;
    for (int i = 0; i < 16; i++) w[i] = (uint32_t)p[4*i] << 24 | (uint32_t)p[4*i+1] << 16 | (uint32_t)p[4*i+2] << 8 | p[4*i+3];
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = ROR(w[i-15],7) ^ ROR(w[i-15],18) ^ (w[i-15] >> 3);
        uint32_t s1 = ROR(w[i-2],17) ^ ROR(w[i-2],19) ^ (w[i-2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    a = c->h[0]; b = c->h[1]; cc = c->h[2]; d = c->h[3]; e = c->h[4]; f = c->h[5]; g = c->h[6]; h = c->h[7];
    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + (ROR(e,6) ^ ROR(e,11) ^ ROR(e,25)) + ((e & f) ^ (~e & g)) + K256[i] + w[i];
        uint32_t t2 = (ROR(a,2) ^ ROR(a,13) ^ ROR(a,22)) + ((a & b) ^ (a & cc) ^ (b & cc));
        h = g; g = f; f = e; e = d + t1; d = cc; cc = b; b = a; a = t1 + t2;
    }
    c->h[0] += a; c->h[1] += b; c->h[2] += cc; c->h[3] += d; c->h[4] += e; c->h[5] += f; c->h[6] += g; c->h[7] += h;
}
static void sha256_init(sha256_t *c)
{
    static const uint32_t iv[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    memcpy(c->h, iv, sizeof iv); c->len = 0; c->n = 0;
}
static void sha256_update(sha256_t *c, const void *data, size_t len)
{
    const unsigned char *p = data;
    c->len += len;
    while (len) {
        size_t k = 64 - c->n; if (k > len) k = len;
        memcpy(c->buf + c->n, p, k); c->n += k; p += k; len -= k;
        if (c->n == 64) { sha256_block(c, c->buf); c->n = 0; }
    }
}
static void sha256_final(sha256_t *c, char hex[65])
{
    unsigned char pad[72] = {0x80}; uint64_t bits = c->len * 8;
    size_t padlen = (c->n < 56) ? 56 - c->n : 120 - c->n;
    for (int i = 0; i < 8; i++) pad[padlen + i] = (unsigned char)(bits >> (56 - 8 * i));
    sha256_update(c, pad, padlen + 8);
    for (int i = 0; i < 8; i++) sprintf(hex + 8 * i, "%08x", c->h[i]);
}

/* Canonical form of a KAT text: CR and trailing blanks stripped, empty lines
 * dropped, lines joined by '\n'. Mode "nolen" also drops every "X_Len = ..."
 * line, for references that were generated with OUTPUT_BLANK_TEST_VECTORS=1. */
static int is_len_line(const char *l, size_t n)
{
    const char *eq = memchr(l, '=', n);
    if (!eq || eq - l < 5) return 0;
    size_t k = (size_t)(eq - l);
    while (k && l[k-1] == ' ') k--;
    return k >= 4 && !memcmp(l + k - 4, "_Len", 4);
}
/* max_records > 0 limits the digest to the first max_records "Count = " records */
static void canon_hash(const char *text, size_t len, int nolen, int max_records, char hex[65])
{
    sha256_t c; sha256_init(&c);
    const char *p = text, *end = text + len;
    int records = 0;
    while (p < end) {
        const char *l = p; while (p < end && *p != '\n') p++;
        size_t n = (size_t)(p - l); if (p < end) p++;
        while (n && (l[n-1] == '\r' || l[n-1] == ' ' || l[n-1] == '\t')) n--;
        if (!n) continue;
        if (n >= 8 && !memcmp(l, "Count = ", 8) && max_records > 0 && ++records > max_records) break;
        if (nolen && is_len_line(l, n)) continue;
        sha256_update(&c, l, n); sha256_update(&c, "\n", 1);
    }
    sha256_final(&c, hex);
}
static int has_blank_len(const char *text, size_t len)
{
    const char *p = text, *end = text + len;
    while (p < end) {
        const char *l = p; while (p < end && *p != '\n') p++;
        size_t n = (size_t)(p - l); if (p < end) p++;
        while (n && (l[n-1] == '\r' || l[n-1] == ' ')) n--;
        if (n && l[n-1] == '=' && is_len_line(l, n)) return 1;
    }
    return 0;
}
static char *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb"); if (!f) return NULL;
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return NULL; }
    char *buf = malloc((size_t)sz + 1);
    if (!buf || fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); return NULL; }
    buf[sz] = 0; fclose(f); *len = (size_t)sz; return buf;
}

/* manifest: <hex64> <strict|nolen> <filename> */
typedef struct { char hex[65]; int nolen; int records; char name[256]; } mline_t;
static const char *mode_str(int nolen, int records)
{
    static char b[32];
    if (records > 0) snprintf(b, sizeof b, "%s:%d", nolen ? "nolen" : "strict", records);
    else snprintf(b, sizeof b, "%s", nolen ? "nolen" : "strict");
    return b;
}
static mline_t *g_man = NULL; static size_t g_man_n = 0;
static const char *g_kat_sha = NULL, *g_write_manifest = NULL, *g_label = NULL;
static const char *manifest_key(const char *fname)
{
    static char key[512];
    if (g_label) snprintf(key, sizeof key, "%s/%s", g_label, fname); else snprintf(key, sizeof key, "%s", fname);
    return key;
}

static void load_manifest(const char *path)
{
    size_t len; char *t = read_file(path, &len);
    if (!t) return;
    char *p = t;
    while (*p) {
        char *l = p; while (*p && *p != '\n') p++; if (*p) *p++ = 0;
        mline_t m; char mode[16];
        if (sscanf(l, "%64s %15s %255[^\r\n]", m.hex, mode, m.name) == 3 && strlen(m.hex) == 64) {
            m.nolen = !strncmp(mode, "nolen", 5);
            m.records = strchr(mode, ':') ? atoi(strchr(mode, ':') + 1) : 0;
            g_man = realloc(g_man, (g_man_n + 1) * sizeof *g_man); g_man[g_man_n++] = m;
        }
    }
    free(t);
}
static const mline_t *find_manifest(const char *fname)
{
    const char *key = manifest_key(fname);
    for (size_t i = 0; i < g_man_n; i++) if (!strcmp(g_man[i].name, key)) return &g_man[i];
    if (g_label)   /* hand-written manifests may use the bare file name */
        for (size_t i = 0; i < g_man_n; i++) if (!strcmp(g_man[i].name, fname)) return &g_man[i];
    return NULL;
}
/* add or replace one line in the manifest file (kept sorted by name) */
static void write_manifest_line(const char *path, const char *hex, int nolen, int records, const char *fname)
{
    load_manifest(path);
    int found = 0;
    for (size_t i = 0; i < g_man_n; i++) if (!strcmp(g_man[i].name, fname)) {
        strcpy(g_man[i].hex, hex); g_man[i].nolen = nolen; g_man[i].records = records; found = 1; }
    if (!found) {
        mline_t m; strcpy(m.hex, hex); m.nolen = nolen; m.records = records; snprintf(m.name, sizeof m.name, "%s", fname);
        g_man = realloc(g_man, (g_man_n + 1) * sizeof *g_man); g_man[g_man_n++] = m;
    }
    for (size_t i = 1; i < g_man_n; i++)            /* insertion sort by name */
        for (size_t j = i; j && strcmp(g_man[j-1].name, g_man[j].name) > 0; j--) { mline_t t = g_man[j]; g_man[j] = g_man[j-1]; g_man[j-1] = t; }
    FILE *f = fopen(path, "wb");
    if (!f) result(RC_CRYPTOFAIL, "cannot write %s: %s", path, strerror(errno));
    for (size_t i = 0; i < g_man_n; i++) fprintf(f, "%s %s %s\n", g_man[i].hex, mode_str(g_man[i].nolen, g_man[i].records), g_man[i].name);
    fclose(f);
    free(g_man); g_man = NULL; g_man_n = 0;
}

/* ---------------- KAT text output ---------------- */
static FILE *g_kat = NULL;      /* generated KAT text (memstream) */
static char *g_kat_buf = NULL;
static size_t g_kat_len = 0;

static void kat_begin(void)
{
    g_kat = open_memstream(&g_kat_buf, &g_kat_len);
    if (!g_kat) result(RC_CRYPTOFAIL, "open_memstream failed");
}

static void kat_len(const char *ident, unsigned long long len)
{
    fprintf(g_kat, "%s%llu\n", ident, len);
}

static void kat_hex(const char *ident, const unsigned char *p, unsigned long long len)
{
    fprintf(g_kat, "%s", ident);
    for (unsigned long long i = 0; i < len; i++) fprintf(g_kat, "%02X", p[i]);
    fprintf(g_kat, "\n");
}

/* Finish the memstream, write it to out-dir/<fname>, compare against kat-dir/<fname>.
 * Returns RC_PASS / RC_MISMATCH / RC_NOKAT and describes the first mismatch. */
static int kat_finish(const char *fname, char *why, size_t whylen)
{
    fclose(g_kat); g_kat = NULL;
    if (g_out_dir) {
        char path[4096];
        snprintf(path, sizeof path, "%s/%s", g_out_dir, fname);
        FILE *f = fopen(path, "wb");
        if (!f) result(RC_CRYPTOFAIL, "cannot write %s: %s", path, strerror(errno));
        fwrite(g_kat_buf, 1, g_kat_len, f);
        fclose(f);
        if (!g_quiet) printf("wrote  %s (%zu bytes)\n", path, g_kat_len);
    }
    int rc = RC_NOKAT;
    why[0] = 0;
    const mline_t *ml = g_kat_sha ? find_manifest(fname) : NULL;
    if (ml) {
        char hex[65];
        canon_hash(g_kat_buf, g_kat_len, ml->nolen, 0, hex);
        if (!g_quiet) printf("manifest %s %s %s\n", ml->hex, mode_str(ml->nolen, ml->records), fname);
        if (ml->records && ml->records != g_count) {
            rc = RC_MISMATCH; snprintf(why, whylen, "%s: manifest covers %d records, this run has %d", fname, ml->records, g_count);
        } else if (!strcmp(hex, ml->hex)) { rc = RC_PASS; if (ml->nolen) g_blank_len++; }
        else { rc = RC_MISMATCH; snprintf(why, whylen, "%s: sha256 %s != manifest %s", fname, hex, ml->hex); }
    } else if (g_kat_dir) {
        char path[4096];
        snprintf(path, sizeof path, "%s/%s", g_kat_dir, fname);
        FILE *f = fopen(path, "rb");
        if (!f) {
            snprintf(why, whylen, "no reference file %s", path);
        } else {
            /* token-wise compare: split both texts into "Key = Value" lines,
               ignore blank lines and CR, report first differing line */
            char *ref = NULL; size_t rlen = 0;
            fseek(f, 0, SEEK_END); rlen = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
            ref = malloc(rlen + 1);
            if (fread(ref, 1, rlen, f) != rlen) rlen = 0;
            ref[rlen] = 0; fclose(f);
            if (!g_quiet) printf("compare %s (%zu bytes)\n", path, rlen);
            char *a = g_kat_buf, *b = ref;
            size_t line = 0, rec = 0;
            rc = RC_PASS;
            for (;;) {
                char *la, *lb; size_t na, nb;
                /* next non-empty line of a */
                for (;;) { la = a; while (*a && *a != '\n') a++; na = (size_t)(a - la); if (*a) a++;
                           while (na && (la[na-1] == '\r' || la[na-1] == ' ')) na--;
                           if (na || !*a) break; }
                for (;;) { lb = b; while (*b && *b != '\n') b++; nb = (size_t)(b - lb); if (*b) b++;
                           while (nb && (lb[nb-1] == '\r' || lb[nb-1] == ' ')) nb--;
                           if (nb || !*b) break; }
                if (na == 0 && nb == 0) break;
                /* shortened run (--count N): generated text ends where the
                   reference continues with record N */
                if (na == 0 && g_count_given && nb >= 8 && !memcmp(lb, "Count = ", 8) && atoi(lb + 8) == g_count) {
                    g_prefix_records = g_count; break; }
                line++;
                if (na >= 7 && !strncmp(la, "Count =", 7)) rec++;
                /* reference generated with OUTPUT_BLANK_TEST_VECTORS=1 leaves
                   "X_Len = " empty; tolerate that but count it */
                if (na > nb && nb >= 7 && lb[nb-1] == '=' && !memcmp(la, lb, nb) &&
                    memmem(lb, nb, "_Len =", 6)) { g_blank_len++; continue; }
                if (na != nb || memcmp(la, lb, na)) {
                    char sa[80], sb[80];
                    snprintf(sa, sizeof sa, "%.*s", (int)(na > 60 ? 60 : na), la);
                    snprintf(sb, sizeof sb, "%.*s", (int)(nb > 60 ? 60 : nb), lb);
                    snprintf(why, whylen, "%s line %zu (record %zu): got '%s%s' expected '%s%s'",
                             fname, line, rec, sa, na > 60 ? "..." : "", sb, nb > 60 ? "..." : "");
                    rc = RC_MISMATCH;
                    break;
                }
            }
            free(ref);
        }
    }
    free(g_kat_buf); g_kat_buf = NULL; g_kat_len = 0;
    return rc;
}

/* ---------------- library loading ---------------- */
static void *g_lib = NULL;
static int (*p_seed)(const unsigned char *, unsigned long long);

static void *need_sym(const char *name)
{
    void *p = dlsym(g_lib, name);
    if (!p) result(RC_LOADFAIL, "missing symbol %s", name);
    return p;
}

static void seed_alg(const unsigned char *seed)
{
    if (p_seed(seed, SEED_LEN_BYTES) != 0)
        result(RC_CRYPTOFAIL, "ngcc_seed failed");
}

static void print_meta(const ngcc_meta_t *m)
{
    printf("id          = %s\n", m->id);
    printf("type        = %s\n", ngcc_type_name(m->type));
    printf("algorithm   = %s\n", m->algorithm);
    printf("instance    = %s\n", m->instance);
    printf("variant     = %s\n", m->variant);
    printf("source_dir  = %s\n", m->source_dir);
    printf("compiler    = %s\n", m->compiler);
    printf("build_flags = %s\n", m->build_flags);
    switch (m->type) {
    case NGCC_TYPE_KEM: {
        const ngcc_meta_kem_t *k = (const ngcc_meta_kem_t *)m;
        printf("pk_len      = %llu\nsk_len      = %llu\nct_len      = %llu\nss_len      = %llu\n",
               (unsigned long long)k->pk_len, (unsigned long long)k->sk_len,
               (unsigned long long)k->ct_len, (unsigned long long)k->ss_len);
        break; }
    case NGCC_TYPE_SIG: {
        const ngcc_meta_sig_t *s = (const ngcc_meta_sig_t *)m;
        printf("pk_len      = %llu\nsk_len      = %llu\nsn_len      = %llu\n",
               (unsigned long long)s->pk_len, (unsigned long long)s->sk_len, (unsigned long long)s->sn_len);
        break; }
    case NGCC_TYPE_KEX: {
        const ngcc_meta_kex_t *x = (const ngcc_meta_kex_t *)m;
        printf("passes      = %llu\npk_len      = %llu\nsk_len      = %llu\nsta_len     = %llu\n"
               "stb_len     = %llu\nss_len      = %llu\ntotal_msg   = %llu\n",
               (unsigned long long)x->passes, (unsigned long long)x->pk_len, (unsigned long long)x->sk_len,
               (unsigned long long)x->sta_len, (unsigned long long)x->stb_len, (unsigned long long)x->ss_len,
               (unsigned long long)x->total_msg_len);
        break; }
    case NGCC_TYPE_HASH: {
        const ngcc_meta_hash_t *h = (const ngcc_meta_hash_t *)m;
        printf("digest_bits = %llu\ndigest_len  = %llu\n",
               (unsigned long long)h->digest_bits, (unsigned long long)h->digest_len);
        break; }
    }
    fflush(stdout);
}

/* the "seed" nonce used by all three PKC drivers */
static void init_seed_drng(DRNG_ctx *d)
{
    unsigned char nonce[SEED_LEN_BYTES];
    for (int i = 0; i < SEED_LEN_BYTES / 4; i++) memcpy(nonce + 4 * i, "seed", 4);
    init_random_number(d, nonce, SEED_LEN_BYTES);
}

/* ---------------- KEM ---------------- */
static int run_kem(const ngcc_meta_kem_t *m, char *why, size_t whylen)
{
    ngcc_kem_api_t api;
#define X(f, s) api.f = (void *)need_sym(s);
    NGCC_KEM_SYMBOLS
#undef X
    DRNG_ctx drng_seed; init_seed_drng(&drng_seed);
    unsigned char seed[SEED_LEN_BYTES];
    gbuf_t pk = gbuf(m->pk_len, "pk"), sk = gbuf(m->sk_len, "sk"), ct = gbuf(m->ct_len, "ct"),
           ss = gbuf(m->ss_len, "ss"), ss1 = gbuf(m->ss_len, "ss1");
    unsigned long long pk_len = m->pk_len, sk_len = m->sk_len, ct_len = m->ct_len, ss_len = m->ss_len;
    double t_kg = 0, t_enc = 0, t_dec = 0, t0;
    kat_begin();
    for (int i = 0; i < g_count; i++) {
        fprintf(g_kat, "Count = %d\n", i);
        get_random_number(&drng_seed, seed, SEED_LEN_BYTES * 8);
        fprintf(g_kat, "Seed_Len = %d\n", SEED_LEN_BYTES);
        kat_hex("Seed = ", seed, SEED_LEN_BYTES);
        seed_alg(seed);
        pk_len = m->pk_len; sk_len = m->sk_len; ct_len = m->ct_len; ss_len = m->ss_len;
        t0 = now_s();
        int r = api.keygen(pk.p, &pk_len, sk.p, &sk_len);
        t_kg += now_s() - t0;
        gcheck(&pk, "kem_keygen"); gcheck(&sk, "kem_keygen");
        if (r) { snprintf(why, whylen, "kem_keygen returned %d at count %d", r, i); return RC_CRYPTOFAIL; }
        gcheck_len(&pk, pk_len, "kem_keygen"); gcheck_len(&sk, sk_len, "kem_keygen");
        kat_len("PK_Len = ", pk_len); kat_hex("PK = ", pk.p, pk_len);
        kat_len("SK_Len = ", sk_len); kat_hex("SK = ", sk.p, sk_len);
        t0 = now_s();
        r = api.enc(pk.p, pk_len, ss.p, &ss_len, ct.p, &ct_len);
        t_enc += now_s() - t0;
        gcheck(&ss, "kem_enc"); gcheck(&ct, "kem_enc");
        if (r) { snprintf(why, whylen, "kem_enc returned %d at count %d", r, i); return RC_CRYPTOFAIL; }
        gcheck_len(&ss, ss_len, "kem_enc"); gcheck_len(&ct, ct_len, "kem_enc");
        kat_len("CT_Len = ", ct_len); kat_hex("CT = ", ct.p, ct_len);
        kat_len("SS_Len = ", ss_len); kat_hex("SS = ", ss.p, ss_len);
        unsigned long long ss1_len = ss_len;
        t0 = now_s();
        r = api.dec(sk.p, sk_len, ct.p, ct_len, ss1.p, &ss1_len);
        t_dec += now_s() - t0;
        gcheck(&ss1, "kem_dec");
        if (r) { snprintf(why, whylen, "kem_dec returned %d at count %d", r, i); return RC_CRYPTOFAIL; }
        if (ss1_len != ss_len || memcmp(ss.p, ss1.p, ss_len)) {
            snprintf(why, whylen, "decapsulated ss != encapsulated ss at count %d", i); return RC_CRYPTOFAIL; }
        fprintf(g_kat, "\n");
    }
    if (!g_quiet) printf("timing  keygen %.3f ms  enc %.3f ms  dec %.3f ms (avg over %d)\n",
                         1e3 * t_kg / g_count, 1e3 * t_enc / g_count, 1e3 * t_dec / g_count, g_count);
    char fname[256];
    snprintf(fname, sizeof fname, "KAT_KEM_%s.txt", g_kat_name);
    return kat_finish(fname, why, whylen);
}

/* ---------------- SIG ---------------- */
static int run_sig(const ngcc_meta_sig_t *m, char *why, size_t whylen)
{
    ngcc_sig_api_t api;
#define X(f, s) api.f = (void *)need_sym(s);
    NGCC_SIG_SYMBOLS
#undef X
    DRNG_ctx drng_seed, drng_msg;
    init_seed_drng(&drng_seed);
    unsigned char nonce2[SEED_LEN_BYTES] = {0};
    for (int i = 0; i < SEED_LEN_BYTES / 3; i++) memcpy(nonce2 + 3 * i, "msg", 3);
    memcpy(nonce2 + SEED_LEN_BYTES - 1, "m", 1);
    init_random_number(&drng_msg, nonce2, SEED_LEN_BYTES);
    unsigned char seed[SEED_LEN_BYTES];
    gbuf_t pk = gbuf(m->pk_len, "pk"), sk = gbuf(m->sk_len, "sk"), sn = gbuf(m->sn_len, "sn"),
           msg = gbuf(128 + 8 * (size_t)(g_count > 10 ? g_count : 10), "m");
    int m_len = 56;
    double t_kg = 0, t_sign = 0, t_ver = 0, t0;
    kat_begin();
    for (int i = 0; i < g_count; i++) {
        unsigned long long pk_len = m->pk_len, sk_len = m->sk_len, sn_len = m->sn_len;
        fprintf(g_kat, "Count = %d\n", i);
        get_random_number(&drng_seed, seed, SEED_LEN_BYTES * 8);
        get_random_number(&drng_msg, msg.p, (unsigned long long)m_len * 8);
        fprintf(g_kat, "Seed_Len = %d\n", SEED_LEN_BYTES);
        kat_hex("Seed = ", seed, SEED_LEN_BYTES);
        seed_alg(seed);
        t0 = now_s();
        int r = api.keygen(pk.p, &pk_len, sk.p, &sk_len);
        t_kg += now_s() - t0;
        gcheck(&pk, "sig_keygen"); gcheck(&sk, "sig_keygen");
        if (r) { snprintf(why, whylen, "sig_keygen returned %d at count %d", r, i); return RC_CRYPTOFAIL; }
        gcheck_len(&pk, pk_len, "sig_keygen"); gcheck_len(&sk, sk_len, "sig_keygen");
        kat_len("PK_Len = ", pk_len); kat_hex("PK = ", pk.p, pk_len);
        kat_len("SK_Len = ", sk_len); kat_hex("SK = ", sk.p, sk_len);
        fprintf(g_kat, "M_Len = %d\n", m_len);
        kat_hex("M = ", msg.p, (unsigned long long)m_len);
        t0 = now_s();
        r = api.sign(sk.p, sk_len, msg.p, (unsigned long long)m_len, sn.p, &sn_len);
        t_sign += now_s() - t0;
        gcheck(&sn, "sig_sign"); gcheck(&msg, "sig_sign");
        if (r) { snprintf(why, whylen, "sig_sign returned %d at count %d", r, i); return RC_CRYPTOFAIL; }
        gcheck_len(&sn, sn_len, "sig_sign");
        kat_len("Sn_Len = ", sn_len); kat_hex("Sn = ", sn.p, sn_len);
        t0 = now_s();
        r = api.verify(pk.p, pk_len, sn.p, sn_len, msg.p, (unsigned long long)m_len);
        t_ver += now_s() - t0;
        if (r) { snprintf(why, whylen, "sig_verify returned %d at count %d", r, i); return RC_CRYPTOFAIL; }
        /* extra self-check, not part of the KAT: a flipped message bit must not verify */
        if (m_len > 0) {
            msg.p[0] ^= 0x80;
            r = api.verify(pk.p, pk_len, sn.p, sn_len, msg.p, (unsigned long long)m_len);
            msg.p[0] ^= 0x80;
            if (r == 0) { snprintf(why, whylen, "sig_verify accepted a modified message at count %d", i); return RC_CRYPTOFAIL; }
        }
        fprintf(g_kat, "\n");
        m_len += 8;
    }
    if (!g_quiet) printf("timing  keygen %.3f ms  sign %.3f ms  verify %.3f ms (avg over %d)\n",
                         1e3 * t_kg / g_count, 1e3 * t_sign / g_count, 1e3 * t_ver / g_count, g_count);
    char fname[256];
    snprintf(fname, sizeof fname, "KAT_SIG_%s.txt", g_kat_name);
    return kat_finish(fname, why, whylen);
}

/* ---------------- KEX ---------------- */
/* Mirrors the official KAT_KEX.c: passes 1..3 are always attempted in order
 * until one returns 1 (finished); the official driver then stops after pass 3.
 * For protocols claiming more passes we continue with kex_generate_pass4_msg_b,
 * kex_generate_pass5_msg_a, ... (resolved by name) exactly as the driver's
 * comment describes. A 0-pass (non-interactive) protocol goes through the same
 * three calls with empty messages, as the official driver would. */
#define NGCC_KEX_MAX_PASSES 16

static int run_kex(const ngcc_meta_kex_t *m, char *why, size_t whylen)
{
    ngcc_kex_api_t api;
    memset(&api, 0, sizeof api);
#define X(f, s) api.f = (void *)need_sym(s);
    NGCC_KEX_SYMBOLS
#undef X
#define X(f, s) api.f = (void *)dlsym(g_lib, s);
    NGCC_KEX_OPTIONAL_SYMBOLS
#undef X
    ngcc_kex_passn_fn passfn[NGCC_KEX_MAX_PASSES + 1];
    memset(passfn, 0, sizeof passfn);
    passfn[2] = api.pass2_b; passfn[3] = api.pass3_a;
    for (int k = 4; k <= NGCC_KEX_MAX_PASSES; k++) {
        char name[64];
        snprintf(name, sizeof name, "kex_generate_pass%d_msg_%c", k, (k & 1) ? 'a' : 'b');
        passfn[k] = (ngcc_kex_passn_fn)dlsym(g_lib, name);
    }
    DRNG_ctx drng_seed; init_seed_drng(&drng_seed);
    unsigned char seed[SEED_LEN_BYTES];
    unsigned long long pass = m->passes;
    if (pass > NGCC_KEX_MAX_PASSES) { snprintf(why, whylen, "unsupported pass count %llu", pass); return RC_CRYPTOFAIL; }
    /* the official driver calls pass1..3 unconditionally (a compliant protocol
       returns 1 at its last pass, so later ones are never reached); beyond 3
       only what is claimed. Functions up to the claimed count must exist. */
    unsigned long long need = pass < 3 ? 3 : pass;
    for (unsigned long long k = 2; k <= pass; k++)
        if (!passfn[k]) { snprintf(why, whylen, "missing kex_generate_pass%llu_msg_%c", k, (k & 1) ? 'a' : 'b'); return RC_LOADFAIL; }

    gbuf_t pka = gbuf(m->pk_len, "pka"), ska = gbuf(m->sk_len, "ska"),
           pkb = gbuf(m->pk_len, "pkb"), skb = gbuf(m->sk_len, "skb"),
           sta = gbuf(m->sta_len, "sta"), stb = gbuf(m->stb_len, "stb"),
           ssa = gbuf(m->ss_len, "ssa"), ssb = gbuf(m->ss_len, "ssb");
    gbuf_t msg[NGCC_KEX_MAX_PASSES + 1];
    for (int k = 1; k <= NGCC_KEX_MAX_PASSES; k++) msg[k] = gbuf(k <= (int)need ? m->total_msg_len : 0, "m");
    double t_total = 0, t0;
    kat_begin();
    for (int i = 0; i < g_count; i++) {
        unsigned long long pka_len = m->pk_len, ska_len = m->sk_len, pkb_len = m->pk_len, skb_len = m->sk_len,
            sta_len = m->sta_len, stb_len = m->stb_len, ssa_len = m->ss_len, ssb_len = m->ss_len,
            mlen[NGCC_KEX_MAX_PASSES + 1], ma_len = 0, mb_len = 0;
        unsigned char *ma = NULL, *mb = NULL;
        int r;
        memset(mlen, 0, sizeof mlen);
        fprintf(g_kat, "Count = %d\n", i);
        get_random_number(&drng_seed, seed, SEED_LEN_BYTES * 8);
        fprintf(g_kat, "Seed_Len = %d\n", SEED_LEN_BYTES);
        kat_hex("Seed = ", seed, SEED_LEN_BYTES);
        seed_alg(seed);
        kat_len("Pass_Num = ", pass);
        t0 = now_s();
        r = api.init_a(pka.p, &pka_len, ska.p, &ska_len, sta.p, &sta_len);
        gcheck(&pka, "kex_init_a"); gcheck(&ska, "kex_init_a"); gcheck(&sta, "kex_init_a");
        if (r < 0) { snprintf(why, whylen, "kex_init_a returned %d at count %d", r, i); return RC_CRYPTOFAIL; }
        gcheck_len(&pka, pka_len, "kex_init_a"); gcheck_len(&ska, ska_len, "kex_init_a"); gcheck_len(&sta, sta_len, "kex_init_a");
        kat_len("PKa_Len = ", pka_len); kat_hex("PKa = ", pka.p, pka_len);
        kat_len("SKa_Len = ", ska_len); kat_hex("SKa = ", ska.p, ska_len);
        kat_len("Init_Sta_Len = ", sta_len); kat_hex("Init_Sta = ", sta.p, sta_len);
        r = api.init_b(pkb.p, &pkb_len, skb.p, &skb_len, stb.p, &stb_len);
        gcheck(&pkb, "kex_init_b"); gcheck(&skb, "kex_init_b"); gcheck(&stb, "kex_init_b");
        if (r < 0) { snprintf(why, whylen, "kex_init_b returned %d at count %d", r, i); return RC_CRYPTOFAIL; }
        gcheck_len(&pkb, pkb_len, "kex_init_b"); gcheck_len(&skb, skb_len, "kex_init_b"); gcheck_len(&stb, stb_len, "kex_init_b");
        kat_len("PKb_Len = ", pkb_len); kat_hex("PKb = ", pkb.p, pkb_len);
        kat_len("SKb_Len = ", skb_len); kat_hex("SKb = ", skb.p, skb_len);
        kat_len("Init_Stb_Len = ", stb_len); kat_hex("Init_Stb = ", stb.p, stb_len);

        for (unsigned long long k = 1; k <= need; k++) {
            int a_side = (k & 1);               /* odd passes: initiator */
            gbuf_t *st = a_side ? &sta : &stb;
            unsigned long long *st_len = a_side ? &sta_len : &stb_len;
            char label[32], tag[8];
            snprintf(tag, sizeof tag, "pass%llu", k);
            if (k > 1 && !passfn[k]) {   /* not claimed, not implemented: the driver would stop here too */
                if (pass >= k) { snprintf(why, whylen, "protocol did not finish by pass %llu", k - 1); return RC_CRYPTOFAIL; }
                break;
            }
            if (k == 1)
                r = api.pass1_a(ska.p, ska_len, pkb.p, pkb_len, sta.p, &sta_len, msg[1].p, &mlen[1]);
            else if (a_side)
                r = passfn[k](ska.p, ska_len, pkb.p, pkb_len, msg[k-1].p, mlen[k-1], sta.p, &sta_len, msg[k].p, &mlen[k]);
            else
                r = passfn[k](skb.p, skb_len, pka.p, pka_len, msg[k-1].p, mlen[k-1], stb.p, &stb_len, msg[k].p, &mlen[k]);
            gcheck(st, tag); gcheck(&msg[k], tag);
            if (r < 0) { snprintf(why, whylen, "kex_generate_pass%llu_msg_%c returned %d at count %d", k, a_side ? 'a' : 'b', r, i); return RC_CRYPTOFAIL; }
            gcheck_len(st, *st_len, tag); gcheck_len(&msg[k], mlen[k], tag);
            snprintf(label, sizeof label, "Pass%llu_St%c_Len = ", k, a_side ? 'a' : 'b');
            kat_len(label, *st_len);
            snprintf(label, sizeof label, "Pass%llu_St%c = ", k, a_side ? 'a' : 'b');
            kat_hex(label, st->p, *st_len);
            snprintf(label, sizeof label, "M%llu_Len = ", k); kat_len(label, mlen[k]);
            snprintf(label, sizeof label, "M%llu = ", k);     kat_hex(label, msg[k].p, mlen[k]);
            if (a_side) { ma = msg[k].p; ma_len = mlen[k]; } else { mb = msg[k].p; mb_len = mlen[k]; }
            if (r == 1) {
                if (pass != k) { snprintf(why, whylen, "pass%llu reported finished but Pass_Num=%llu", k, pass); return RC_CRYPTOFAIL; }
                break;
            }
        }
        r = api.derive_a(ska.p, ska_len, pkb.p, pkb_len, mb, mb_len, sta.p, sta_len, ssa.p, &ssa_len);
        gcheck(&ssa, "kex_derive_ss_a");
        if (r < 0) { snprintf(why, whylen, "kex_derive_ss_a returned %d at count %d", r, i); return RC_CRYPTOFAIL; }
        gcheck_len(&ssa, ssa_len, "derive_a");
        r = api.derive_b(skb.p, skb_len, pka.p, pka_len, ma, ma_len, stb.p, stb_len, ssb.p, &ssb_len);
        gcheck(&ssb, "kex_derive_ss_b");
        if (r < 0) { snprintf(why, whylen, "kex_derive_ss_b returned %d at count %d", r, i); return RC_CRYPTOFAIL; }
        gcheck_len(&ssb, ssb_len, "derive_b");
        t_total += now_s() - t0;
        if (ssa_len != ssb_len || memcmp(ssa.p, ssb.p, ssa_len)) {
            snprintf(why, whylen, "initiator ss != responder ss at count %d", i); return RC_CRYPTOFAIL; }
        kat_len("SS_Len = ", ssa_len); kat_hex("SS = ", ssa.p, ssa_len);
        fprintf(g_kat, "\n");
    }
    if (!g_quiet) printf("timing  full exchange %.3f ms (avg over %d)\n", 1e3 * t_total / g_count, g_count);
    char fname[256];
    snprintf(fname, sizeof fname, "KAT_KEX_%s.txt", g_kat_name);
    return kat_finish(fname, why, whylen);
}

/* ---------------- HASH ---------------- */
#define HIGH_N_BIT_MASK(N) ((unsigned char)((~0U) << (8 - (N))))

static void hash_msg_full(const unsigned char *msg, unsigned long long bits)
{
    fprintf(g_kat, "Msg_Len = %llu\n", bits);
    fprintf(g_kat, "Msg = ");
    for (unsigned long long i = 0; i < bits / 8; i++) fprintf(g_kat, "%02X", msg[i]);
    if (bits % 8) fprintf(g_kat, "%02X", HIGH_N_BIT_MASK(bits % 8) & msg[bits / 8]);
    fprintf(g_kat, "\n");
}

static void hash_msg_partial(const unsigned char *msg, unsigned long long bits, const unsigned char *seed)
{
    fprintf(g_kat, "Msg_Len = %llu\n", bits);
    fprintf(g_kat, "Msg_Seed = ");
    if (seed) for (int i = 0; i < SEED_LEN_BYTES; i++) fprintf(g_kat, "%02X", seed[i]);
    fprintf(g_kat, "\n");
    fprintf(g_kat, "Msg_Exp = ");
    for (int i = 0; i < 8; i++) fprintf(g_kat, "%02X", msg[i]);
    fprintf(g_kat, " .... ");
    for (int i = 0; i < 8; i++) fprintf(g_kat, "%02X", msg[bits / 8 - 8 + i]);
    fprintf(g_kat, "\n");
}

static void hash_digest(const unsigned char *d, int bits)
{
    fprintf(g_kat, "Dst_Len = %d\n", bits);
    fprintf(g_kat, "Dst = ");
    for (int i = 0; i < bits / 8; i++) fprintf(g_kat, "%02X", d[i]);
    fprintf(g_kat, "\n");
}

static void hash_seed_from_name(unsigned char seed[SEED_LEN_BYTES], const char *fname)
{
    for (int i = 0; i < SEED_LEN_BYTES / 8; i++) memcpy(seed + 8 * i, fname, 8);
}

static int worst(int a, int b) { return a > b ? a : b; }

static int run_hash(const ngcc_meta_hash_t *m, char *why, size_t whylen)
{
    ngcc_hash_api_t api;
#define X(f, s) api.f = (void *)need_sym(s);
    NGCC_HASH_SYMBOLS
#undef X
    const int dbits = (int)m->digest_bits;
    char fname[256], w[512];
    int rc = RC_PASS, r;
    unsigned char seed[SEED_LEN_BYTES];
    DRNG_ctx d;
    gbuf_t dig = gbuf((size_t)dbits / 8, "digest");
    double t0;

    /* KAT_2_12: every length 0..4096 bits */
    snprintf(fname, sizeof fname, "KAT_2_12_%s.txt", g_kat_name);
    hash_seed_from_name(seed, fname);
    {
        gbuf_t msg = gbuf(4096 / 8, "msg");
        init_random_number(&d, seed, SEED_LEN_BYTES);
        kat_begin();
        t0 = now_s();
        for (unsigned long long bits = 0; bits <= 4096; bits++) {
            memset(msg.p, 0, 4096 / 8);
            get_random_number(&d, msg.p, bits);
            hash_msg_full(msg.p, bits);
            r = api.hash(dbits, msg.p, bits, dig.p);
            gcheck(&dig, "CryptHash"); gcheck(&msg, "CryptHash");
            if (r) { snprintf(why, whylen, "CryptHash returned %d for msg_len=%llu bits", r, bits); return RC_CRYPTOFAIL; }
            hash_digest(dig.p, dbits);
            fprintf(g_kat, "\n");
        }
        if (!g_quiet) printf("timing  KAT_2_12 %.3f s\n", now_s() - t0);
        r = kat_finish(fname, w, sizeof w);
        if (r != RC_PASS && !why[0]) snprintf(why, whylen, "%s", w);
        rc = worst(rc, r);
    }
    /* KAT_2_23 and (optionally) KAT_2_33: all-0, all-1, random */
    for (int big = 0; big < 2; big++) {
        unsigned long long bits = big ? (1ULL << 33) : (1ULL << 23);
        if (big && !g_full) break;
        snprintf(fname, sizeof fname, big ? "KAT_2_33_%s.txt" : "KAT_2_23_%s.txt", g_kat_name);
        hash_seed_from_name(seed, fname);
        gbuf_t msg = gbuf((size_t)(bits / 8), "msg");
        init_random_number(&d, seed, SEED_LEN_BYTES);
        kat_begin();
        t0 = now_s();
        for (int k = 0; k < 3; k++) {
            if (k == 0) memset(msg.p, 0, (size_t)(bits / 8));
            else if (k == 1) memset(msg.p, 0xFF, (size_t)(bits / 8));
            else { memset(msg.p, 0, (size_t)((bits + 7) / 8)); get_random_number(&d, msg.p, bits); }
            hash_msg_partial(msg.p, bits, k == 2 ? seed : NULL);
            r = api.hash(dbits, msg.p, bits, dig.p);
            gcheck(&dig, "CryptHash"); gcheck(&msg, "CryptHash");
            if (r) { snprintf(why, whylen, "CryptHash returned %d for 2^%d-bit msg", r, big ? 33 : 23); return RC_CRYPTOFAIL; }
            hash_digest(dig.p, dbits);
            fprintf(g_kat, "\n");
        }
        if (!g_quiet) printf("timing  %s %.3f s\n", big ? "KAT_2_33" : "KAT_2_23", now_s() - t0);
        free(msg.p - GUARD);
        r = kat_finish(fname, w, sizeof w);
        if (r != RC_PASS && !why[0]) snprintf(why, whylen, "%s", w);
        rc = worst(rc, r);
    }
    /* KAT_Loop: 2^13-bit message, 1,000,000 iterations */
    if (g_loop || g_full) {
        const unsigned long long bits = 8192;
        snprintf(fname, sizeof fname, "KAT_Loop_%s.txt", g_kat_name);
        hash_seed_from_name(seed, fname);
        gbuf_t msg = gbuf(bits / 8, "msg"), buf = gbuf((size_t)dbits / 8, "buf");
        init_random_number(&d, seed, SEED_LEN_BYTES);
        get_random_number(&d, msg.p, bits);
        kat_begin();
        hash_msg_full(msg.p, bits);
        t0 = now_s();
        r = api.hash(dbits, msg.p, bits, dig.p);
        gcheck(&dig, "CryptHash");
        if (r) { snprintf(why, whylen, "CryptHash returned %d in loop test", r); return RC_CRYPTOFAIL; }
        const size_t db = (size_t)dbits / 8, mb = (size_t)bits / 8;
        for (int i = 0; i < 1000000; i++) {
            memcpy(buf.p, msg.p, db);
            memmove(msg.p, msg.p + db, mb - db);
            memcpy(msg.p + mb - db, buf.p, db);
            for (size_t j = 0; j < db; j++) msg.p[j] ^= dig.p[j];
            api.hash(dbits, msg.p, bits, dig.p);
        }
        gcheck(&dig, "CryptHash"); gcheck(&msg, "CryptHash");
        if (!g_quiet) printf("timing  KAT_Loop %.3f s (1e6 x %llu-bit)\n", now_s() - t0, bits);
        hash_digest(dig.p, dbits);
        fprintf(g_kat, "\n");
        r = kat_finish(fname, w, sizeof w);
        if (r != RC_PASS && !why[0]) snprintf(why, whylen, "%s", w);
        rc = worst(rc, r);
    }
    return rc;
}

/* ---------------- main ---------------- */
static void usage(void)
{
    fprintf(stderr, "usage: ngcc_kat [--out-dir D] [--kat-dir D] [--kat-sha F] [--write-manifest F]\n"
                    "                [--kat-name N] [--count N] [--full]\n"
                    "                [--loop] [--timeout S] [--mem-limit MB] [--meta-only] [--quiet] lib.so\n");
    exit(RC_LOADFAIL);
}

int main(int argc, char **argv)
{
    const char *libpath = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--out-dir") && i + 1 < argc) g_out_dir = argv[++i];
        else if (!strcmp(argv[i], "--kat-dir") && i + 1 < argc) g_kat_dir = argv[++i];
        else if (!strcmp(argv[i], "--kat-name") && i + 1 < argc) g_kat_name = argv[++i];
        else if (!strcmp(argv[i], "--kat-sha") && i + 1 < argc) g_kat_sha = argv[++i];
        else if (!strcmp(argv[i], "--label") && i + 1 < argc) g_label = argv[++i];
        else if (!strcmp(argv[i], "--write-manifest") && i + 1 < argc) g_write_manifest = argv[++i];
        else if (!strcmp(argv[i], "--count") && i + 1 < argc) { g_count = atoi(argv[++i]); g_count_given = 1; }
        else if (!strcmp(argv[i], "--timeout") && i + 1 < argc) g_timeout = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--mem-limit") && i + 1 < argc) g_mem_limit_mb = atol(argv[++i]);
        else if (!strcmp(argv[i], "--full")) g_full = 1;
        else if (!strcmp(argv[i], "--loop")) g_loop = 1;
        else if (!strcmp(argv[i], "--meta-only")) g_meta_only = 1;
        else if (!strcmp(argv[i], "--quiet")) g_quiet = 1;
        else if (argv[i][0] == '-') usage();
        else libpath = argv[i];
    }
    if (!libpath) usage();
    if (g_kat_dir && !*g_kat_dir) g_kat_dir = NULL;

    /* crash / timeout reporting */
    struct sigaction sa; memset(&sa, 0, sizeof sa); sa.sa_handler = on_signal;
    sigaction(SIGSEGV, &sa, NULL); sigaction(SIGBUS, &sa, NULL); sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL); sigaction(SIGABRT, &sa, NULL); sigaction(SIGALRM, &sa, NULL);
    if (g_timeout > 0) alarm((unsigned)g_timeout);
    if (g_mem_limit_mb > 0) {
        struct rlimit rl; rl.rlim_cur = rl.rlim_max = (rlim_t)g_mem_limit_mb << 20;
        setrlimit(RLIMIT_AS, &rl);
    }
    if (g_out_dir) mkdir(g_out_dir, 0755);

    /* prefer an absolute/relative path so dlopen does not search LD_LIBRARY_PATH */
    char pathbuf[4096];
    if (!strchr(libpath, '/')) { snprintf(pathbuf, sizeof pathbuf, "./%s", libpath); libpath = pathbuf; }
    g_lib = dlopen(libpath, RTLD_NOW | RTLD_LOCAL);
    if (!g_lib) { printf("dlopen: %s\n", dlerror()); result(RC_LOADFAIL, "dlopen failed"); }

    const ngcc_meta_t *(*p_meta)(void) = (void *)need_sym(NGCC_SYM_META);
    p_seed = (void *)need_sym(NGCC_SYM_SEED);
    const ngcc_meta_t *meta = p_meta();
    if (!meta || meta->magic != NGCC_META_MAGIC) result(RC_LOADFAIL, "bad metadata magic");
    if (meta->abi != NGCC_LINK_ABI) result(RC_LOADFAIL, "ABI %u, harness expects %u", meta->abi, NGCC_LINK_ABI);
    g_id = meta->id; g_instance = meta->instance;
    if (!g_kat_name) g_kat_name = meta->instance;
    if (!g_quiet) { printf("library     = %s\n", libpath); print_meta(meta); }
    if (g_meta_only) result(RC_PASS, "meta-only");
    if (g_kat_sha) load_manifest(g_kat_sha);
    if (g_write_manifest) {
        /* hash the reference files this library would be compared against */
        const char *kinds[4]; int nk = 0;
        char names[4][256];
        if (!g_kat_dir) result(RC_LOADFAIL, "--write-manifest needs --kat-dir");
        switch (meta->type) {
        case NGCC_TYPE_KEM:  kinds[nk++] = "KAT_KEM_%s.txt"; break;
        case NGCC_TYPE_SIG:  kinds[nk++] = "KAT_SIG_%s.txt"; break;
        case NGCC_TYPE_KEX:  kinds[nk++] = "KAT_KEX_%s.txt"; break;
        default: kinds[nk++] = "KAT_2_12_%s.txt"; kinds[nk++] = "KAT_2_23_%s.txt";
                 kinds[nk++] = "KAT_2_33_%s.txt"; kinds[nk++] = "KAT_Loop_%s.txt"; break;
        }
        int done = 0;
        for (int k = 0; k < nk; k++) {
            char path[4096]; size_t len; char *t;
            snprintf(names[k], sizeof names[k], kinds[k], g_kat_name);
            snprintf(path, sizeof path, "%s/%s", g_kat_dir, names[k]);
            if (!(t = read_file(path, &len))) continue;
            int nolen = has_blank_len(t, len);
            int records = (meta->type != NGCC_TYPE_HASH && g_count_given) ? g_count : 0;
            char hex[65]; canon_hash(t, len, nolen, records, hex); free(t);
            write_manifest_line(g_write_manifest, hex, nolen, records, manifest_key(names[k]));
            if (!g_quiet) printf("manifest %s %s %s (%zu bytes)\n", hex, mode_str(nolen, records), names[k], len);
            done++;
        }
        if (!done) result(RC_NOKAT, "no reference KAT files for %s in %s", g_kat_name, g_kat_dir);
        result(RC_PASS, "manifest: %d file(s) hashed into %s", done, g_write_manifest);
    }

    char why[1024] = "";
    int rc;
    switch (meta->type) {
    case NGCC_TYPE_KEM:  rc = run_kem((const ngcc_meta_kem_t *)meta, why, sizeof why); break;
    case NGCC_TYPE_SIG:  rc = run_sig((const ngcc_meta_sig_t *)meta, why, sizeof why); break;
    case NGCC_TYPE_KEX:  rc = run_kex((const ngcc_meta_kex_t *)meta, why, sizeof why); break;
    case NGCC_TYPE_HASH: rc = run_hash((const ngcc_meta_hash_t *)meta, why, sizeof why); break;
    default: result(RC_LOADFAIL, "unknown type %u", meta->type);
    }
    if (rc == RC_PASS && g_prefix_records)
        result(rc, "(first %d records only)%s", g_prefix_records, g_blank_len ? " (blank _Len lines in reference)" : "");
    if (rc == RC_PASS && g_blank_len)
        result(rc, "(%ld blank _Len lines in reference: generated in template mode)", g_blank_len);
    result(rc, "%s", why);
    return rc;
}
