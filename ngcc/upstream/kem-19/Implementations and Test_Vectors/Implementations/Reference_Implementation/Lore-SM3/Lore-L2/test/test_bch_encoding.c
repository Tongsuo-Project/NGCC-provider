#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../params.h"
#include "../bch_codec.h"

/* Original byte-table recurrence, used only as an independent test oracle. */
static void reference(struct bch_control *bch, const uint8_t *data,
                      unsigned int len, uint8_t *ecc)
{
    uint32_t words[(LORE_BCH_M * LORE_BCH_T + 31) / 32] = {0};
    unsigned int count = (bch->m * bch->t + 31) / 32;
    for (unsigned int i = 0; i < bch->ecc_bytes; ++i)
        words[i / 4] |= (uint32_t)ecc[i] << (24 - 8 * (i % 4));
    for (unsigned int j = 0; j < len; ++j) {
        const uint32_t *row = bch->mod8_tab + count * ((words[0] >> 24) ^ data[j]);
        for (unsigned int i = 0; i + 1 < count; ++i)
            words[i] = (words[i] << 8) ^ (words[i + 1] >> 24) ^ row[i];
        words[count - 1] = (words[count - 1] << 8) ^ row[count - 1];
    }
    for (unsigned int i = 0; i < bch->ecc_bytes; ++i)
        ecc[i] = (uint8_t)(words[i / 4] >> (24 - 8 * (i % 4)));
}

int main(void)
{
    struct bch_control *bch = init_bch(LORE_BCH_M, LORE_BCH_T, 0);
    if (!bch || bch->ecc_bytes > LORE_ECC_BYTES) return 1;
    uint8_t data[LORE_MSG_BYTES + 4];
    uint8_t actual[LORE_ECC_BYTES], expected[LORE_ECC_BYTES];
    int failed = 0;
    for (unsigned int trial = 0; trial < 256; ++trial) {
        for (unsigned int i = 0; i < sizeof data; ++i)
            data[i] = (uint8_t)(trial + 37 * i);
        for (unsigned int offset = 0; offset < 4; ++offset) {
            unsigned int len = trial % (LORE_MSG_BYTES + 1);
            for (unsigned int i = 0; i < sizeof actual; ++i)
                actual[i] = expected[i] = (uint8_t)(trial ^ i);
            reference(bch, data + offset, len, expected);
            encode_bch(bch, data + offset, len / 2, actual);
            encode_bch(bch, data + offset + len / 2, len - len / 2, actual);
            if (memcmp(actual, expected, sizeof actual)) failed = 1;
        }
    }
    free_bch(bch);
    if (failed) return 1;
    puts("BCH encoding: 1024 alignment/length/incremental comparisons passed");
    return 0;
}
