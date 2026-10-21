/* Scalar Gaussian stream used by the SM3 backend.
 * Adapted from HAETAE v2.0, Copyright (c) 2023 Team HAETAE (MIT).
 * Modifications Copyright (c) 2026 Hang Zhang. */
#define BIT_GAUSS_SM3_BATCH_CANDIDATES 127U
#define BIT_GAUSS_SM3_REFILL_CANDIDATES 8U
#define BIT_GAUSS_SM3_INITIAL_BYTES (GAUSS_RAND_BYTES * BIT_N)
#define BIT_GAUSS_SM3_INITIAL_CANDIDATES                                                           \
    ((BIT_GAUSS_SM3_INITIAL_BYTES - BIT_N / 8) / GAUSS_RAND_BYTES)

static int sample_gauss_chunk_sm3(uint64_t samples[BIT_N], uint8_t signs[BIT_N / 8], fp96_76 *sqsum,
                                  const uint8_t *seed, uint16_t nonce, int with_aux)
{
    bit_xof256_state state;
    fp96_76 lane_sum[4] = {{{0, 0}}, {{0, 0}}, {{0, 0}}, {{0, 0}}};
    uint8_t randbuf[BIT_GAUSS_SM3_BATCH_CANDIDATES * GAUSS_RAND_BYTES];
    size_t accepted = 0;
    size_t initial_remaining = BIT_GAUSS_SM3_INITIAL_CANDIDATES;
    const size_t needed = BIT_N + (size_t)with_aux;

    bit_xof256_init(&state, seed, BIT_SEEDBYTES, nonce);
    if (bit_xof256_squeeze(&state, signs, BIT_N / 8) != 0) {
        bit_xof256_zeroize(&state);
        bit_secure_zero(randbuf, sizeof(randbuf));
        bit_secure_zero(lane_sum, sizeof(lane_sum));
        return -1;
    }
    while (accepted < needed) {
        size_t candidates;

        if (initial_remaining != 0) {
            candidates = initial_remaining < BIT_GAUSS_SM3_BATCH_CANDIDATES
                             ? initial_remaining
                             : BIT_GAUSS_SM3_BATCH_CANDIDATES;
            initial_remaining -= candidates;
        } else {
            candidates = BIT_GAUSS_SM3_REFILL_CANDIDATES;
        }
        if (bit_xof256_squeeze(&state, randbuf, candidates * GAUSS_RAND_BYTES) != 0) {
            bit_xof256_zeroize(&state);
            bit_secure_zero(randbuf, sizeof(randbuf));
            bit_secure_zero(lane_sum, sizeof(lane_sum));
            return -1;
        }
        for (size_t i = 0; i < candidates && accepted < needed; i++) {
            uint64_t sample;
            fp96_76 square;
            uint64_t mask;
            int ok = sample_gauss76(&sample, &square, randbuf + i * GAUSS_RAND_BYTES);
            fp96_76 *acc = &lane_sum[i & 3U];

            mask = (uint64_t)-(int64_t)ok;
            acc->limb48[0] += square.limb48[0] & mask;
            acc->limb48[1] += square.limb48[1] & mask;
            if (ok) {
                if (accepted < BIT_N)
                    samples[accepted] = sample;
                accepted++;
            }
        }
    }
    for (size_t i = 0; i < 4; i++) {
        sqsum->limb48[0] += lane_sum[i].limb48[0];
        sqsum->limb48[1] += lane_sum[i].limb48[1];
    }
    fp_renormalize(sqsum);
    bit_xof256_zeroize(&state);
    bit_secure_zero(randbuf, sizeof(randbuf));
    bit_secure_zero(lane_sum, sizeof(lane_sum));
    return 0;
}
