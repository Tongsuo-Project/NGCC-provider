#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "kem.h"
#include "rng.h"
#include "scheme_api.h"
#include "goppa.h"
#include "m2e.h"
#include "seeded_keygen.h"

void FIPS202_SHAKE256(const unsigned char *input, unsigned int inputByteLen,
                      unsigned char *output, int outputByteLen);

static int debug_last_error[ERROR_WEIGHT];
static int debug_last_error_valid = 0;

static int trace_dec_enabled(void)
{
    return getenv("LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_TRACE_DEC") != NULL;
}

void xor(unsigned char *a, unsigned char *b, int length)
{
    int i;

    for (i = 0; i < length; i++) {
        a[i] ^= b[i];
    }
}

static int scheme_public_t_aligned(void)
{
    return (SYSTEMATIC_QC_ROWS % ORDER) == 0 &&
           ((LENGTH - SYSTEMATIC_QC_ROWS) % ORDER) == 0 &&
           SYSTEMATIC_TAIL_ROWS == 2 &&
           CODIMENSION == MATGEN_TOTAL_ROWS &&
           PUBLICKEY_BITS == PUBLIC_T_BITS;
}

static int pk_bit(const unsigned char *pk, int bit)
{
    return (pk[(size_t)bit / 8U] >> (bit % 8)) & 1U;
}

static int shifted_public_position(int public_pos, int shift)
{
    int block = public_pos / ORDER;
    int in_block = public_pos % ORDER;

    return block * ORDER + ((in_block + ORDER - shift) % ORDER);
}

static int hybrid_pk_t_bit(const unsigned char *pk, int row, int col)
{
    if (row < SYSTEMATIC_QC_ROWS) {
        int base_block = row / ORDER;
        int shift = row % ORDER;
        int public_pos = col;
        int src_public_pos = shifted_public_position(public_pos, shift);

        return pk_bit(pk, base_block * PUBLIC_T_COLUMNS + src_public_pos);
    }
    return pk_bit(pk, PSI_T1_BITS +
                  (row - SYSTEMATIC_QC_ROWS) * PUBLIC_T_COLUMNS + col);
}

static void apply_decode_map(unsigned char *out, const unsigned char *map,
                             const unsigned char *in)
{
    int i;
    int j;
    int in_bytes = BITS_TO_BYTES(CODIMENSION);

    memset(out, 0, FULL_SYNDROME_BYTES);
    for (i = 0; i < FULL_SYNDROME_LENGTH; i++) {
        unsigned char acc = 0;
        const unsigned char *row = map + (size_t)i * in_bytes;

        for (j = 0; j < in_bytes; j++) {
            unsigned char v = row[j] & in[j];
            v ^= (unsigned char)(v >> 4);
            v ^= (unsigned char)(v >> 2);
            v ^= (unsigned char)(v >> 1);
            acc ^= (unsigned char)(v & 1U);
        }
        if (acc) {
            out[i / 8] ^= (unsigned char)(1U << (i % 8));
        }
    }
}

static void poly_to_bytes(unsigned char *out, poly_t p, int degree)
{
    int i;
    int j;

    memset(out, 0, FULL_SYNDROME_BYTES);
    for (i = 0; i < degree; i++) {
        gfindex_t x = gf_to_index(poly_coeff(p, i));

        for (j = 0; j < gf_extd(); j++) {
            if ((x >> j) & 1U) {
                int bit = i * gf_extd() + j;

                out[bit / 8] ^= (unsigned char)(1U << (bit % 8));
            }
        }
    }
}

static void bytes_to_poly(const unsigned char *in, poly_t p, int degree)
{
    int i;
    int j;

    poly_set_to_zero(p);
    for (i = 0; i < degree; i++) {
        gfindex_t x = 0;
        gf_t a;

        for (j = 0; j < gf_extd(); j++) {
            int bit = i * gf_extd() + j;

            if ((in[bit / 8] >> (bit % 8)) & 1U) {
                x ^= (gfindex_t)(1U << j);
            }
        }
        gf_from_index(a, x);
        poly_set_coeff(p, i, a);
    }
    poly_calcule_deg(p);
}

