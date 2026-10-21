#ifndef GREATWALL160_POLYNOMIALS_H
#define GREATWALL160_POLYNOMIALS_H
#include <assert.h>
#include <stdbool.h>
#include "block.h"
#include "reference_arithmetic.h"
#define POLY_VEC_LEN 1
#define GF160_REDUCTION_LOW UINT32_C(0x2d)
typedef uint8_t poly1_vec;
static inline poly1_vec poly1_set_all(uint8_t x) { return x; }
static inline poly1_vec poly1_load(uint64_t x, unsigned int i) { return (uint8_t)(0u - ((x >> i) & 1u)); }
static inline poly1_vec poly1_load_offset8(const void* p, size_t i) { return poly1_load(((const uint8_t*)p)[i/8], i%8); }
#define REF_POLY(N,W) \
typedef struct { uint64_t data[W]; } poly##N##_vec; \
static inline poly##N##_vec poly##N##_set_zero(void) { poly##N##_vec x={{0}}; return x; } \
static inline poly##N##_vec poly##N##_load(const void* p) { poly##N##_vec x; ref_load_le(x.data,W,p,N/8); return x; } \
static inline poly##N##_vec poly##N##_load_dup(const void* p) { return poly##N##_load(p); } \
static inline void poly##N##_store(void* p,poly##N##_vec x) { ref_store_le(p,x.data,N/8); } \
static inline void poly##N##_store1(void* p,poly##N##_vec x) { poly##N##_store(p,x); } \
static inline poly##N##_vec poly##N##_set_low32(uint32_t v) { poly##N##_vec x={{0}}; x.data[0]=v; return x; } \
static inline poly##N##_vec poly##N##_add(poly##N##_vec x,poly##N##_vec y) { for(size_t i=0;i<W;++i)x.data[i]^=y.data[i]; return x; } \
static inline bool poly##N##_eq(poly##N##_vec x,poly##N##_vec y) { for(size_t i=0;i<W;++i)if(x.data[i]!=y.data[i])return false; return true; } \
static inline poly##N##_vec poly##N##_extract(poly##N##_vec x,size_t i) { assert(i==0); (void)i; return x; }
REF_POLY(64,1)
REF_POLY(128,2)
REF_POLY(160,3)
REF_POLY(224,4)
REF_POLY(320,5)
#undef REF_POLY
static inline poly128_vec poly64_mul(poly64_vec a, poly64_vec b) {
    poly128_vec p; ref_clmul64(p.data,a.data[0],b.data[0]); return p;
}

