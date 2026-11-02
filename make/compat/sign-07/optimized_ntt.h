/* The author's ntt.c defines MultiplyNTT as static, while ntt.h declares an
 * unused external function of that name. Rename only that unused declaration
 * in this translation unit. The author's static implementation is unchanged. */
#define MultiplyNTT ngcc_unused_external_MultiplyNTT
#include NGCC_CS_NTT_HEADER
#undef MultiplyNTT