static int debug_sun_syndrome_to_sui_definition(poly_t R, poly_t S, poly_t g)
{
    int i;
    int j;
    int t;
    gf_t coeff;
    gf_t term;

    if (R == NULL || S == NULL || g == NULL) {
        return 0;
    }
    t = poly_deg(g);
    if (t <= 0) {
        return 0;
    }

    poly_set_to_zero(R);
    for (i = 0; i < t; i++) {
        gf_set_to_zero(coeff);
        for (j = i + 1; j <= t; j++) {
            gf_mul(term, poly_coeff(g, j), poly_coeff(S, j - 1 - i));
            gf_add(coeff, coeff, term);
        }
        poly_set_coeff(R, i, coeff);
    }
    poly_calcule_deg(R);
    return 1;
}

static int sun_syndrome_bytes_to_definition(unsigned char *out,
                                            const unsigned char *sun,
                                            poly_t g)
{
    poly_t sun_poly;
    poly_t def_poly;

    sun_poly = poly_alloc(GOPPA_DEGREE);
    def_poly = poly_alloc(GOPPA_DEGREE);
    if (sun_poly == NULL || def_poly == NULL) {
        poly_free(sun_poly);
        poly_free(def_poly);
        return FAIL;
    }

    bytes_to_poly(sun, sun_poly, GOPPA_DEGREE);
    if (!debug_sun_syndrome_to_sui_definition(def_poly, sun_poly, g)) {
        poly_free(sun_poly);
        poly_free(def_poly);
        return FAIL;
    }
    poly_to_bytes(out, def_poly, GOPPA_DEGREE);

    poly_free(sun_poly);
    poly_free(def_poly);
    return SUCCESS;
}

static int compare_debug_full_syndrome(const unsigned char *actual,
                                       gfelt_t *L, poly_t g, gf_t eta)
{
    int i;
    int diff = 0;
    poly_t acc;
    poly_t col;
    poly_t sun_col;
    poly_t def_col;
    unsigned char expected[FULL_SYNDROME_BYTES];

    if (!debug_last_error_valid) {
        return 0;
    }

    acc = poly_alloc(GOPPA_DEGREE);
    col = poly_alloc(GOPPA_DEGREE);
    sun_col = poly_alloc(GOPPA_DEGREE);
    def_col = poly_alloc(GOPPA_DEGREE);
    if (acc == NULL || col == NULL || sun_col == NULL || def_col == NULL) {
        poly_free(acc);
        poly_free(col);
        poly_free(sun_col);
        poly_free(def_col);
        return -1;
    }

    poly_set_to_zero(acc);
    for (i = 0; i < ERROR_WEIGHT; i++) {
        int j;

        poly_syndrome_twisted_explicit_row(col, L + debug_last_error[i],
                                           g, eta, DEFAULT_TWIST_ROW);
        poly_set_to_zero(sun_col);
        for (j = 0; j < GOPPA_DEGREE - 1; j++) {
            poly_set_coeff(sun_col, j, poly_coeff(col, j));
        }
        poly_set_coeff_to_unit(sun_col, GOPPA_DEGREE - 1);
        poly_calcule_deg(sun_col);
        if (!debug_sun_syndrome_to_sui_definition(def_col, sun_col, g)) {
            poly_free(acc);
            poly_free(col);
            poly_free(sun_col);
            poly_free(def_col);
            return -1;
        }
        for (j = 0; j < GOPPA_DEGREE; j++) {
            poly_addto_coeff(acc, j, poly_coeff(def_col, j));
        }
    }
    poly_calcule_deg(acc);
    poly_to_bytes(expected, acc, GOPPA_DEGREE);
    for (i = 0; i < FULL_SYNDROME_BYTES; i++) {
        if (expected[i] != actual[i]) {
            diff++;
        }
    }
    poly_free(acc);
    poly_free(col);
    poly_free(sun_col);
    poly_free(def_col);
    return diff;
}

static void error_to_vector(unsigned char *e, const int *error)
{
    int i;

    memset(e, 0, BITS_TO_BYTES(LENGTH));
    for (i = 0; i < ERROR_WEIGHT; i++) {
        e[error[i] / 8] ^= (unsigned char)(1U << (error[i] % 8));
    }
}

static void set_bit_msb(unsigned char *out, size_t bit, int value)
{
    if (value) {
        out[bit / 8U] |= (unsigned char)(1U << (7U - (bit % 8U)));
    }
}

