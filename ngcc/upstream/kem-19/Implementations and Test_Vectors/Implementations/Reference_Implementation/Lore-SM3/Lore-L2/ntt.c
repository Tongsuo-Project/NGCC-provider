#include <stdio.h>
#include <stdint.h>
#include "params.h"
#include "ntt.h"
#include "reduce.h"

/* Precomputed zeta values for NTT */
int16_t zetas[128] = {
       1,   -16,    64,     4,    -8,   128,     2,   -32,
    -121,  -120,   -34,    30,   -60,   -68,    15,    17,
      81,   -11,    44,    67,   123,    88,   -95,   -22,
     -35,    46,    73,   117,    23,  -111,   -70,    92,
       9,   113,    62,    36,   -72,   124,    18,   -31,
     -61,   -52,   -49,    13,   -26,   -98,  -122,  -104,
     -42,   -99,  -118,    89,    79,    21,   -84,    59,
     -58,  -100,  -114,    25,   -50,    29,  -116,    57,
       3,   -48,   -65,    12,   -24,   127,     6,   -96,
    -106,  -103,  -102,    90,    77,    53,    45,    51,
     -14,   -33,  -125,   -56,   112,     7,   -28,   -66,
    -105,  -119,   -38,    94,    69,   -76,    47,    19,
      27,    82,   -71,   108,    41,   115,    54,   -93,
      74,   101,   110,    39,   -78,   -37,  -109,   -55,
    -126,   -40,   -97,    10,   -20,    63,     5,   -80,
      83,   -43,   -85,    75,   107,    87,   -91,   -86,
};

int16_t fqmul(int16_t a, int16_t b) {
  return (int16_t)montgomery_reduce((int64_t)a * b);
}

/* N=512 */
#if LORE_N == 512
/* Multiplication of polynomials in Zq[X]/(X^4-zeta) */
static void basemul4(int16_t r[4], const int16_t a[4], const int16_t b[4], int16_t zeta) {
    int32_t c[7] = {0};
    for(int i=0; i<4; i++) {
        for(int j=0; j<4; j++) {
            c[i+j] += (int32_t)fqmul(a[i], b[j]);
        }
    }
    r[0] = (int16_t)(c[0] + fqmul((int16_t)c[4], zeta));
    r[1] = (int16_t)(c[1] + fqmul((int16_t)c[5], zeta));
    r[2] = (int16_t)(c[2] + fqmul((int16_t)c[6], zeta));
    r[3] = (int16_t)c[3];
}

void ntt(int16_t r[512]) {
    unsigned int len, start, j, k;
    int16_t t, zeta;
    k = 1;
    for(len = 256; len >= 4; len >>= 1) {
        for(start = 0; start < 512; start = j + len) {
            zeta = zetas[k++];
            for(j = start; j < start + len; j++) {
                t = fqmul(zeta, r[j + len]);
                r[j + len] = r[j] - t;
                r[j] = r[j] + t;
            }
        }
    }
}

void invntt_tomont(int16_t r[512]) {
    unsigned int len, start, j, k;
    int16_t t, zeta;
    const int16_t f = 255;
    k = 127;
    for(len = 4; len <= 256; len <<= 1) {
        for(start = 0; start < 512; start = j + len) {
            zeta = zetas[k--];
            for(j = start; j < start + len; j++) {
                t = r[j];
                r[j] = barrett_reduce(t + r[j + len]);
                r[j + len] = r[j + len] - t;
                r[j + len] = fqmul(zeta, r[j + len]);
            }
        }
    }
    for(j = 0; j < 512; j++) r[j] = fqmul(r[j], f);
}

void poly_mul_ntt(int16_t r[512], const int16_t a[512], const int16_t b[512]) {
    for(int i = 0; i < 64; i++) {
        int16_t zeta = zetas[64 + i];
        basemul4(&r[8*i],   &a[8*i],   &b[8*i],   zeta);
        basemul4(&r[8*i+4], &a[8*i+4], &b[8*i+4], -zeta);
    }
}

