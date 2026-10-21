/* verify_kat.c — read a generated KAT file, re-verify every signature.
 * Build-linked; reads KAT from stdin. Exits 0 if all verify, 1 otherwise. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "SIG_AlgorithmInstance.h"

static int hex_to_bytes(uint8_t* dst, size_t cap, size_t need, const char* s) {
    if (need > cap) return -1;
    for (size_t i = 0; i < need; ++i) { unsigned v; if (sscanf(s+2*i,"%2x",&v)!=1) return -1; dst[i]=v; }
    return 0;
}

int main(void) {
    char line[1 << 20];  /* 1 MiB — large enough for 512-bit Sn hex (~100KB) */
    long ok = 0, bad = 0, n = 0;
    /* per-record buffers */
    uint8_t pk[256], sk[128], sn[1<<16], m[1<<12];
    unsigned long long pk_len=0, sk_len=0, sn_len=0, m_len=0;
    int have = 0;
    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!strncmp(line, "Count = ", 8)) { if (have) { n++; if (sig_verify(pk,pk_len,sn,sn_len,m,m_len)==0) ok++; else bad++; } have=0; continue; }
        else if (!strncmp(line, "PK_Len = ", 9)) pk_len = atoll(line+9);
        else if (!strncmp(line, "PK = ", 5)) { if(hex_to_bytes(pk,sizeof(pk),pk_len,line+5)) bad++; }
        else if (!strncmp(line, "SK_Len = ", 9)) sk_len = atoll(line+9);
        else if (!strncmp(line, "SK = ", 5)) { if(hex_to_bytes(sk,sizeof(sk),sk_len,line+5)) bad++; }
        else if (!strncmp(line, "Sn_Len = ", 9)) sn_len = atoll(line+9);
        else if (!strncmp(line, "Sn = ", 5)) { if(hex_to_bytes(sn,sizeof(sn),sn_len,line+5)) bad++; }
        else if (!strncmp(line, "M_Len = ", 8)) m_len = atoll(line+8);
        else if (!strncmp(line, "M = ", 4)) { if(hex_to_bytes(m,sizeof(m),m_len,line+4)) bad++; have=1; }
    }
    if (have) { n++; if (sig_verify(pk,pk_len,sn,sn_len,m,m_len)==0) ok++; else bad++; }
    printf("KAT verify: %ld records, %ld ok, %ld bad\n", n, ok, bad);
    return bad ? 1 : 0;
}
