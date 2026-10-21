#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scheme_api.h"
#include "seeded_keygen.h"
#include "kem.h"
#include "m2e.h"
#include "rng.h"

void FIPS202_SHAKE256(const unsigned char *input, unsigned int inputByteLen,
                      unsigned char *output, int outputByteLen);

#define REPORT_TEX CRYPTO_ALGNAME "_trace_report.tex"

static int get_bit(const unsigned char *in, size_t bit)
{
    return (in[bit / 8U] >> (bit % 8U)) & 1U;
}

static void set_bit(unsigned char *out, size_t bit)
{
    out[bit / 8U] |= (unsigned char)(1U << (bit % 8U));
}

static void copy_bits(unsigned char *out, const unsigned char *in,
                      size_t bit_offset, size_t bit_len)
{
    size_t i;

    memset(out, 0, BITS_TO_BYTES(bit_len));
    for (i = 0; i < bit_len; i++) {
        if (get_bit(in, bit_offset + i)) {
            set_bit(out, i);
        }
    }
}

static void error_to_vector(unsigned char *e, const int *error)
{
    int i;

    memset(e, 0, BITS_TO_BYTES(LENGTH));
    for (i = 0; i < ERROR_WEIGHT; i++) {
        e[error[i] / 8] ^= (unsigned char)(1U << (error[i] % 8));
    }
}

static int hash_session_key(unsigned char *ss, unsigned char b,
                            const unsigned char *e,
                            const unsigned char *ct)
{
    size_t e_bytes = BITS_TO_BYTES(LENGTH);
    size_t in_len = 1U + e_bytes + CIPHERTEXT_BYTES;
    unsigned char *in = malloc(in_len);

    if (in == NULL || in_len > (size_t)4294967295U) {
        free(in);
        return FAIL;
    }

    in[0] = b;
    memcpy(in + 1, e, e_bytes);
    memcpy(in + 1 + e_bytes, ct, CIPHERTEXT_BYTES);
    FIPS202_SHAKE256(in, (unsigned int)in_len, ss, CRYPTO_BYTES);
    free(in);
    return SUCCESS;
}

static void fprint_hex(FILE *fp, const unsigned char *buf, size_t len,
                       size_t bytes_per_line)
{
    size_t i;

    for (i = 0; i < len; i++) {
        fprintf(fp, "%02X", buf[i]);
        if ((i + 1U) % bytes_per_line == 0U) {
            fprintf(fp, "\n");
        }
    }
    if (len == 0 || len % bytes_per_line != 0U) {
        fprintf(fp, "\n");
    }
}

static void tex_print_escaped(FILE *fp, const char *s)
{
    while (*s != '\0') {
        switch (*s) {
        case '\\':
            fprintf(fp, "\\textbackslash{}");
            break;
        case '{':
            fprintf(fp, "\\{");
            break;
        case '}':
            fprintf(fp, "\\}");
            break;
        case '_':
            fprintf(fp, "\\_");
            break;
        case '&':
            fprintf(fp, "\\&");
            break;
        case '%':
            fprintf(fp, "\\%%");
            break;
        case '#':
            fprintf(fp, "\\#");
            break;
        case '$':
            fprintf(fp, "\\$");
            break;
        case '^':
            fprintf(fp, "\\^{}");
            break;
        case '~':
            fprintf(fp, "\\~{}");
            break;
        default:
            fputc(*s, fp);
            break;
        }
        s++;
    }
}

static void tex_hex_block(FILE *fp, const char *title,
                          const unsigned char *buf, size_t len)
{
    fprintf(fp, "\\subsubsection*{%s (%zu bytes)}\n", title, len);
    fprintf(fp, "\\begin{Verbatim}[fontsize=\\tiny]\n");
    fprint_hex(fp, buf, len, 32U);
    fprintf(fp, "\\end{Verbatim}\n\n");
}

static void tex_gf_array(FILE *fp, const char *title,
                         const gfelt_t *a, size_t len)
{
    size_t i;

    fprintf(fp, "\\subsubsection*{%s (%zu field elements)}\n", title, len);
    fprintf(fp, "\\begin{Verbatim}[fontsize=\\tiny]\n");
    for (i = 0; i < len; i++) {
        fprintf(fp, "%04zu: 0x%08X\n", i, (unsigned)a[i]);
    }
    fprintf(fp, "\\end{Verbatim}\n\n");
}

