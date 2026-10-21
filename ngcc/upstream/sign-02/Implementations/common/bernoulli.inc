/* Copyright (c) 2026 Hang Zhang. */

/* Return the next base-2^32 digit of rem/den and update the remainder. */
static uint32_t ratio_digit32(uint64_t *rem, uint64_t den)
{
    uint32_t digit = 0;
    for (unsigned int i = 0; i < 32; i++) {
        uint64_t carry = *rem >> 63;
        *rem <<= 1;
        digit <<= 1;
        uint64_t take = carry | (uint64_t)(*rem >= den);
        *rem -= den & (UINT64_C(0) - take);
        digit |= (uint32_t)take;
    }
    return digit;
}

/* Exact Bernoulli(num/den); -1 means the nonce domain is exhausted. */
static int sample_bernoulli_ratio(const uint8_t *seed, uint16_t *nonce, uint64_t num, uint64_t den)
{
    bit_xof256_state st;
    uint64_t rem = num;

    if (*nonce == UINT16_MAX)
        return -1;
    bit_xof256_init(&st, seed, BIT_SEEDBYTES, *nonce);
    *nonce = (uint16_t)(*nonce + 1U);

    if ((num == 0) | (num >= den)) {
        bit_xof256_zeroize(&st);
        return num >= den;
    }

    for (;;) {
        uint8_t buf[4];
        uint32_t digit = ratio_digit32(&rem, den);
        uint32_t r;

        if (bit_xof256_squeeze(&st, buf, sizeof(buf)) != 0) {
            bit_xof256_zeroize(&st);
            return -1;
        }
        r = (uint32_t)load16_le(buf) | ((uint32_t)load16_le(buf + 2) << 16);
        if (r != digit) {
            int accept = r < digit;
            bit_xof256_zeroize(&st);
            return accept;
        }
    }
}
