/*
 * Platform compatibility shims for the NGCC provider.
 *
 * explicit_bzero() is a glibc/BSD extension not exported by every libc.
 * A weak definition lets the module link everywhere; on platforms where libc
 * provides it the library definition wins.
 */
#include <stddef.h>

__attribute__((weak))
void explicit_bzero(void *dst, size_t len)
{
    volatile unsigned char *p = (volatile unsigned char *)dst;

    if (dst == NULL)
        return;
    while (len-- > 0)
        *p++ = 0;
}