/* N=768 */
#elif LORE_N == 768
/* Multiplication of polynomials in Zq[X]/(X^6-zeta) */
static void basemul6(int16_t r[6], const int16_t a[6], const int16_t b[6], int16_t zeta) {
    int32_t c[11] = {0};
    for(int i = 0; i < 6; i++) {
        for(int j = 0; j < 6; j++) {
            c[i+j] += (int32_t)fqmul(a[i], b[j]);
        }
    }
    r[0] = (int16_t)(c[0] + fqmul((int16_t)c[6], zeta));
    r[1] = (int16_t)(c[1] + fqmul((int16_t)c[7], zeta));
    r[2] = (int16_t)(c[2] + fqmul((int16_t)c[8], zeta));
    r[3] = (int16_t)(c[3] + fqmul((int16_t)c[9], zeta));
    r[4] = (int16_t)(c[4] + fqmul((int16_t)c[10], zeta));
    r[5] = (int16_t)c[5];
}

void ntt(int16_t r[768]) {
    unsigned int len, start, j, k;
    int16_t t, zeta;
    k = 1;
    for(len = 384; len >= 6; len >>= 1) {
        for(start = 0; start < 768; start = j + len) {
            zeta = zetas[k++];
            for(j = start; j < start + len; j++) {
                t = fqmul(zeta, r[j + len]);
                r[j + len] = r[j] - t;
                r[j] = r[j] + t;
            }
        }
    }
}

void invntt_tomont(int16_t r[768]) {
    unsigned int start, len, j, k;
    int16_t t, zeta;
    const int16_t f = 255;
    k = 127;
    for(len = 6; len <= 384; len <<= 1) {
        for(start = 0; start < 768; start = j + len) {
            zeta = zetas[k--];
            for(j = start; j < start + len; j++) {
                t = r[j];
                r[j] = barrett_reduce(t + r[j + len]);
                r[j + len] = r[j + len] - t;
                r[j + len] = fqmul(zeta, r[j + len]);
            }
        }
    }
    for(j = 0; j < 768; j++) r[j] = fqmul(r[j], f);
}

void poly_mul_ntt(int16_t r[768], const int16_t a[768], const int16_t b[768]) {
    for(int i = 0; i < 64; i++) {
        int16_t zeta = zetas[64 + i];
        basemul6(&r[12*i],   &a[12*i],   &b[12*i],   zeta);
        basemul6(&r[12*i+6], &a[12*i+6], &b[12*i+6], -zeta);
    }
}

/* N=1024 */
#elif LORE_N == 1024
/* q=257 permits 128 degree-eight base rings, not a full negacyclic NTT. */
static void basemul8(int16_t r[8], const int16_t a[8], const int16_t b[8], int16_t zeta) {
    /* Reduce each convolution sum once. The NTT input range keeps the sums
       within montgomery_reduce's 32-bit intermediate bound. */
    int64_t c[15] = {0};
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            c[i + j] += (int32_t)a[i] * b[j];
        }
    }
    for (int i = 0; i < 7; ++i) {
        int16_t low = montgomery_reduce(c[i]);
        int16_t high = montgomery_reduce(c[i + 8]);
        r[i] = (int16_t)(low + fqmul(high, zeta));
    }
    r[7] = montgomery_reduce(c[7]);
}

void ntt(int16_t r[1024]) {
    unsigned int len, start, j, k = 1;
    int16_t t, zeta;
    for (len = 512; len >= 8; len >>= 1) {
        for (start = 0; start < 1024; start = j + len) {
            zeta = zetas[k++];
            for (j = start; j < start + len; ++j) {
                t = fqmul(zeta, r[j + len]);
                r[j + len] = r[j] - t;
                r[j] = r[j] + t;
            }
        }
    }
}

void invntt_tomont(int16_t r[1024]) {
    unsigned int len, start, j, k = 127;
    int16_t t, zeta;
    const int16_t f = 255;
    for (len = 8; len <= 512; len <<= 1) {
        for (start = 0; start < 1024; start = j + len) {
            zeta = zetas[k--];
            for (j = start; j < start + len; ++j) {
                t = r[j];
                r[j] = barrett_reduce(t + r[j + len]);
                r[j + len] = r[j + len] - t;
                r[j + len] = fqmul(zeta, r[j + len]);
            }
        }
    }
    for (j = 0; j < 1024; ++j) r[j] = fqmul(r[j], f);
}

void poly_mul_ntt(int16_t r[1024], const int16_t a[1024], const int16_t b[1024]) {
    for (int i = 0; i < 64; ++i) {
        int16_t zeta = zetas[64 + i];
        basemul8(&r[16 * i], &a[16 * i], &b[16 * i], zeta);
        basemul8(&r[16 * i + 8], &a[16 * i + 8], &b[16 * i + 8], -zeta);
    }
}
#else
#error "Unsupported LORE_N for NTT"
#endif
