/*
 * rans_oracle.c -- C rANS encode oracle for the SHUTTLE Python cross-check
 * (P12, deliverable 2).
 *
 * Reads a symbol triple in the ref/test/rans_vectors_<set>.txt format from a
 * file path given on argv[1], calls shuttle_rans_encode, and prints
 * "RANSCOM <hex>" + "RANSLEN <n>".  The Python side (xcheck_rans.py) encodes
 * the SAME symbols with rans_ref and asserts byte-equality -- proving the
 * Python rANS codec is byte-exact to the C on real, model-distributed
 * response vectors.
 *
 * Build:
 *   gcc -O2 -std=c99 -I../../ref -I../../tools -DDISABLE_NAMESPACE=1 \
 *       -DSHUTTLE_MODE=<m> rans_oracle.c ../../ref/rans.c -o rans_oracle_<m>
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rans.h"

static size_t read_ints(FILE *f, const char *key, int32_t *out, size_t cap)
{
    char line[1 << 16];
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#')
            continue;
        char *sp = strchr(line, ' ');
        if (!sp)
            continue;
        size_t klen = (size_t)(sp - line);
        if (strncmp(line, key, klen) != 0 || strlen(key) != klen)
            continue;
        size_t n = 0;
        char *tok = strtok(sp + 1, " \t\r\n");
        while (tok && n < cap) {
            out[n++] = (int32_t)atoi(tok);
            tok = strtok(NULL, " \t\r\n");
        }
        return n;
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <vector.txt>\n", argv[0]);
        return 2;
    }
    static int32_t q0[1 << 14], qs[1 << 14], hh[1 << 14];
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        return 2;
    }
    size_t nq0 = read_ints(f, "q0", q0, 1 << 14);
    rewind(f);
    size_t nqs = read_ints(f, "qs", qs, 1 << 14);
    rewind(f);
    size_t nh = read_ints(f, "h", hh, 1 << 14);
    fclose(f);

    uint8_t com[RANS_RESERVED_BYTES];
    size_t clen = 0;
    int rc = shuttle_rans_encode(com, &clen, RANS_RESERVED_BYTES, q0, qs, hh,
                                 nq0, nqs, nh);
    if (rc != 0) {
        printf("RANSRC %d\n", rc);
        return 1;
    }
    printf("RANSLEN %zu\n", clen);
    printf("RANSCOM ");
    for (size_t i = 0; i < clen; ++i)
        printf("%02x", com[i]);
    printf("\n");
    return 0;
}