static void tex_int_array(FILE *fp, const char *title, const int *a, size_t len)
{
    size_t i;

    fprintf(fp, "\\subsubsection*{%s (%zu integers)}\n", title, len);
    fprintf(fp, "\\begin{Verbatim}[fontsize=\\tiny]\n");
    for (i = 0; i < len; i++) {
        fprintf(fp, "%04zu: %d\n", i, a[i]);
    }
    fprintf(fp, "\\end{Verbatim}\n\n");
}

static void tex_public_matrix(FILE *fp, const unsigned char *pk)
{
    int j;
    size_t col_bytes = BITS_TO_BYTES(CODIMENSION);

    fprintf(fp, "\\subsubsection*{Public matrix T, column-major (%d columns)}\n",
            DIMENSION);
    fprintf(fp, "\\begin{Verbatim}[fontsize=\\tiny]\n");
    for (j = 0; j < DIMENSION; j++) {
        fprintf(fp, "col %04d: ", j);
        fprint_hex(fp, pk + (size_t)j * col_bytes, col_bytes, 32U);
    }
    fprintf(fp, "\\end{Verbatim}\n\n");
}

static void tex_raw_twisted_matrix(FILE *fp, goppa_t gamma)
{
    int i;
    int j;
    int k;
    int row;
    size_t row_bytes = BITS_TO_BYTES(LENGTH);
    unsigned char *H;
    poly_t col;

    H = calloc((size_t)CODIMENSION * row_bytes, 1);
    col = poly_alloc(GOPPA_DEGREE - 1);
    if (H == NULL || col == NULL) {
        free(H);
        if (col != NULL) {
            poly_free(col);
        }
        fprintf(fp, "\\subsubsection*{Raw twisted syndrome matrix}\n");
        fprintf(fp, "Allocation failed while building the report matrix.\n\n");
        return;
    }

    for (i = 0; i < LENGTH; i++) {
        poly_syndrome_twisted(col, gamma->L + i, gamma->g, gamma->eta);
        for (j = 0; j < GOPPA_DEGREE; j++) {
            gfindex_t x = gf_to_index(poly_coeff(col, j));
            for (k = 0; k < EXT_DEGREE; k++) {
                if ((x & (gfindex_t)(1U << k)) != 0) {
                    row = j * EXT_DEGREE + k;
                    H[(size_t)row * row_bytes + (size_t)i / 8U] ^=
                        (unsigned char)(1U << (i % 8));
                }
            }
        }
    }
    poly_free(col);

    fprintf(fp, "\\subsubsection*{Raw twisted syndrome matrix before systematization (%d x %d bits)}\n",
            CODIMENSION, LENGTH);
    fprintf(fp, "\\begin{Verbatim}[fontsize=\\tiny]\n");
    for (row = 0; row < CODIMENSION; row++) {
        fprintf(fp, "row %04d: ", row);
        fprint_hex(fp, H + (size_t)row * row_bytes, row_bytes, 32U);
    }
    fprintf(fp, "\\end{Verbatim}\n\n");
    free(H);
}

static void tex_begin(FILE *fp)
{
    fprintf(fp, "\\documentclass[10pt,a4paper]{article}\n");
    fprintf(fp, "\\usepackage[margin=16mm]{geometry}\n");
    fprintf(fp, "\\usepackage{fontspec}\n");
    fprintf(fp, "\\usepackage{xeCJK}\n");
    fprintf(fp, "\\usepackage{amsmath,amssymb}\n");
    fprintf(fp, "\\usepackage{booktabs,longtable}\n");
    fprintf(fp, "\\usepackage{fancyvrb}\n");
    fprintf(fp, "\\IfFontExistsTF{Times New Roman}{\\setmainfont{Times New Roman}}{\\setmainfont{TeX Gyre Termes}}\n");
    fprintf(fp, "\\IfFontExistsTF{Songti SC}{\\setCJKmainfont{Songti SC}}{\\setCJKmainfont{STSong}}\n");
    fprintf(fp, "\\IfFontExistsTF{Menlo}{\\setmonofont{Menlo}}{\\setmonofont{Latin Modern Mono}}\n");
    fprintf(fp, "\\IfFontExistsTF{PingFang SC}{\\setCJKmonofont{PingFang SC}}{\\setCJKmonofont{STSong}}\n");
    fprintf(fp, "\\setlength{\\parindent}{0pt}\n");
    fprintf(fp, "\\setlength{\\parskip}{0.35em}\n");
    fprintf(fp, "\\begin{document}\n");
    fprintf(fp, "\\begin{center}\n");
    fprintf(fp, "{\\Large\\bfseries ");
    tex_print_escaped(fp, CRYPTO_ALGNAME);
    fprintf(fp, " Trace Report}\\\\[0.4em]\n");
    fprintf(fp, "{\\normalsize QC-MC0425 FixedAB / SeededKeyGen / Encap / Decap full successful-path dump}\n");
    fprintf(fp, "\\end{center}\n\n");
}

