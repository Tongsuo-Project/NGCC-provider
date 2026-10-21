#ifndef GF163_H
#define GF163_H
#include <assert.h>
#include <stdbool.h>
#include "reference_arithmetic.h"
#define BLOCK163_BITS 163
#define BLOCK163_WORDS 3
#define BLOCK163_BYTES 21
#define BLOCK163_TOP_MASK UINT64_C(0x7ffffffff)
#define BLOCK163_REDUCTION_LOW UINT64_C(0xc9)
typedef struct { uint64_t data[3]; } block163;
static inline void block163_canonicalize(block163* a) { a->data[2]&=BLOCK163_TOP_MASK; }
static inline void block163_mask(block163* a) { block163_canonicalize(a); }
static inline bool block163_is_canonical(const block163* a) { return !(a->data[2]&~BLOCK163_TOP_MASK); }
static inline void block163_load_to_block(block163* a,const uint8_t* p)
{ assert(!(p[20]&0xf8)); ref_load_le(a->data,3,p,21); block163_canonicalize(a); }
static inline void block163_store_from_block(uint8_t* p,const block163* a)
{ assert(block163_is_canonical(a)); ref_store_le(p,a->data,21); }
static inline void block163_load_words(block163* a,const uint64_t w[3])
{ memmove(a->data,w,sizeof(a->data)); block163_canonicalize(a); }
static inline block163 block163_xor(block163 a,block163 b)
{ for(size_t i=0;i<3;++i) a.data[i]^=b.data[i]; return a; }
static inline block163 block163_set_one(void) { block163 a={{1,0,0}}; return a; }
static inline bool block163_is_zero(const block163* a)
{ return !(a->data[0]|a->data[1]|a->data[2]); }

static inline unsigned block163_parity64(uint64_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return (unsigned)__builtin_parityll(x);
#else
    x^=x>>32; x^=x>>16; x^=x>>8; x^=x>>4;
    return (0x6996u>>(x&15))&1;
#endif
}
static inline bool block163_dot_is_one(const block163* a,const block163* b)
{
    return block163_parity64((a->data[0]&b->data[0]) ^
        (a->data[1]&b->data[1]) ^ (a->data[2]&b->data[2]&BLOCK163_TOP_MASK))!=0;
}
static inline void block163_mul(block163* out,const block163* a,const block163* b)
{
    uint64_t product[6];
    ref_product(product,6,a->data,163,b->data,163);

    for(int i=324;i>=163;--i) if(ref_bit(product,(size_t)i)) {
        ref_toggle(product,(size_t)i);
        ref_toggle(product,(size_t)(i-163));
        ref_toggle(product,(size_t)(i-163+3));
        ref_toggle(product,(size_t)(i-163+6));
        ref_toggle(product,(size_t)(i-163+7));
    }
    memcpy(out->data,product,sizeof(out->data));
    block163_canonicalize(out);
}
static inline void block163_sqr(block163* out,const block163* a) { block163_mul(out,a,a); }
static inline void block163_square_n(block163* out,const block163* a,unsigned n)
{ *out=*a; for(unsigned i=0;i<n;++i) block163_sqr(out,out); }
static inline void block163_pow_words(block163* a,const uint64_t exponent[3])
{
    block163 x=*a,r=block163_set_one();
    for(size_t i=163;i-- > 0;) {
        block163_sqr(&r,&r);
        if(ref_bit(exponent,i)) block163_mul(&r,&r,&x);
    }
    *a=r;
}
static inline void block163_mersenne_inverse1(block163* a)
{
    const uint64_t e[3]={UINT64_C(0xef7bdef7bdef7bdd),UINT64_C(0xef7bdef7bdeef7bd),UINT64_C(0x77bdef7bd)};
    block163_pow_words(a,e);
}
static inline void block163_mersenne_inverse2(block163* a)
{
    const uint64_t e[3]={UINT64_C(0xf77bbddeef77bbdd),UINT64_C(0x77bbddeef77bddee),UINT64_C(0x77bbddeef)};
    block163_pow_words(a,e);
}
static inline void block163_multiply_with_GF2_matrix(block163* a,const uint64_t m[163][3])
{
    block163 out={{0}};
    for(size_t r=0;r<163;++r) {
        unsigned bit=block163_parity64((m[r][0]&a->data[0]) ^
            (m[r][1]&a->data[1]) ^ (m[r][2]&a->data[2]&BLOCK163_TOP_MASK));
        out.data[r/64]|=(uint64_t)bit<<(r%64);
    }
    *a=out;
}
#endif
