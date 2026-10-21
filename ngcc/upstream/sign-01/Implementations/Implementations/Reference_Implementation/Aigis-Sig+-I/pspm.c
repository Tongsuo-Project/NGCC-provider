#include <stdio.h>
#include "pspm.h"

#include <string.h>

#include "polyvec.h"
#include "params.h"
#include "reduce.h"

static inline uint64_t load64(const void *p)
{
    uint64_t x;
    memcpy(&x, p, sizeof(x));
    return x;
}

static inline void store64(void *p, uint64_t x)
{
    memcpy(p, &x, sizeof(x));
}

void emulate_cs1(polyvecl *r, const poly *c, const uint8_t s1_table[PARAM_L][PARAM_N * 3])
{
    uint8_t w[PARAM_N];

    for (int l = 0; l < PARAM_L; l++)
    {
        memset(w, 0, 512);
        for (int i = 0; i < PARAM_N; i++)
        {
            if (c->coeffs[i] != 0)
            {
                const uint8_t *s = s1_table[l] + PARAM_N - i + (PARAM_N & (c->coeffs[i] >> 31));
                for (int j = 0; j < PARAM_N; j+=8)
                {
                    store64(w + j, load64(w + j) + load64(s + j));
                }
            }
        }
        for (int i = 0; i < PARAM_N; i++)
        {
            r->vec[l].coeffs[i] = w[i] - BETA1;
        }
    }
}

int emulate_ct0(polyveck *r, const poly *c, const polyveck *t0)
{
    int32_t stable[PARAM_N * 3];
    int32_t w[PARAM_N];  

    int32_t *stable32;
    int32_t a;

    for (int k = 0; k < PARAM_K; k++)
    {
        memset(w, 0, PARAM_N * 4);
        for (int i = 0; i < PARAM_N; i++)
        {
            stable[i] = -t0->vec[k].coeffs[i];
            stable[PARAM_N + i] = t0->vec[k].coeffs[i];
            stable[PARAM_N * 2 + i] = -t0->vec[k].coeffs[i];
        }

        for (int i = 0; i < PARAM_N; i++)
        {
            if (c->coeffs[i] != 0)
            {
                stable32 = stable + PARAM_N - i + (PARAM_N & (c->coeffs[i] >> 31));
                for (int j = 0; j < PARAM_N; j++)
                {
                    w[j] += stable32[j];
                }
            }
        }

        for (int i = 0; i < PARAM_N; i++)
        {     
            r->vec[k].coeffs[i] = w[i];
        }
    }
    return 0;
}

int emulate_ct1(polyveck *r, const poly *c, const polyveck *t)
{
    int32_t stable[PARAM_N * 3];
    int32_t w[PARAM_N];  

    int32_t *stable32;

    for (int k = 0; k < PARAM_K; k++)
    {
        memset(w, 0, PARAM_N * 4);
        for (int i = 0; i < PARAM_N; i++)
        {
            stable[i] = -t->vec[k].coeffs[i];
            stable[PARAM_N + i] = t->vec[k].coeffs[i];
            stable[PARAM_N * 2 + i] =- t->vec[k].coeffs[i];
        }

        for (int i = 0; i < PARAM_N; i++)
        {
            if (c->coeffs[i] != 0)
            {
                stable32 = stable + PARAM_N - i + (PARAM_N & (c->coeffs[i] >> 31));
                for (int j = 0; j < PARAM_N; j++)
                    w[j] += stable32[j];
            }
        }

        for (int i = 0; i < PARAM_N; i++)
            r->vec[k].coeffs[i] = w[i] ;
    }
    return 0;
}

#if PARAMS == 1

void emulate_cs2(polyveck *r, const poly *c, const uint16_t s2_table[PARAM_K][PARAM_N * 3])
{
    uint16_t w[PARAM_N];
    uint64_t f, e;

    for (int k = 0; k < PARAM_K; k++)
    {
        memset(w, 0, 512 * 2);
        for (int i = 0; i < PARAM_N; i++)
        {
            if (c->coeffs[i] != 0)
            {
                const uint16_t *s = s2_table[k] + PARAM_N - i + (PARAM_N & (c->coeffs[i] >> 31));
                for (int j = 0; j < PARAM_N; j+=4)
                {
                    f = load64(w + j);
                    e = load64(s + j);
                    store64(w + j, f + e);
                }
            }
        }
        for (int i = 0; i < PARAM_N; i++)
        {
            r->vec[k].coeffs[i] = w[i] - BETA2;
        }
    }
}

#else 

void emulate_cs2(polyveck *r, const poly *c, const uint8_t s2_table[PARAM_K][PARAM_N * 3])
{

    uint8_t w[PARAM_N];


    for (int k = 0; k < PARAM_K; k++)
    {
        memset(w, 0, 512);
        for (int i = 0; i < PARAM_N; i++)
        {
            if (c->coeffs[i] != 0)
            {
                const uint8_t* s = s2_table[k] + PARAM_N - i + (PARAM_N & (c->coeffs[i] >> 31));
                for (int j = 0; j < PARAM_N; j+=8)
                    store64(w + j, load64(w + j) + load64(s + j));
            }
        }
        for (int i = 0; i < PARAM_N; i++)
        {
            r->vec[k].coeffs[i] = w[i] - BETA2;
        }
    }
}

#endif