static void tex_params(FILE *fp, const scheme_keygen_stats_t *stats,
                       int fw_rejects, int dec_status, int ss_match)
{
    fprintf(fp, "\\section*{Run Summary}\n");
    fprintf(fp, "\\begin{longtable}{ll}\\toprule\n");
    fprintf(fp, "Item & Value\\\\\\midrule\n");
    fprintf(fp, "Algorithm & ");
    tex_print_escaped(fp, CRYPTO_ALGNAME);
    fprintf(fp, "\\\\\n");
    fprintf(fp, "$(n,m,\\ell,r,w,t_0)$ & $(%d,%d,%d,%d,%d,%d)$\\\\\n",
            LENGTH, EXT_DEGREE, ORDER, GOPPA_DEGREE, ERROR_WEIGHT, PARAM_T0);
    fprintf(fp, "$\\sigma_1,\\sigma_2,\\rho$ & $(%d,%d,%d)$\\\\\n",
            PARAM_SIGMA1, PARAM_SIGMA2, PARAM_RHO);
    fprintf(fp, "Public key bytes & %d\\\\\n", PUBLICKEY_BYTES);
    fprintf(fp, "Secret key bytes & %d\\\\\n", SECRETKEY_BYTES);
    fprintf(fp, "Ciphertext bytes & %d\\\\\n", CIPHERTEXT_BYTES);
    fprintf(fp, "KeyGen attempts & %lu\\\\\n", stats->attempts_used);
    fprintf(fp, "Support rejects & %lu\\\\\n", stats->support_rejects);
    fprintf(fp, "Goppa polynomial rejects & %lu\\\\\n", stats->goppa_poly_rejects);
    fprintf(fp, "Systematic-form rejects & %lu\\\\\n", stats->systematic_form_rejects);
    fprintf(fp, "Goppa init rejects & %lu\\\\\n", stats->goppa_init_rejects);
    fprintf(fp, "FixedWeight rejects before success & %d\\\\\n", fw_rejects);
    fprintf(fp, "Decapsulation status & %s\\\\\n", dec_status == SUCCESS ? "success" : "failure");
    fprintf(fp, "Shared-secret match & %s\\\\\n", ss_match ? "yes" : "no");
    fprintf(fp, "\\bottomrule\\end{longtable}\n\n");
}

