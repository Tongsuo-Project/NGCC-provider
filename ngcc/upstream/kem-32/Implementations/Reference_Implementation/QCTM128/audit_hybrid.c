#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <scheme_api.h>
#include <seeded_keygen.h>
#include <rng.h>

#define WORD_BITS ((int)(8 * sizeof(unsigned long)))

static int get_bit(unsigned long **M, int row, int col)
{
    return (int)((M[row][col / WORD_BITS] >> (col % WORD_BITS)) & 1UL);
}

static void flip_bit(unsigned long **M, int row, int col)
{
    M[row][col / WORD_BITS] ^= 1UL << (col % WORD_BITS);
}

static void xor_row(unsigned long *dst, const unsigned long *src, int words)
{
    int i;

    for (i = 0; i < words; i++) {
        dst[i] ^= src[i];
    }
}

static void swap_row(unsigned long **M, int a, int b)
{
    unsigned long *tmp;

    if (a == b) {
        return;
    }
    tmp = M[a];
    M[a] = M[b];
    M[b] = tmp;
}

static unsigned long **alloc_matrix(int rows, int cols)
{
    int i;
    int words = 1 + (cols - 1) / WORD_BITS;
    unsigned long **M = malloc((size_t)rows * sizeof(*M));

    if (M == NULL) {
        return NULL;
    }
    for (i = 0; i < rows; i++) {
        M[i] = calloc((size_t)words, sizeof(**M));
        if (M[i] == NULL) {
            int j;

            for (j = 0; j < i; j++) {
                free(M[j]);
            }
            free(M);
            return NULL;
        }
    }
    return M;
}

static void free_matrix(unsigned long **M, int rows)
{
    int i;

    if (M == NULL) {
        return;
    }
    for (i = 0; i < rows; i++) {
        free(M[i]);
    }
    free(M);
}

static int rank_copy(unsigned long **M, int rows, int cols)
{
    int i;
    int j;
    int rank = 0;
    int words = 1 + (cols - 1) / WORD_BITS;
    unsigned long **A = alloc_matrix(rows, cols);

    if (A == NULL) {
        return -1;
    }
    for (i = 0; i < rows; i++) {
        memcpy(A[i], M[i], (size_t)words * sizeof(**A));
    }

    for (j = 0; j < cols && rank < rows; j++) {
        int pivot = -1;

        for (i = rank; i < rows; i++) {
            if (get_bit(A, i, j)) {
                pivot = i;
                break;
            }
        }
        if (pivot < 0) {
            continue;
        }
        swap_row(A, pivot, rank);
        for (i = 0; i < rows; i++) {
            if (i != rank && get_bit(A, i, j)) {
                xor_row(A[i], A[rank], words);
            }
        }
        rank++;
    }

    free_matrix(A, rows);
    return rank;
}

static int stacked_rank(unsigned long **A, int rows_a,
                        unsigned long **B, int rows_b, int cols)
{
    int i;
    int words = 1 + (cols - 1) / WORD_BITS;
    unsigned long **S = alloc_matrix(rows_a + rows_b, cols);
    int rank;

    if (S == NULL) {
        return -1;
    }
    for (i = 0; i < rows_a; i++) {
        memcpy(S[i], A[i], (size_t)words * sizeof(**S));
    }
    for (i = 0; i < rows_b; i++) {
        memcpy(S[rows_a + i], B[i], (size_t)words * sizeof(**S));
    }
    rank = rank_copy(S, rows_a + rows_b, cols);
    free_matrix(S, rows_a + rows_b);
    return rank;
}

