#include "polarkem_polar.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "auxfunc.h"

#define POLARKEM_PERM_BLOCK_BYTES 4096u

const uint16_t polarkem_info_positions[POLARKEM_MESSAGE_BITS] = {
    255, 383, 439, 443, 445, 446, 447, 463, 471, 475, 477, 478, 479, 487, 491, 492,
    493, 494, 495, 498, 499, 500, 501, 502, 503, 504, 505, 506, 507, 508, 509, 510,
    511, 623, 631, 635, 637, 638, 639, 671, 687, 695, 698, 699, 700, 701, 702, 703,
    717, 718, 719, 723, 725, 726, 727, 729, 730, 731, 732, 733, 734, 735, 739, 741,
    742, 743, 745, 746, 747, 748, 749, 750, 751, 753, 754, 755, 756, 757, 758, 759,
    760, 761, 762, 763, 764, 765, 766, 767, 799, 811, 813, 814, 815, 819, 821, 822,
    823, 825, 826, 827, 828, 829, 830, 831, 839, 843, 845, 846, 847, 851, 853, 854,
    855, 857, 858, 859, 860, 861, 862, 863, 867, 869, 870, 871, 873, 874, 875, 876,
    877, 878, 879, 881, 882, 883, 884, 885, 886, 887, 888, 889, 890, 891, 892, 893,
    894, 895, 903, 907, 909, 910, 911, 915, 917, 918, 919, 920, 921, 922, 923, 924,
    925, 926, 927, 930, 931, 932, 933, 934, 935, 936, 937, 938, 939, 940, 941, 942,
    943, 944, 945, 946, 947, 948, 949, 950, 951, 952, 953, 954, 955, 956, 957, 958,
    959, 961, 962, 963, 964, 965, 966, 967, 968, 969, 970, 971, 972, 973, 974, 975,
    976, 977, 978, 979, 980, 981, 982, 983, 984, 985, 986, 987, 988, 989, 990, 991,
    992, 993, 994, 995, 996, 997, 998, 999, 1000, 1001, 1002, 1003, 1004, 1005, 1006, 1007,
    1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015, 1016, 1017, 1018, 1019, 1020, 1021, 1022, 1023
};

typedef struct {
    unsigned char bytes[POLARKEM_PERM_BLOCK_BYTES];
    size_t next;
    uint32_t block_number;
    const unsigned char *seed;
} polarkem_perm_stream;

/**
 * Refill a permutation stream from the next canonical 4096-byte XOF block.
 *
 * @param[in,out] stream Stream state containing the 32-byte seed, LE32 block
 *                       counter, 4096-byte buffer, and next-byte offset.
 * @return 0 on success, or -4 if pseudoXOF reports an error.
 */
static int polarkem_perm_refill(polarkem_perm_stream *stream)
{
    static const unsigned char domain[] = "PolarKEM-PERM-v1";
    unsigned char input[(sizeof(domain) - 1u) + POLARKEM_SEED_BYTES + 4u];
    size_t offset = 0u;
    uint32_t block = stream->block_number;

    memcpy(input + offset, domain, sizeof(domain) - 1u);
    offset += sizeof(domain) - 1u;
    memcpy(input + offset, stream->seed, POLARKEM_SEED_BYTES);
    offset += POLARKEM_SEED_BYTES;
    input[offset + 0u] = (unsigned char)(block & 0xffu);
    input[offset + 1u] = (unsigned char)((block >> 8) & 0xffu);
    input[offset + 2u] = (unsigned char)((block >> 16) & 0xffu);
    input[offset + 3u] = (unsigned char)((block >> 24) & 0xffu);

    if (pseudoXOF(
            (unsigned long long)POLARKEM_PERM_BLOCK_BYTES * 8ull,
            input,
            (unsigned long long)sizeof(input) * 8ull,
            stream->bytes) != 0) {
        return -4;
    }
    stream->next = 0u;
    stream->block_number++;
    return 0;
}

/**
 * Consume one four-byte little-endian word from a permutation stream.
 *
 * @param[in,out] stream Canonical block-stream state.
 * @param[out] word      Parsed 32-bit unsigned word.
 * @return 0 on success, or -4 if refilling the stream fails.
 */