static inline poly320_vec poly160_mul(poly160_vec a, poly160_vec b) {
    uint64_t d0[2],d1[2],d2[2],c01[2],c02[2],c12[2];
    assert(!(a.data[2]>>32) && !(b.data[2]>>32));
    ref_clmul64(d0,a.data[0],b.data[0]);
    ref_clmul64(d1,a.data[1],b.data[1]);
    ref_clmul64(d2,a.data[2],b.data[2]);
    ref_clmul64(c01,a.data[0]^a.data[1],b.data[0]^b.data[1]);
    ref_clmul64(c02,a.data[0]^a.data[2],b.data[0]^b.data[2]);
    ref_clmul64(c12,a.data[1]^a.data[2],b.data[1]^b.data[2]);
    for(size_t i=0;i<2;++i) {
        c01[i]^=d0[i]^d1[i];
        c02[i]^=d0[i]^d2[i];
        c12[i]^=d1[i]^d2[i];
    }
    poly320_vec p={{d0[0],d0[1]^c01[0],
        d1[0]^c01[1]^c02[0],d1[1]^c02[1]^c12[0],d2[0]^c12[1]}};
    return p;
}
static inline poly224_vec poly64x160_mul(poly64_vec a, poly160_vec b) {
    uint64_t p0[2],p1[2],p2[2];
    ref_clmul64(p0,a.data[0],b.data[0]);
    ref_clmul64(p1,a.data[0],b.data[1]);
    ref_clmul64(p2,a.data[0],b.data[2]);
    poly224_vec p={{p0[0],p0[1]^p1[0],p1[1]^p2[0],p2[1]}};
    return p;
}
static inline poly64_vec poly128_reduce64(poly128_vec p) {

    uint64_t h=p.data[1];
    uint64_t spill=(h>>63)^(h>>61)^(h>>60);
    poly64_vec r={{p.data[0]^h^(h<<1)^(h<<3)^(h<<4)
        ^spill^(spill<<1)^(spill<<3)^(spill<<4)}};
    return r;
}
static inline poly64_vec poly64_exp(poly64_vec x,size_t e) {
    poly64_vec r=poly64_set_low32(1);
    while(e) { if(e&1)r=poly128_reduce64(poly64_mul(r,x)); e>>=1; x=poly128_reduce64(poly64_mul(x,x)); }
    return r;
}
static inline poly160_vec poly320_reduce160(poly320_vec p) {

    uint64_t h0=(p.data[2]>>32)|(p.data[3]<<32);
    uint64_t h1=(p.data[3]>>32)|(p.data[4]<<32);
    uint64_t h2=p.data[4]>>32;
    poly160_vec r={{p.data[0]^h0^(h0<<2)^(h0<<3)^(h0<<5),
        p.data[1]^h1^(h1<<2)^(h1<<3)^(h1<<5)^(h0>>62)^(h0>>61)^(h0>>59),
        (p.data[2]&UINT32_MAX)^h2^(h2<<2)^(h2<<3)^(h2<<5)
            ^(h1>>62)^(h1>>61)^(h1>>59)}};
    uint64_t spill=r.data[2]>>32;
    r.data[2]&=UINT32_MAX;
    r.data[0]^=spill^(spill<<2)^(spill<<3)^(spill<<5);
    return r;
}
static inline poly160_vec poly224_reduce160(poly224_vec p) {
    poly320_vec wide={{p.data[0],p.data[1],p.data[2],p.data[3],0}};
    return poly320_reduce160(wide);
}
static inline poly128_vec poly128_from_64(poly64_vec x) { poly128_vec r={{x.data[0],0}}; return r; }
static inline poly64_vec poly64_mul_a64_reduce64(poly64_vec x) { poly128_vec r={{0,x.data[0]}}; return poly128_reduce64(r); }
static inline poly160_vec poly160_from_64(poly64_vec x) { poly160_vec r={{x.data[0],0,0}}; return r; }
static inline poly160_vec poly160_from_1(poly1_vec x) { return poly160_set_low32(x&1); }
static inline poly224_vec poly224_from_160(poly160_vec x) { poly224_vec r={{x.data[0],x.data[1],x.data[2],0}}; return r; }
static inline poly320_vec poly320_from_160(poly160_vec x) { poly320_vec r={{x.data[0],x.data[1],x.data[2],0,0}}; return r; }
static inline poly160_vec poly1x160_mul(poly1_vec x,poly160_vec y) { return x ? y : poly160_set_zero(); }
static inline poly320_vec poly1x320_mul(poly1_vec x,poly320_vec y) { return x ? y : poly320_set_zero(); }
static inline poly320_vec poly320_shift(poly320_vec x,unsigned int n) {
    for(size_t i=4;i>0;--i)x.data[i]=(x.data[i]<<n)|(x.data[i-1]>>(64-n));
    x.data[0]<<=n; return x;
}
static inline poly320_vec poly320_shift_left_1(poly320_vec x) { return poly320_shift(x,1); }
static inline poly320_vec poly320_shift_left_8(poly320_vec x) { return poly320_shift(x,8); }
static inline poly160_vec poly160_pack_bits(const poly1_vec* p,size_t n) {
    poly160_vec r={{0}}; assert(n<=160);
    for(size_t i=0;i<n;++i)if(p[i]&1)ref_toggle(r.data,i);
    return r;
}
static inline poly160_vec poly160_pack_coefficients(const poly160_vec* p,size_t n) {
    poly320_vec r={{0}}; assert(n<=160);
    for(size_t i=0;i<n;++i)ref_shift_xor(r.data,5,p[i].data,3,i);
    return poly320_reduce160(r);
}
static inline poly160_vec poly160_exp(poly160_vec x,size_t exponent) {
    poly160_vec r=poly160_set_low32(1);
    while(exponent) { if(exponent&1)r=poly320_reduce160(poly160_mul(r,x)); exponent>>=1; x=poly320_reduce160(poly160_mul(x,x)); }
    return r;
}
typedef poly160_vec poly_secpar_vec;
typedef poly224_vec poly_secpar_plus_64_vec;
typedef poly320_vec poly_2secpar_vec;
#define poly_secpar_load poly160_load
#define poly_secpar_load_dup poly160_load_dup
#define poly_secpar_store poly160_store
#define poly_secpar_store1 poly160_store1
#define poly_secpar_set_zero poly160_set_zero
#define poly_secpar_set_low32 poly160_set_low32
#define poly_secpar_add poly160_add
#define poly_secpar_eq poly160_eq
#define poly_secpar_extract poly160_extract
#define poly_secpar_plus_64_load poly224_load
#define poly_secpar_plus_64_load_dup poly224_load_dup
#define poly_secpar_plus_64_store poly224_store
#define poly_secpar_plus_64_store1 poly224_store1
#define poly_secpar_plus_64_set_zero poly224_set_zero
#define poly_secpar_plus_64_set_low32 poly224_set_low32
#define poly_secpar_plus_64_add poly224_add
#define poly_secpar_plus_64_eq poly224_eq
#define poly_secpar_plus_64_extract poly224_extract
#define poly_2secpar_load poly320_load
#define poly_2secpar_load_dup poly320_load_dup
#define poly_2secpar_store poly320_store
#define poly_2secpar_store1 poly320_store1
#define poly_2secpar_set_zero poly320_set_zero
#define poly_2secpar_set_low32 poly320_set_low32
#define poly_2secpar_add poly320_add
#define poly_2secpar_eq poly320_eq
#define poly_2secpar_extract poly320_extract
#define poly_secpar_mul poly160_mul
#define poly1xsecpar_mul poly1x160_mul
#define poly1x2secpar_mul poly1x320_mul
#define poly64xsecpar_mul poly64x160_mul
#define poly_2secpar_shift_left_1 poly320_shift_left_1
#define poly_2secpar_shift_left_8 poly320_shift_left_8
#define poly_2secpar_reduce_secpar poly320_reduce160
#define poly_secpar_plus_64_reduce_secpar poly224_reduce160
#define poly_secpar_plus_64_from_secpar poly224_from_160
#define poly_2secpar_from_secpar poly320_from_160
#define poly_secpar_from_64 poly160_from_64
#define poly_secpar_from_1 poly160_from_1
#define poly_secpar_from_byte poly160_set_low32
#define poly_secpar_exp poly160_exp
static inline poly160_vec poly_secpar_from_secpar_poly1(const poly1_vec* p) { return poly160_pack_bits(p,160); }
static inline poly160_vec poly_secpar_from_8_poly1(const poly1_vec* p) { return poly160_pack_bits(p,8); }
static inline poly160_vec poly_secpar_from_16_poly1(const poly1_vec* p) { return poly160_pack_bits(p,16); }
static inline poly160_vec poly_secpar_from_secpar_poly_secpar(const poly160_vec* p) { return poly160_pack_coefficients(p,160); }
static inline poly160_vec poly_secpar_from_8_poly_secpar(const poly160_vec* p) { return poly160_pack_coefficients(p,8); }
static inline poly160_vec poly_secpar_from_16_poly_secpar(const poly160_vec* p) { return poly160_pack_coefficients(p,16); }
static inline poly160_vec poly_secpar_from_8_poly64(const poly64_vec* p) {
    poly160_vec lifted[8]; for(size_t i=0;i<8;++i)lifted[i]=poly160_from_64(p[i]); return poly160_pack_coefficients(lifted,8);
}
#endif