static int get_bit_msb(const unsigned char *in, size_t bit)
{
    return (in[bit / 8U] >> (7U - (bit % 8U))) & 1U;
}

static int get_bit_lsb(const unsigned char *in, size_t bit)
{
    return (in[bit / 8U] >> (bit % 8U)) & 1U;
}

static void set_bit_lsb(unsigned char *out, size_t bit, int value)
{
    if (value) {
        out[bit / 8U] |= (unsigned char)(1U << (bit % 8U));
    }
}

static void append_field_element_msb(unsigned char *out, size_t *bitpos,
                                     gf_t a)
{
    int j;
    gfindex_t x = gf_to_index(a);

    for (j = 0; j < EXT_DEGREE; j++) {
        set_bit_msb(out, *bitpos, (int)((x >> j) & 1U));
        (*bitpos)++;
    }
}

static void read_field_element_msb(gf_t a, const unsigned char *in,
                                   size_t *bitpos)
{
    int j;
    gfindex_t x = 0;

    for (j = 0; j < EXT_DEGREE; j++) {
        if (get_bit_msb(in, *bitpos)) {
            x |= (gfindex_t)(1U << j);
        }
        (*bitpos)++;
    }
    gf_from_index(a, x);
}

static int serialize_gamma_prime(unsigned char *out, poly_t g, gfelt_t *L)
{
    int i;
    size_t bitpos = 0;

    if (out == NULL || g == NULL || L == NULL) {
        return FAIL;
    }

    memset(out, 0, SECRETKEY_GAMMA_BYTES);
    for (i = 0; i < GOPPA_DEGREE; i++) {
        append_field_element_msb(out, &bitpos, poly_coeff(g, i));
    }
    for (i = 0; i < LENGTH; i++) {
        append_field_element_msb(out, &bitpos, L + i);
    }

    return bitpos == (size_t)SECRETKEY_GAMMA_BITS ? SUCCESS : FAIL;
}

static int deserialize_gamma_prime(const unsigned char *in, poly_t g,
                                   gfelt_t *L, gf_t eta)
{
    int i;
    size_t bitpos = 0;

    if (in == NULL || g == NULL || L == NULL || eta == NULL) {
        return FAIL;
    }

    poly_set_to_zero(g);
    for (i = 0; i < GOPPA_DEGREE; i++) {
        read_field_element_msb(poly_coeff(g, i), in, &bitpos);
    }
    poly_set_coeff_to_unit(g, GOPPA_DEGREE);
    poly_set_deg(g, GOPPA_DEGREE);
    for (i = 0; i < LENGTH; i++) {
        read_field_element_msb(L + i, in, &bitpos);
    }
    if (bitpos != (size_t)SECRETKEY_GAMMA_BITS ||
        gf_is_zero(poly_coeff(g, GOPPA_DEGREE - 1))) {
        return FAIL;
    }
    gf_inv(eta, poly_coeff(g, GOPPA_DEGREE - 1));
    return SUCCESS;
}

static void pack_bits_msb_from_lsb(unsigned char *out,
                                   const unsigned char *in,
                                   size_t bit_len)
{
    size_t i;

    memset(out, 0, BITS_TO_BYTES(bit_len));
    for (i = 0; i < bit_len; i++) {
        set_bit_msb(out, i, get_bit_lsb(in, i));
    }
}

