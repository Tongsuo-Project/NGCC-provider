/*
 * kat_gen.c — generate KAT vectors for GALAS in the NGCC KAT_SIG format.
 *
 * Reads Seed/M pairs from API_PKC/Test_Vector/KAT_SIG_AlgorithmInstance.txt,
 * runs sig_keygen (seeded by the NGCC DRNG) and sig_sign, and writes a KAT
 * file in the same format with PK/SK/Sn filled in.
 *
 * Usage: ./kat_gen < input.txt > output.txt
 *   (input/output are the KAT_SIG_AlgorithmInstance.txt format)
 *
 * The output preserves the Seed_Len and Seed fields from the NGCC template.
 * sig_keygen is seeded with that value through galas_set_kat_seed, so PK, SK,
 * and Sn are reproducible from the KAT record.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "SIG_AlgorithmInstance.h"

/* parse one hex string of length nchars into dst (nbytes) */
static int parse_hex(uint8_t* dst, size_t nbytes, const char* s) {
    for (size_t i = 0; i < nbytes; ++i) {
        unsigned v;
        if (sscanf(s + 2 * i, "%2x", &v) != 1) return -1;
        dst[i] = (uint8_t)v;
    }
    return 0;
}

static void print_hex(const uint8_t* b, size_t n) {
    for (size_t i = 0; i < n; ++i) printf("%02X", b[i]);
}

int main(void) {
    char line[1 << 16]; long count = -1; (void)count;
    /* KAT fields */
    uint8_t seed[64]; size_t seed_len = 0;
    uint8_t msg[1 << 12]; size_t m_len = 0;
    int have_seed = 0, have_m = 0;

    unsigned long long pklen = sig_get_pk_len_bytes();
    unsigned long long sklen = sig_get_sk_len_bytes();
    unsigned long long snlen = sig_get_sn_len_bytes();
    uint8_t* pk = malloc(pklen);
    uint8_t* sk = malloc(sklen);
    uint8_t* sn = malloc(snlen);

    while (fgets(line, sizeof(line), stdin)) {
        /* strip trailing newline */
        line[strcspn(line, "\r\n")] = 0;
        if (strncmp(line, "Count = ", 8) == 0) {
            /* flush previous record if complete */
            if (have_seed && have_m) {
                galas_set_kat_seed(seed, seed_len);   /* reproducible from KAT Seed */
                unsigned long long gpk, gsk, gsn;
                int rc_kg = sig_keygen(pk, &gpk, sk, &gsk);
                int rc_sg = (rc_kg == 0) ? sig_sign(sk, gsk, msg, m_len, sn, &gsn) : -99;
                printf("Seed_Len = %zu\n", seed_len);
                printf("Seed = "); print_hex(seed, seed_len); printf("\n");
                printf("PK_Len = %llu\n", rc_kg == 0 ? gpk : 0UL);
                printf("PK = "); if (rc_kg == 0) print_hex(pk, gpk); printf("\n");
                printf("SK_Len = %llu\n", rc_kg == 0 ? gsk : 0UL);
                printf("SK = "); if (rc_kg == 0) print_hex(sk, gsk); printf("\n");
                printf("M_Len = %zu\n", m_len);
                printf("M = "); print_hex(msg, m_len); printf("\n");
                printf("Sn_Len = %llu\n", rc_sg == 0 ? gsn : 0UL);
                printf("Sn = "); if (rc_sg == 0) print_hex(sn, gsn); printf("\n\n");
            }
            count = atol(line + 8);
            printf("%s\n", line);
            have_seed = have_m = 0;
        } else if (strncmp(line, "Seed_Len = ", 11) == 0) {
            seed_len = atol(line + 11);
        } else if (strncmp(line, "Seed = ", 7) == 0) {
            if (seed_len <= sizeof(seed)) parse_hex(seed, seed_len, line + 7);
            have_seed = 1;
        } else if (strncmp(line, "M_Len = ", 8) == 0) {
            m_len = atol(line + 8);
        } else if (strncmp(line, "M = ", 4) == 0) {
            if (m_len <= sizeof(msg)) parse_hex(msg, m_len, line + 4);
            have_m = 1;
        } else if (line[0] == 0 || strncmp(line, "PK", 2) == 0 ||
                   strncmp(line, "SK", 2) == 0 || strncmp(line, "Sn", 2) == 0) {
            /* blank or output fields in input template: ignore */
        }
    }
    /* flush last record */
    if (have_seed && have_m) {
        galas_set_kat_seed(seed, seed_len);
        unsigned long long gpk, gsk, gsn;
        int rc_kg = sig_keygen(pk, &gpk, sk, &gsk);
        int rc_sg = (rc_kg == 0) ? sig_sign(sk, gsk, msg, m_len, sn, &gsn) : -99;
        printf("Seed_Len = %zu\n", seed_len);
        printf("Seed = "); print_hex(seed, seed_len); printf("\n");
        printf("PK_Len = %llu\n", rc_kg == 0 ? gpk : 0UL);
        printf("PK = "); if (rc_kg == 0) print_hex(pk, gpk); printf("\n");
        printf("SK_Len = %llu\n", rc_kg == 0 ? gsk : 0UL);
        printf("SK = "); if (rc_kg == 0) print_hex(sk, gsk); printf("\n");
        printf("M_Len = %zu\n", m_len);
        printf("M = "); print_hex(msg, m_len); printf("\n");
        printf("Sn_Len = %llu\n", rc_sg == 0 ? gsn : 0UL);
        printf("Sn = "); if (rc_sg == 0) print_hex(sn, gsn); printf("\n\n");
    }

    free(pk); free(sk); free(sn);
    return 0;
}
