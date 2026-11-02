/* Platform allocation adapter, not a cryptographic implementation.
 * Darwin's aligned_alloc rejects alignments below sizeof(void *), including
 * the four-byte alignment of the authors' 160-bit block types. */
#ifndef NGCC_AUTHOR_ALLOC_H
#define NGCC_AUTHOR_ALLOC_H
#if defined(__APPLE__)
#include <cstdlib>
#include <cerrno>
static inline void *ngcc_author_aligned_alloc(size_t alignment, size_t size)
{
    void *ptr = nullptr;
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        errno = EINVAL;
        return nullptr;
    }
    if (alignment < sizeof(void *)) alignment = sizeof(void *);
    const int error = posix_memalign(&ptr, alignment, size);
    if (error != 0) { errno = error; return nullptr; }
    return ptr;
}
#define aligned_alloc ngcc_author_aligned_alloc
#endif
#endif
