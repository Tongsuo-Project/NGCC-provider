#ifndef GF163_PROOF_H
#define GF163_PROOF_H
#include "quicksilver.h"
#include "gf163.h"
typedef struct { quicksilver_vec_gf2 bit[163]; } qs_gf163_bits;
void qs_gf163_load_witness(const quicksilver_state*,qs_gf163_bits*,size_t);
void qs_gf163_load_constant(const quicksilver_state*,qs_gf163_bits*,const uint64_t[3]);
void qs_gf163_add(const quicksilver_state*,qs_gf163_bits*,const qs_gf163_bits*,const qs_gf163_bits*);
void qs_gf163_matmul(const quicksilver_state*,qs_gf163_bits*,const uint64_t[163][3],const qs_gf163_bits*);
void qs_gf163_mul_constraints(quicksilver_state*,const qs_gf163_bits*,const qs_gf163_bits*,const qs_gf163_bits*,size_t);
void qs_gf163_dot_constraint(quicksilver_state*,const qs_gf163_bits*,const block163*);
#endif
