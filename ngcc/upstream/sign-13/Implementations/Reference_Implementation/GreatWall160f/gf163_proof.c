#include "gf163_proof.h"
void qs_gf163_load_witness(const quicksilver_state* s,qs_gf163_bits* out,size_t start) {
    for(size_t i=0;i<163;++i)out->bit[i]=quicksilver_get_witness_vec(s,start+i);
}
void qs_gf163_load_constant(const quicksilver_state* s,qs_gf163_bits* out,const uint64_t a[3]) {
    for(size_t i=0;i<163;++i)out->bit[i]=quicksilver_const_gf2(s,poly1_load(a[i/64],i%64));
}
void qs_gf163_add(const quicksilver_state* s,qs_gf163_bits* out,const qs_gf163_bits* a,const qs_gf163_bits* b) {
    for(size_t i=0;i<163;++i)out->bit[i]=quicksilver_add_gf2(s,a->bit[i],b->bit[i]);
}
void qs_gf163_matmul(const quicksilver_state* s,qs_gf163_bits* out,const uint64_t m[163][3],const qs_gf163_bits* in) {
    qs_gf163_bits result;
    for(size_t i=0;i<163;++i) {
        result.bit[i]=quicksilver_zero_gf2();
        for(size_t j=0;j<163;++j)
            if((m[i][j/64]>>(j%64))&1)
                result.bit[i]=quicksilver_add_gf2(s,result.bit[i],in->bit[j]);
    }
    *out=result;
}

static void mul_constraint(quicksilver_state* s,const qs_gf163_bits* x,const qs_gf163_bits* y,const qs_gf163_bits* z) {
    quicksilver_vec_gfsecpar a[163],b[163];
    quicksilver_vec_deg2 product[325];
    for(size_t i=0;i<163;++i) {
        a[i]=quicksilver_combine_1_bit(s,x->bit[i]);
        b[i]=quicksilver_combine_1_bit(s,y->bit[i]);
    }
    for(size_t i=0;i<325;++i)product[i]=quicksilver_zero_deg2();
    for(size_t i=0;i<163;++i)
        for(size_t j=0;j<163;++j)
            product[i+j]=quicksilver_add_deg2(s,product[i+j],quicksilver_mul(s,a[i],b[j]));
    const unsigned taps[4]={0,3,6,7};
    for(int i=324;i>=163;--i)
        for(size_t j=0;j<4;++j)
            product[i-163+taps[j]]=quicksilver_add_deg2(s,product[i-163+taps[j]],product[i]);
    for(size_t i=0;i<163;++i) {
        quicksilver_vec_deg2 zterm=quicksilver_mul(s,quicksilver_combine_1_bit(s,z->bit[i]),quicksilver_one_gfsecpar(s));
        quicksilver_constraint(s,quicksilver_add_deg2(s,product[i],zterm));
    }
}
void qs_gf163_mul_constraints(quicksilver_state* s,const qs_gf163_bits* x,const qs_gf163_bits* y,const qs_gf163_bits* z,size_t n) {
    for(size_t i=0;i<n;++i)mul_constraint(s,x+i,y+i,z+i);
}
void qs_gf163_dot_constraint(quicksilver_state* s,const qs_gf163_bits* x,const block163* alpha) {
    quicksilver_vec_gf2 dot=quicksilver_zero_gf2();
    for(size_t i=0;i<163;++i)
        dot=quicksilver_add_gf2(s,dot,quicksilver_mul_const_gf2(s,x->bit[i],poly1_load(alpha->data[i/64],i%64)));
    quicksilver_inverse_constraint(s,quicksilver_combine_1_bit(s,dot),quicksilver_one_gfsecpar(s));
}
