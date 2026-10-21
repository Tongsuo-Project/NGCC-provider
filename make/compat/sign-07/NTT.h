/* Outer adapter only. Author ntt.c includes "NTT.h"; the attested source
 * file is ntt.h. MultiplyNTT is declared there as an external function but
 * defined static in ntt.c. Hide the unused declaration without editing
 * author sources. Include path: -Imake/compat/sign-07 before the ref tree. */
#define MultiplyNTT MultiplyNTT_unused_decl
#include "ntt.h"
#undef MultiplyNTT