int main(void)
{
    FILE *fp;
    unsigned char entropy_input[48];
    unsigned char kat_seed[48];
    unsigned char keygen_seed[KEYGEN_SEED_BYTES];
    unsigned char stored_delta[KEYGEN_SEED_BYTES];
    unsigned char fallback_s[BITS_TO_BYTES(LENGTH)];
    unsigned char pk[PUBLICKEY_BYTES];
    unsigned char sk[SECRETKEY_BYTES];
    unsigned char ct[CIPHERTEXT_BYTES];
    unsigned char ss[CRYPTO_BYTES];
    unsigned char ss_dec[CRYPTO_BYTES];
    unsigned char *pk_ptr = pk;
    unsigned char *stream = NULL;
    unsigned char *stream_s = NULL;
    unsigned char *support_input = NULL;
    unsigned char *goppa_input = NULL;
    unsigned char *delta_next = NULL;
    unsigned char fixed_weight_bits[FIXED_WEIGHT_BYTES];
    unsigned char e[BITS_TO_BYTES(LENGTH)];
    unsigned char e_dec[BITS_TO_BYTES(LENGTH)];
    int error[ERROR_WEIGHT];
    int error_dec[ERROR_WEIGHT];
    int fw_rejects = 0;
    int dec_status;
    int ss_match;
    size_t q;
    size_t support_bits;
    size_t goppa_bits;
    size_t total_bits;
    size_t stream_len;
    size_t offset;
    goppa_t gamma;
    gf_t eta;
    gfelt_t *sk_L;
    gfelt_t *sk_g;
    gfelt_t *sk_eta;
    const unsigned char *sk_delta;
    const unsigned char *sk_s;
    int i;

    for (i = 0; i < 48; i++) {
        entropy_input[i] = (unsigned char)i;
    }

    randombytes_init(entropy_input, NULL, 256);
    randombytes(kat_seed, sizeof(kat_seed));
    randombytes_init(kat_seed, NULL, 256);

    gf_init(EXT_DEGREE);
    gf_set_to_zero(eta);
    randombytes(keygen_seed, sizeof(keygen_seed));
    memset(pk, 0, sizeof(pk));
    memset(fallback_s, 0, sizeof(fallback_s));

    gamma = scheme_keygen_seeded(LENGTH, ORDER, EXT_DEGREE, GOPPA_DEGREE,
                               eta, keygen_seed, sizeof(keygen_seed), &pk_ptr,
                               NULL, stored_delta, fallback_s);
    if (gamma == NULL) {
        fprintf(stderr, "scheme_keygen_seeded failed\n");
        return 1;
    }

    {
        unsigned char *p = sk;
        memcpy(p, gamma->L, LENGTH * sizeof(gfelt_t));
        p += LENGTH * sizeof(gfelt_t);
        memcpy(p, gamma->g->coeff, (GOPPA_DEGREE + 1) * sizeof(gfelt_t));
        p += (GOPPA_DEGREE + 1) * sizeof(gfelt_t);
        memcpy(p, gamma->eta, sizeof(gfelt_t));
        p += sizeof(gfelt_t);
        memcpy(p, stored_delta, sizeof(stored_delta));
        p += sizeof(stored_delta);
        memcpy(p, fallback_s, sizeof(fallback_s));
    }

    q = (size_t)1U << (unsigned)EXT_DEGREE;
    support_bits = (size_t)PARAM_SIGMA2 * (q - 1U);
    goppa_bits = (size_t)PARAM_SIGMA1 * (size_t)(PARAM_T0 + 1);
    total_bits = (size_t)LENGTH + support_bits + goppa_bits + PARAM_ELL;
    stream_len = BITS_TO_BYTES(total_bits);
    stream = malloc(stream_len);
    stream_s = malloc(BITS_TO_BYTES(LENGTH));
    support_input = malloc(BITS_TO_BYTES(support_bits));
    goppa_input = malloc(BITS_TO_BYTES(goppa_bits));
    delta_next = malloc(BITS_TO_BYTES(PARAM_ELL));
    if (stream == NULL || stream_s == NULL || support_input == NULL ||
        goppa_input == NULL || delta_next == NULL) {
        fprintf(stderr, "allocation failed\n");
        return 1;
    }
    FIPS202_SHAKE256(stored_delta, sizeof(stored_delta),
                     stream, (int)stream_len);
    offset = 0;
    copy_bits(stream_s, stream, offset, LENGTH);
    offset += LENGTH;
    copy_bits(support_input, stream, offset, support_bits);
    offset += support_bits;
    copy_bits(goppa_input, stream, offset, goppa_bits);
    copy_bits(delta_next, stream, total_bits - PARAM_ELL, PARAM_ELL);

    do {
        randombytes(fixed_weight_bits, sizeof(fixed_weight_bits));
        if (m2error(fixed_weight_bits, error) == SUCCESS) {
            break;
        }
        fw_rejects++;
    } while (1);

    error_to_vector(e, error);
    if (encrypt_nied(ct, error, pk) != SUCCESS ||
        hash_session_key(ss, 1, e, ct) != SUCCESS) {
        fprintf(stderr, "encapsulation trace failed\n");
        return 1;
    }

    dec_status = decrypt_nied(ct, error_dec, sk);
    if (dec_status == SUCCESS) {
        error_to_vector(e_dec, error_dec);
        if (hash_session_key(ss_dec, 1, e_dec, ct) != SUCCESS) {
            fprintf(stderr, "decapsulation hash failed\n");
            return 1;
        }
    } else {
        memcpy(e_dec, sk + SECRETKEY_S_OFFSET, BITS_TO_BYTES(LENGTH));
        if (hash_session_key(ss_dec, 0, e_dec, ct) != SUCCESS) {
            fprintf(stderr, "fallback hash failed\n");
            return 1;
        }
    }
    ss_match = memcmp(ss, ss_dec, CRYPTO_BYTES) == 0;

    fp = fopen(REPORT_TEX, "w");
    if (fp == NULL) {
        fprintf(stderr, "could not open report tex\n");
        return 1;
    }

    tex_begin(fp);
    tex_params(fp, scheme_last_keygen_stats(), fw_rejects, dec_status, ss_match);

    fprintf(fp, "\\section*{Deterministic RNG Inputs}\n");
    tex_hex_block(fp, "Initial entropy input", entropy_input, sizeof(entropy_input));
    tex_hex_block(fp, "KAT seed used to reinitialize DRBG", kat_seed, sizeof(kat_seed));

    fprintf(fp, "\\section*{KeyGen Intermediate Variables}\n");
    tex_hex_block(fp, "GoppaPolynomial eta as little-endian uint32 field element",
                  (unsigned char *)eta, sizeof(gfelt_t));
    tex_hex_block(fp, "Initial KeyGen seed delta0", keygen_seed, sizeof(keygen_seed));
    tex_hex_block(fp, "Final successful delta", stored_delta, sizeof(stored_delta));
    tex_hex_block(fp, "G(delta) full SHAKE256 stream", stream, stream_len);
    tex_hex_block(fp, "G(delta) slice s", stream_s, BITS_TO_BYTES(LENGTH));
    tex_hex_block(fp, "DefinedSet input bits", support_input, BITS_TO_BYTES(support_bits));
    tex_hex_block(fp, "GoppaPolynomial input bits", goppa_input, BITS_TO_BYTES(goppa_bits));
    tex_hex_block(fp, "Next delta prime from successful stream", delta_next, BITS_TO_BYTES(PARAM_ELL));
    tex_gf_array(fp, "Permuted support L stored in private key", gamma->L, LENGTH);
    tex_gf_array(fp, "Goppa polynomial coefficients g0 through gr", gamma->g->coeff,
                 GOPPA_DEGREE + 1U);
    tex_hex_block(fp, "Fallback vector s stored in private key", fallback_s, sizeof(fallback_s));

    sk_L = (gfelt_t *)sk;
    sk_g = (gfelt_t *)(sk + LENGTH * sizeof(gfelt_t));
    sk_eta = (gfelt_t *)(sk + SECRETKEY_DELTA_OFFSET - sizeof(gfelt_t));
    sk_delta = sk + SECRETKEY_DELTA_OFFSET;
    sk_s = sk + SECRETKEY_S_OFFSET;
    fprintf(fp, "\\section*{Serialized Secret Key Slices}\n");
    tex_gf_array(fp, "sk.L", sk_L, LENGTH);
    tex_gf_array(fp, "sk.g coefficients", sk_g, GOPPA_DEGREE + 1U);
    tex_gf_array(fp, "sk.eta", sk_eta, 1U);
    tex_hex_block(fp, "sk.delta", sk_delta, HASH_SIZE);
    tex_hex_block(fp, "sk.s", sk_s, BITS_TO_BYTES(LENGTH));
    tex_hex_block(fp, "Complete serialized secret key", sk, sizeof(sk));

    fprintf(fp, "\\section*{Public Matrix Variables}\n");
    tex_raw_twisted_matrix(fp, gamma);
    tex_public_matrix(fp, pk);
    tex_hex_block(fp, "Complete serialized public key", pk, sizeof(pk));

    fprintf(fp, "\\section*{Encapsulation Intermediate Variables}\n");
    tex_hex_block(fp, "FixedWeight random bits for successful sample",
                  fixed_weight_bits, sizeof(fixed_weight_bits));
    tex_int_array(fp, "Error positions", error, ERROR_WEIGHT);
    tex_hex_block(fp, "Error vector e", e, sizeof(e));
    tex_hex_block(fp, "Ciphertext C = Encode(e,T)", ct, sizeof(ct));
    tex_hex_block(fp, "Session key H(1,e,C)", ss, sizeof(ss));

    fprintf(fp, "\\section*{Decapsulation Intermediate Variables}\n");
    tex_int_array(fp, "Decoded error positions", error_dec, ERROR_WEIGHT);
    tex_hex_block(fp, "Decoded/fallback vector used for K", e_dec, sizeof(e_dec));
    tex_hex_block(fp, "Decapsulation session key", ss_dec, sizeof(ss_dec));
    fprintf(fp, "\\section*{Check Result}\n");
    fprintf(fp, "Shared-secret comparison result: \\textbf{%s}.\\\\\n",
            ss_match ? "MATCH" : "MISMATCH");
    fprintf(fp, "\\end{document}\n");
    fclose(fp);

    free_goppa(gamma);
    free(stream);
    free(stream_s);
    free(support_input);
    free(goppa_input);
    free(delta_next);

    return ss_match ? 0 : 1;
}
