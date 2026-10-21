#ifndef GREATWALL160_REFERENCE_BLOCK_H
#define GREATWALL160_REFERENCE_BLOCK_H
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "config.h"
#include "gf163.h"
#include "util.h"
typedef struct { uint64_t data[2]; } block128;
typedef struct { uint64_t data[3]; } block192;
typedef struct { uint64_t data[4]; } block256;
typedef struct { block128 data[3]; } block384;
typedef struct { block256 data[2]; } block512;
typedef struct { block256 data[4]; } block1024;

typedef struct { uint32_t data[5]; } block160;
typedef struct { uint32_t data[10]; } block320;
typedef block160 block_secpar;
typedef block320 block_2secpar;
#define BLOCK_SECPAR_BYTES 20
#define BLOCK_2SECPAR_BYTES 40
#define BLOCK_SECPAR_LEN 2
#define BLOCK_2SECPAR_LEN 3
#define VOLE_BLOCK_SHIFT 0
#define VOLE_BLOCK 1
#define POLY_VEC_LEN_SHIFT 0
typedef block128 vole_block;
GW_STATIC_ASSERT(sizeof(block_secpar)==20,"seed/MAC stride");
GW_STATIC_ASSERT(sizeof(block_2secpar)==40,"double-width stride");
#define REF_BLOCK_OPS(name,wordtype,n) \
static inline name name##_xor(name a,name b) { for(size_t i=0;i<n;++i) a.data[i]^=b.data[i]; return a; } \
static inline name name##_and(name a,name b) { for(size_t i=0;i<n;++i) a.data[i]&=b.data[i]; return a; } \
static inline name name##_set_zero(void) { name a={{0}}; return a; } \
static inline name name##_set_all_8(uint8_t b) { name a; memset(&a,b,sizeof(a)); return a; } \
static inline name name##_set_low32(uint32_t b) { name a={{0}}; a.data[0]=b; return a; } \
static inline name name##_set_low64(uint64_t b) { name a={{0}}; uint8_t bytes[8]; ref_store_le(bytes,&b,8); memcpy(&a,bytes,8); return a; } \
static inline bool name##_any_zeros(name a) { const uint8_t* p=(const uint8_t*)&a; for(size_t i=0;i<sizeof(a);++i) if(!p[i]) return true; return false; }
REF_BLOCK_OPS(block128,uint64_t,2)
REF_BLOCK_OPS(block192,uint64_t,3)
REF_BLOCK_OPS(block256,uint64_t,4)
REF_BLOCK_OPS(block160,uint32_t,5)
REF_BLOCK_OPS(block320,uint32_t,10)
REF_BLOCK_OPS(block_secpar,uint32_t,5)
REF_BLOCK_OPS(block_2secpar,uint32_t,10)
REF_BLOCK_OPS(vole_block,uint64_t,2)
#undef REF_BLOCK_OPS
static inline block256 block256_set_128(block128 x,block128 y)
{ block256 z={{x.data[0],x.data[1],y.data[0],y.data[1]}}; return z; }
static inline block256 block256_set_low128(block128 x) { return block256_set_128(x,block128_set_zero()); }
static inline block256 block256_from_2_block128(block128 x,block128 y) { return block256_set_128(x,y); }
static inline block128 block128_byte_reverse(block128 x)
{ block128 z; const uint8_t* a=(const uint8_t*)&x; uint8_t* b=(uint8_t*)&z; for(size_t i=0;i<16;++i) b[i]=a[15-i]; return z; }
#endif
