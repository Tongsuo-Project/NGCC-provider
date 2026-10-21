#ifndef GREATWALL160_REFERENCE_ARITHMETIC_H
#define GREATWALL160_REFERENCE_ARITHMETIC_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static inline void ref_clmul64(uint64_t out[2], uint64_t a, uint64_t b)
{
    uint64_t lo=0, hi=0;
    for(unsigned i=0;i<64;++i) {
        uint64_t mask=UINT64_C(0)-((b>>i)&1);
        lo^=(a<<i)&mask;
        if(i) hi^=(a>>(64-i))&mask;
    }
    out[0]=lo; out[1]=hi;
}
static inline unsigned ref_bit(const uint64_t* a, size_t i)
{ return (unsigned)((a[i / 64] >> (i % 64)) & 1); }
static inline void ref_toggle(uint64_t* a, size_t i)
{ a[i / 64] ^= UINT64_C(1) << (i % 64); }
static inline void ref_shift_xor(uint64_t* out, size_t words,
                                  const uint64_t* a, size_t a_words, size_t shift)
{
    size_t offset=shift/64; unsigned bits=(unsigned)(shift%64);
    for(size_t j=0;j<a_words;++j) {
        if(offset+j<words) out[offset+j]^=a[j]<<bits;
        if(bits && offset+j+1<words) out[offset+j+1]^=a[j]>>(64-bits);
    }
}
static inline void ref_product(uint64_t* out, size_t words,
    const uint64_t* a, size_t a_bits, const uint64_t* b, size_t b_bits)
{
    memset(out,0,words*sizeof(*out));
    for(size_t i=0;i<b_bits;++i)
        if(ref_bit(b,i)) ref_shift_xor(out,words,a,(a_bits+63)/64,i);
}
static inline void ref_reduce(uint64_t* p, size_t bits, size_t degree, uint64_t low)
{
    for(size_t i=bits;i-- > degree;) if(ref_bit(p,i)) {
        ref_toggle(p,i);
        for(size_t j=0;j<64;++j) if((low>>j)&1) ref_toggle(p,i-degree+j);
    }
}
static inline void ref_load_le(uint64_t* words,size_t nwords,const void* src,size_t bytes)
{
    const uint8_t* p=src; memset(words,0,nwords*sizeof(*words));
    for(size_t i=0;i<bytes;++i) words[i/8]|=(uint64_t)p[i]<<(8*(i%8));
}
static inline void ref_store_le(void* dst,const uint64_t* words,size_t bytes)
{
    uint8_t* p=dst;
    for(size_t i=0;i<bytes;++i) p[i]=(uint8_t)(words[i/8]>>(8*(i%8)));
}
#endif
