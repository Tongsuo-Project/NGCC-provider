/* test_xof.c — smoke test for the XOF layer over pseudoXOF. */
#include <stdio.h>
#include <string.h>
#include "xof.h"

int main(void) {
    /* determinism: same input -> same output */
    uint8_t a[32], b[32];
    xof_oneshot(128, XOF_DOMAIN_H2, "hello", 5, a, 32);
    xof_oneshot(128, XOF_DOMAIN_H2, "hello", 5, b, 32);
    printf("determinism: %s\n", memcmp(a, b, 32) == 0 ? "ok" : "FAIL");

    /* Raw xof_* domains select the backend only.  H0..H4 domain separation is
       explicit in random_oracle.c, where the FAEST separator byte is absorbed
       before finalization. */
    uint8_t c[32];
    xof_oneshot(128, XOF_DOMAIN_H1, "hello", 5, c, 32);
    printf("raw-domain-neutral: %s\n", memcmp(a, c, 32) == 0 ? "ok" : "FAIL");

    /* incremental == oneshot */
    xof_ctx ctx;
    xof_init(&ctx, 128, XOF_DOMAIN_H2);
    xof_update(&ctx, "hel", 3);
    xof_update(&ctx, "lo", 2);
    xof_final(&ctx);
    uint8_t d[32];
    xof_squeeze(&ctx, d, 16);
    xof_squeeze(&ctx, d + 16, 16);
    xof_clear(&ctx);
    printf("incremental: %s\n", memcmp(a, d, 32) == 0 ? "ok" : "FAIL");

    /* extend: squeeze 16 then 32 more == squeeze 48 at once (first 48 bytes) */
    uint8_t full48[48], split48[48];
    xof_oneshot(128, XOF_DOMAIN_H3, "abc", 3, full48, 48);
    xof_init(&ctx, 128, XOF_DOMAIN_H3);
    xof_update(&ctx, "abc", 3); xof_final(&ctx);
    xof_squeeze(&ctx, split48, 16);
    xof_squeeze(&ctx, split48 + 16, 32);
    xof_clear(&ctx);
    printf("extend:      %s\n", memcmp(full48, split48, 48) == 0 ? "ok" : "FAIL");

    return 0;
}
