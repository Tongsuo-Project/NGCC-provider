/* Copyright (c) 2026 Hang Zhang. */
#ifndef BIT_CT_AUDIT_H
#define BIT_CT_AUDIT_H

/* TIMECOP: BIT_TIMECOP exposes Gaussian acceptance and key-validity flags,
 * assuming independent candidates and valid generated signing keys.
 * BIT_TIMECOP_HAETAE also exposes candidate challenges, responses and hints,
 * including candidates rejected later. These assumptions need distribution proofs.
 * See https://eprint.iacr.org/2024/2077, section 9.5.
 * Enable with -DBIT_TIMECOP=1 and, optionally, -DBIT_TIMECOP_HAETAE=1. */
#ifndef BIT_TIMECOP
#define BIT_TIMECOP 0
#endif
#ifndef BIT_TIMECOP_HAETAE
#define BIT_TIMECOP_HAETAE 0
#endif
#if BIT_TIMECOP_HAETAE && !BIT_TIMECOP
#error "BIT_TIMECOP_HAETAE requires BIT_TIMECOP"
#endif

#if BIT_TIMECOP
#include <valgrind/memcheck.h>
#define BIT_CT_DECLASSIFY(p, n) ((void)VALGRIND_MAKE_MEM_DEFINED((p), (n)))

static inline int bit_ct_gaussian_accept(int accepted)
{
    BIT_CT_DECLASSIFY(&accepted, sizeof accepted);
    return accepted;
}
#define BIT_CT_GAUSSIAN_ACCEPT(value) bit_ct_gaussian_accept(value)
#else
#define BIT_CT_DECLASSIFY(p, n) ((void)0)
#define BIT_CT_GAUSSIAN_ACCEPT(value) (value)
#endif

#if BIT_TIMECOP_HAETAE
#define BIT_CT_HAETAE_PUBLIC(p, n) BIT_CT_DECLASSIFY(p, n)
#else
#define BIT_CT_HAETAE_PUBLIC(p, n) ((void)0)
#endif
#endif
