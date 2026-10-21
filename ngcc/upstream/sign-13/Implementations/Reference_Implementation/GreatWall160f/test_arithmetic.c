/* Independent bit-polynomial oracle for the scalar authentication backend. */
#include "polynomials.h"
#include <stdio.h>
#include <stdlib.h>

static uint64_t test_rng=UINT64_C(0xf5b19d42706e3ac1);
static uint64_t next_word(void) {
    test_rng^=test_rng<<13; test_rng^=test_rng>>7; test_rng^=test_rng<<17;
    return test_rng;
}
static void check160(poly160_vec a,poly160_vec b) {
    uint64_t expected[5];
    ref_product(expected,5,a.data,160,b.data,160);
    poly320_vec p=poly160_mul(a,b);
    assert(!memcmp(p.data,expected,sizeof(expected)));
    ref_reduce(expected,320,160,GF160_REDUCTION_LOW);
    poly160_vec r=poly320_reduce160(p);
    assert(!memcmp(r.data,expected,sizeof(r.data)));
    assert(!(r.data[2]>>32));
}
int main(void) {
    poly160_vec zero={{0}},one={{1,0,0}};
    poly160_vec all={{UINT64_MAX,UINT64_MAX,UINT32_MAX}};
    check160(zero,zero); check160(zero,all); check160(one,all); check160(all,all);
    for(size_t i=0;i<160;++i)for(size_t j=0;j<160;++j) {
        poly160_vec a={{0}},b={{0}};
        ref_toggle(a.data,i); ref_toggle(b.data,j); check160(a,b);
    }
    for(size_t i=0;i<10000;++i) {
        poly160_vec a={{next_word(),next_word(),next_word()&UINT32_MAX}};
        poly160_vec b={{next_word(),next_word(),next_word()&UINT32_MAX}};
        check160(a,b);
        poly64_vec u={{next_word()}},v={{next_word()}};
        uint64_t oracle64[2]; ref_product(oracle64,2,u.data,64,v.data,64);
        poly128_vec p64=poly64_mul(u,v);
        assert(!memcmp(p64.data,oracle64,sizeof(oracle64)));
        p64.data[0]=next_word(); p64.data[1]=next_word();
        memcpy(oracle64,p64.data,sizeof(oracle64)); ref_reduce(oracle64,128,64,0x1b);
        assert(poly128_reduce64(p64).data[0]==oracle64[0]);
        poly224_vec p224=poly64x160_mul(u,a);
        uint64_t oracle224[4]; ref_product(oracle224,4,u.data,64,a.data,160);
        assert(!memcmp(p224.data,oracle224,sizeof(oracle224)));
        for(size_t j=0;j<4;++j)p224.data[j]=next_word();
        p224.data[3]&=UINT32_MAX;
        memcpy(oracle224,p224.data,sizeof(oracle224));
        ref_reduce(oracle224,224,160,GF160_REDUCTION_LOW);
        poly160_vec r224=poly224_reduce160(p224);
        assert(!memcmp(r224.data,oracle224,sizeof(r224.data)));
        poly320_vec wide; for(size_t j=0;j<5;++j)wide.data[j]=next_word();
        uint64_t oracle320[5]; memcpy(oracle320,wide.data,sizeof(oracle320));
        ref_reduce(oracle320,320,160,GF160_REDUCTION_LOW);
        poly160_vec reduced=poly320_reduce160(wide);
        assert(!memcmp(reduced.data,oracle320,sizeof(reduced.data)));
        if(i<1000) {
            block163 x={{next_word(),next_word(),next_word()&BLOCK163_TOP_MASK}};
            block163 y={{next_word(),next_word(),next_word()&BLOCK163_TOP_MASK}};
            uint64_t expected[6]; ref_product(expected,6,x.data,163,y.data,163);
            ref_reduce(expected,326,163,BLOCK163_REDUCTION_LOW);
            block163 product; block163_mul(&product,&x,&y);
            assert(!memcmp(product.data,expected,sizeof(product.data)));
            block163 alias=x; block163_mul(&alias,&alias,&y);
            assert(!memcmp(alias.data,expected,sizeof(alias.data)));
            ref_product(expected,6,x.data,163,x.data,163);
            ref_reduce(expected,326,163,BLOCK163_REDUCTION_LOW);
            alias=x; block163_sqr(&alias,&alias);
            assert(!memcmp(alias.data,expected,sizeof(alias.data)));
            unsigned dot=0;
            for(size_t j=0;j<163;++j)dot^=ref_bit(x.data,j)&ref_bit(y.data,j);
            assert(block163_dot_is_one(&x,&y)==(dot!=0));
        }
    }
    uint64_t matrix[163][3];
    block163 input={{next_word(),next_word(),next_word()&BLOCK163_TOP_MASK}};
    block163 expected={{0}},actual=input;
    for(size_t i=0;i<163;++i) {
        for(size_t j=0;j<3;++j)matrix[i][j]=next_word();
        matrix[i][2]&=BLOCK163_TOP_MASK;
        unsigned bit=0;
        for(size_t j=0;j<163;++j)bit^=ref_bit(matrix[i],j)&ref_bit(input.data,j);
        expected.data[i/64]|=(uint64_t)bit<<(i%64);
    }
    block163_multiply_with_GF2_matrix(&actual,(const uint64_t (*)[3])matrix);
    assert(!memcmp(actual.data,expected.data,sizeof(actual.data)));
    puts("PASS: 25600 GF160 basis products; 10000 randomized GF64/GF160 cases; 1000 GF163 products/squares/alias/dot cases; GF163 matrix parity");
    return 0;
}