static int build_full_and_hybrid(unsigned long **full_H,
                                 unsigned long **hybrid_H,
                                 gfelt_t *L, poly_t g,
                                 int n, int t, gf_t eta)
{
    int i;
    int j;
    int k;
    int m = gf_extd();
    poly_t col = poly_alloc(t - 1);

    if (col == NULL) {
        return 0;
    }

    for (i = 0; i < n; i++) {
        gfindex_t x;

        poly_syndrome_twisted_explicit_row(col, L + i, g, eta, t - 1);
        for (j = 0; j < t; j++) {
            x = gf_to_index(poly_coeff(col, j));
            for (k = 0; k < m; k++) {
                if (x & (1 << k)) {
                    flip_bit(full_H, j * m + k, i);
                    if (j < t - 1) {
                        flip_bit(hybrid_H, j * m + k, i);
                    }
                }
            }
        }
        flip_bit(hybrid_H, m * (t - 1), i);
    }

    poly_free(col);
    return 1;
}

int main(void)
{
    unsigned char entropy_input[48];
    unsigned char seed[KEYGEN_SEED_BYTES];
    goppa_t gamma;
    gf_t eta;
    unsigned long **full_H;
    unsigned long **hybrid_H;
    int i;
    int full_rows = EXT_DEGREE * GOPPA_DEGREE;
    int hybrid_rows = EXT_DEGREE * (GOPPA_DEGREE - 1) + 1;
    int full_rank;
    int hybrid_rank;
    int stacked;
    int entropy_offset = 0;
    const char *offset_text = getenv("hybrid_ENTROPY_OFFSET");

    if (offset_text != NULL && offset_text[0] != '\0') {
        entropy_offset = atoi(offset_text);
    }

    for (i = 0; i < 48; i++) {
        entropy_input[i] = (unsigned char)(i + entropy_offset);
    }
    randombytes_init(entropy_input, NULL, 256);
    randombytes(seed, sizeof(seed));

    gf_init(EXT_DEGREE);
    if (!goppa_fixed_eta(ORDER, eta)) {
        fprintf(stderr, "eta generation failed\n");
        return 2;
    }

    gamma = scheme_keygen_seeded(LENGTH, ORDER, EXT_DEGREE, GOPPA_DEGREE,
                               eta, seed, sizeof(seed), NULL, NULL, NULL, NULL);
    if (gamma == NULL) {
        fprintf(stderr, "seeded Gamma generation failed\n");
        return 1;
    }

    full_H = alloc_matrix(full_rows, LENGTH);
    hybrid_H = alloc_matrix(hybrid_rows, LENGTH);
    if (full_H == NULL || hybrid_H == NULL ||
        !build_full_and_hybrid(full_H, hybrid_H, gamma->L, gamma->g,
                               LENGTH, GOPPA_DEGREE, eta)) {
        fprintf(stderr, "matrix construction failed\n");
        free_matrix(full_H, full_rows);
        free_matrix(hybrid_H, hybrid_rows);
        free_goppa(gamma);
        return 2;
    }

    full_rank = rank_copy(full_H, full_rows, LENGTH);
    hybrid_rank = rank_copy(hybrid_H, hybrid_rows, LENGTH);
    stacked = stacked_rank(full_H, full_rows, hybrid_H, hybrid_rows, LENGTH);

    printf("hybrid all-one-row audit\n");
    printf("parameters: m=%d n=%d l=%d t=%d\n",
           EXT_DEGREE, LENGTH, ORDER, GOPPA_DEGREE);
    printf("full binary rows=%d rank=%d\n", full_rows, full_rank);
    printf("hybrid rows=m*(t-1)+1=%d rank=%d\n",
           hybrid_rows, hybrid_rank);
    printf("stacked rank rank([H_full;H_hybrid])=%d\n", stacked);
    printf("ordinary systematic possible=%s\n",
           hybrid_rank == hybrid_rows ? "yes" : "no");
    printf("row-space equivalent to full decoder matrix=%s\n",
           full_rank == hybrid_rank && hybrid_rank == stacked ? "yes" : "no");
    printf("full rows recoverable from hybrid syndrome=%s\n",
           full_rank == stacked ? "yes" : "no");
    printf("hybrid row count mod l=%d\n", hybrid_rows % ORDER);

    free_matrix(full_H, full_rows);
    free_matrix(hybrid_H, hybrid_rows);
    free_goppa(gamma);
    return full_rank == hybrid_rank && hybrid_rank == stacked ? 0 : 1;
}