static int polarkem_perm_u32(polarkem_perm_stream *stream, uint32_t *word)
{
    const unsigned char *p;

    if (stream->next > POLARKEM_PERM_BLOCK_BYTES - 4u) {
        if (polarkem_perm_refill(stream) != 0) {
            return -4;
        }
    }
    p = stream->bytes + stream->next;
    stream->next += 4u;
    *word = ((uint32_t)p[0]) |
            ((uint32_t)p[1] << 8) |
            ((uint32_t)p[2] << 16) |
            ((uint32_t)p[3] << 24);
    return 0;
}

void polarkem_polar_transform(unsigned char bits[POLARKEM_N])
{
    size_t half;

    for (half = 1u; half < POLARKEM_N; half <<= 1) {
        size_t block;
        for (block = 0u; block < POLARKEM_N; block += (half << 1)) {
            size_t j;
            for (j = 0u; j < half; ++j) {
                bits[block + j] = (unsigned char)(
                    (bits[block + j] ^ bits[block + half + j]) & 1u);
            }
        }
    }
}

void polarkem_polar_encode(
    const unsigned char mu[POLARKEM_MESSAGE_BYTES],
    unsigned char codeword[POLARKEM_N])
{
    size_t j;

    memset(codeword, 0, POLARKEM_N);
    for (j = 0u; j < POLARKEM_MESSAGE_BITS; ++j) {
        codeword[polarkem_info_positions[j]] =
            (unsigned char)((mu[j >> 3] >> (j & 7u)) & 1u);
    }
    polarkem_polar_transform(codeword);
}

void polarkem_polar_decode(
    const unsigned char codeword[POLARKEM_N],
    unsigned char mu[POLARKEM_MESSAGE_BYTES])
{
    unsigned char transformed[POLARKEM_N];
    size_t j;

    memcpy(transformed, codeword, sizeof(transformed));
    polarkem_polar_transform(transformed);
    memset(mu, 0, POLARKEM_MESSAGE_BYTES);
    for (j = 0u; j < POLARKEM_MESSAGE_BITS; ++j) {
        mu[j >> 3] |= (unsigned char)(
            (transformed[polarkem_info_positions[j]] & 1u) << (j & 7u));
    }
}

int polarkem_signed_permutation(
    const unsigned char seed[POLARKEM_SEED_BYTES],
    uint16_t permutation[POLARKEM_N],
    int8_t sign[POLARKEM_N])
{
    static const unsigned char sign_domain[] = "PolarKEM-SIGN-v1";
    unsigned char sign_input[(sizeof(sign_domain) - 1u) +
                             POLARKEM_SEED_BYTES];
    unsigned char sign_bits[(POLARKEM_N + 7u) / 8u];
    polarkem_perm_stream stream;
    size_t i;

    memset(&stream, 0, sizeof(stream));
    stream.next = POLARKEM_PERM_BLOCK_BYTES;
    stream.seed = seed;
    for (i = 0u; i < POLARKEM_N; ++i) {
        permutation[i] = (uint16_t)i;
    }

    for (i = POLARKEM_N - 1u; i > 0u; --i) {
        uint32_t sample;
        uint32_t bound = (uint32_t)(i + 1u);
        uint64_t limit = (UINT64_C(0x100000000) / bound) * bound;
        size_t j;
        uint16_t temporary;

        do {
            if (polarkem_perm_u32(&stream, &sample) != 0) {
                return -4;
            }
        } while ((uint64_t)sample >= limit);
        j = (size_t)(sample % bound);
        temporary = permutation[i];
        permutation[i] = permutation[j];
        permutation[j] = temporary;
    }

    memcpy(sign_input, sign_domain, sizeof(sign_domain) - 1u);
    memcpy(sign_input + sizeof(sign_domain) - 1u,
           seed,
           POLARKEM_SEED_BYTES);
    if (pseudoXOF(
            (unsigned long long)POLARKEM_N,
            sign_input,
            (unsigned long long)sizeof(sign_input) * 8ull,
            sign_bits) != 0) {
        return -4;
    }
    for (i = 0u; i < POLARKEM_N; ++i) {
        sign[i] = ((sign_bits[i >> 3] >> (i & 7u)) & 1u) != 0u
                      ? (int8_t)-1
                      : (int8_t)1;
    }
    return 0;
}
