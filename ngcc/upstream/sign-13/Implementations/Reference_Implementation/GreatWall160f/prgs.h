#ifndef PRGS_H
#define PRGS_H
#include "aes.h"
#include "hash.h"

#define DEFINE_REFERENCE_PRG(name) \
typedef aes_round_keys prg_##name##_key; \
typedef block128 prg_##name##_iv; \
typedef block128 prg_##name##_block; \
typedef char prg_##name##_fixed_key; \
static inline void prg_##name##_init(prg_##name##_key* prgs,const prg_##name##_fixed_key* fixed,const block_secpar* keys,const prg_##name##_iv* ivs,size_t n,uint32_t blocks,uint32_t counter,prg_##name##_block* out) { \
    (void)fixed; \
    for(size_t i=0;i<n;++i) { aes_keygen(prgs+i,keys[i]); prgs[i].iv=ivs[i]; \
        for(uint32_t j=0;j<blocks;++j) aes_encrypt_block_portable(prgs+i,aes_add_counter_to_iv_portable(ivs[i],(uint64_t)counter+j),out+i*blocks+j); } \
} \
static inline void prg_##name##_gen(const prg_##name##_key* prgs,const prg_##name##_fixed_key* fixed,size_t n,uint32_t blocks,uint32_t counter,prg_##name##_block* out) { \
    (void)fixed; \
    for(size_t i=0;i<n;++i)for(uint32_t j=0;j<blocks;++j) \
        aes_encrypt_block_portable(prgs+i,aes_add_counter_to_iv_portable(prgs[i].iv,(uint64_t)counter+j),out+i*blocks+j); \
}
#define PRG_VOLE_PREFERRED_WIDTH AES_PREFERRED_WIDTH
#define PRG_VOLE_PREFERRED_WIDTH_SHIFT AES_PREFERRED_WIDTH_SHIFT
#define PRG_TREE_PREFERRED_WIDTH AES_PREFERRED_WIDTH
#define PRG_TREE_PREFERRED_WIDTH_SHIFT AES_PREFERRED_WIDTH_SHIFT
DEFINE_REFERENCE_PRG(vole)
DEFINE_REFERENCE_PRG(tree)
#undef DEFINE_REFERENCE_PRG
typedef char prg_leaf_key;
typedef block128 prg_leaf_iv;
typedef block_secpar prg_leaf_block;
typedef char prg_leaf_fixed_key;
#define PRG_LEAF_PREFERRED_WIDTH_SHIFT 3
#define PRG_LEAF_PREFERRED_WIDTH (1 << PRG_LEAF_PREFERRED_WIDTH_SHIFT)
static inline void prg_leaf_init(prg_leaf_key* prgs,const prg_leaf_fixed_key* fixed,const block_secpar* keys,const prg_leaf_iv* ivs,size_t n,uint32_t blocks,uint32_t counter,prg_leaf_block* out) {
    (void)prgs; (void)fixed; assert(counter==0); (void)counter;
    shake_prg(keys,ivs,n,blocks*sizeof(block_secpar),(uint8_t*)out);
}
static inline void prg_leaf_gen(const prg_leaf_key* prgs,const prg_leaf_fixed_key* fixed,size_t n,uint32_t blocks,uint32_t counter,prg_leaf_block* out) {
    (void)prgs;(void)fixed;(void)counter;(void)out;(void)n;(void)blocks;
    assert(n==0||blocks==0);
}
static inline void init_fixed_keys(prg_tree_fixed_key* tree,prg_leaf_fixed_key* leaf,block_secpar iv) {
    (void)tree;(void)leaf;(void)iv;
}
#endif