static void unpack_bits_lsb_from_msb(unsigned char *out,
                                     const unsigned char *in,
                                     size_t bit_len)
{
    size_t i;

    memset(out, 0, BITS_TO_BYTES(bit_len));
    for (i = 0; i < bit_len; i++) {
        set_bit_lsb(out, i, get_bit_msb(in, i));
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

int encrypt_nied(OUT unsigned char *syndrome, IN int *e, unsigned char *pk)
{
    int i;
    unsigned char *syn_aux;

    if (!scheme_public_t_aligned()) {
        return FAIL;
    }

    syn_aux = malloc(BITS_TO_BYTES(CODIMENSION) * sizeof(unsigned char));
    if (syn_aux == NULL) {
        free(syn_aux);
        return FAIL;
    }

    memset(syn_aux, 0, BITS_TO_BYTES(CODIMENSION));

    for (i = 0; i < ERROR_WEIGHT; ++i) {
        if (e[i] < SYSTEMATIC_QC_ROWS) {
            syn_aux[e[i] / 8] ^= (unsigned char)(1U << (e[i] % 8));
        } else {
            int col = e[i] - SYSTEMATIC_QC_ROWS;
            int row;

            for (row = 0; row < CODIMENSION; row++) {
                if (hybrid_pk_t_bit(pk, row, col)) {
                    syn_aux[row / 8] ^=
                        (unsigned char)(1U << (row % 8));
                }
            }
        }
    }

    memcpy(syndrome, syn_aux, SYNDROME_BYTES);

    free(syn_aux);
    return SUCCESS;
}

int decrypt_nied(IN unsigned char *syndrome, OUT int *error,
                 IN unsigned char *sk, IN unsigned char *pk)
{
    int i;
    int trace_dec = trace_dec_enabled();
    goppa_t gamma;
    gfelt_t *L;
    poly_t g;
    gf_t eta;
    unsigned char *decode_map;
    unsigned char sun_syndrome[FULL_SYNDROME_BYTES];
    unsigned char explicit_syndrome[FULL_SYNDROME_BYTES];

    if (!scheme_public_t_aligned()) {
        if (trace_dec) {
            fprintf(stderr, "dec: public T alignment failed\n");
        }
        return FAIL;
    }

    gf_init(EXT_DEGREE);

    L = malloc((size_t)LENGTH * sizeof(*L));
    if (L == NULL) {
        return FAIL;
    }

    g = poly_alloc(GOPPA_DEGREE);
    if (g == NULL) {
        free(L);
        return FAIL;
    }
    if (deserialize_gamma_prime(sk, g, L, eta) != SUCCESS) {
        free(L);
        poly_free(g);
        return FAIL;
    }

    decode_map = calloc(DECODE_MAP_BYTES, sizeof(*decode_map));
    if (decode_map == NULL) {
        free(L);
        poly_free(g);
        free(decode_map);
        return FAIL;
    }
    if (!goppa_decode_map_from_public_key(L, g, LENGTH, ORDER, GOPPA_DEGREE,
                                          eta, pk, decode_map)) {
        if (trace_dec) {
            fprintf(stderr, "dec: public-key decode map failed\n");
        }
        free(L);
        poly_free(g);
        free(decode_map);
        return FAIL;
    }

    gamma = goppa_init(L, g, LENGTH, GOPPA_DEGREE, ORDER, NULL, eta);
    if (gamma == NULL) {
        free(L);
        poly_free(g);
        free(decode_map);
        return FAIL;
    }

    if (trace_dec) {
        fprintf(stderr, "dec: apply_decode_map\n");
        fflush(stderr);
    }
    apply_decode_map(sun_syndrome, decode_map, syndrome);
    if (sun_syndrome_bytes_to_definition(explicit_syndrome, sun_syndrome,
                                         g) != SUCCESS) {
        free_goppa(gamma);
        free(decode_map);
        return FAIL;
    }
    if (trace_dec && debug_last_error_valid) {
        int diff = compare_debug_full_syndrome(explicit_syndrome,
                                               gamma->L, gamma->g, eta);

        fprintf(stderr, "dec: full_syndrome_diff_bytes=%d\n", diff);
        fflush(stderr);
    }

    gamma->explicit_syndrome_input = 2;
    if (trace_dec) {
        fprintf(stderr, "dec: goppa_decode\n");
        fflush(stderr);
    }
    i = goppa_decode(explicit_syndrome, error, gamma);

    free_goppa(gamma);
    free(decode_map);

    if (i != ERROR_WEIGHT) {
        if (trace_dec) {
            fprintf(stderr, "dec: decoded_weight=%d expected=%d\n",
                    i, ERROR_WEIGHT);
        }
        return FAIL;
    }
    if (trace_dec) {
        fprintf(stderr, "dec: success weight=%d\n", i);
    }
    return SUCCESS;
}

int crypto_kem_keypair(OUT unsigned char *pk, OUT unsigned char *sk)
{
    goppa_t gamma = NULL;
    unsigned char keygen_seed[KEYGEN_SEED_BYTES];
    unsigned char fallback_s[BITS_TO_BYTES(LENGTH)];
    unsigned char *decode_map_check;
    gf_t eta;
    int attempt;

    if (!scheme_public_t_aligned()) {
        return FAIL;
    }

    memset(pk, 0, PUBLICKEY_BYTES);
    memset(fallback_s, 0, sizeof(fallback_s));
    gf_init(EXT_DEGREE);
    gf_set_to_zero(eta);
    decode_map_check = calloc(DECODE_MAP_BYTES, sizeof(*decode_map_check));
    if (decode_map_check == NULL) {
        return FAIL;
    }

    for (attempt = 0; attempt < 64; attempt++) {
        memset(pk, 0, PUBLICKEY_BYTES);
        memset(fallback_s, 0, sizeof(fallback_s));
        memset(decode_map_check, 0, DECODE_MAP_BYTES);
        randombytes(keygen_seed, sizeof(keygen_seed));
#ifdef LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_BASELINE
        if (!goppa_fixed_eta(ORDER, eta)) {
            free(decode_map_check);
            return FAIL;
        }
        randombytes(fallback_s, sizeof(fallback_s));
        gamma = goppa_keygen_rand(LENGTH, ORDER, EXT_DEGREE, GOPPA_DEGREE,
                                  eta, &pk, NULL);
#else
        gamma = scheme_keygen_seeded(LENGTH, ORDER, EXT_DEGREE, GOPPA_DEGREE,
                                   eta, keygen_seed, sizeof(keygen_seed), &pk,
                                   NULL, NULL, fallback_s);
#endif
        if (gamma != NULL &&
            goppa_decode_map_from_public_key(gamma->L, gamma->g, LENGTH,
                                             ORDER, GOPPA_DEGREE, gamma->eta,
                                             pk, decode_map_check)) {
            break;
        }
        if (gamma != NULL) {
            free_goppa(gamma);
            gamma = NULL;
        }
    }
    free(decode_map_check);
    if (gamma == NULL) {
        return FAIL;
    }

    if (serialize_gamma_prime(sk, gamma->g, gamma->L) != SUCCESS) {
        free_goppa(gamma);
        return FAIL;
    }
    sk += SECRETKEY_GAMMA_BYTES;

    memcpy(sk, pk, PUBLICKEY_BYTES);
    sk += PUBLICKEY_BYTES;

    pack_bits_msb_from_lsb(sk, fallback_s, LENGTH);

    free_goppa(gamma);
    return SUCCESS;
}

int crypto_kem_enc(OUT unsigned char *ct, OUT unsigned char *ss,
                   IN unsigned char *pk)
{
    int *error;
    unsigned char *e;
    int status;

    error = malloc(ERROR_WEIGHT * sizeof(int));
    e = calloc(BITS_TO_BYTES(LENGTH), sizeof(unsigned char));
    if (error == NULL || e == NULL) {
        free(error);
        free(e);
        return FAIL;
    }

    status = fixed_weight_random(error);
    if (status == SUCCESS && trace_dec_enabled()) {
        memcpy(debug_last_error, error, sizeof(debug_last_error));
        debug_last_error_valid = 1;
    }
    if (status == SUCCESS) {
        status = encrypt_nied(ct, error, (unsigned char *)pk);
    }
    if (status == SUCCESS) {
        error_to_vector(e, error);
        status = hash_session_key(ss, 1, e, ct);
    }

    free(error);
    free(e);
    return status;
}

int crypto_kem_dec(OUT unsigned char *ss, IN unsigned char *ct,
                   IN unsigned char *sk)
{
    int error[ERROR_WEIGHT];
    unsigned char *e;
    const unsigned char *fallback_s;
    unsigned char b = 1;
    int status;

    e = calloc(BITS_TO_BYTES(LENGTH), sizeof(unsigned char));
    if (e == NULL) {
        return FAIL;
    }

    fallback_s = sk + SECRETKEY_S_OFFSET;
    if (decrypt_nied((unsigned char *)ct, error, sk,
                     sk + SECRETKEY_PK_OFFSET) == SUCCESS) {
        error_to_vector(e, error);
    } else {
        unpack_bits_lsb_from_msb(e, fallback_s, LENGTH);
        b = 0;
    }

    status = hash_session_key(ss, b, e, ct);
    free(e);
    return status;
}
