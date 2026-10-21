/*
 * instances.h — GALAS instance parameter sets (BAVC/VOLEitH-relevant fields).
 *
 * Mirrors the subset of FAEST's faest_param_t that the BAVC/VOLE/QuickSilver
 * machinery needs. Derivations match FAEST (instances.c):
 *   k    = (lambda - w_grind) / tau + 1
 *   tau1 = (lambda - w_grind) % tau
 *   tau0 = tau - tau1
 *   L    = tau1 * (1<<k) + tau0 * (1<<(k-1))   // total leaves across tau trees
 *
 * The 8 instances {galas-160,256,384,512}{s,f} carry the same
 * (tau, T_open, w_grind) triples and FAEST-style key/signature sizes as the
 * optimized implementation.
 */
#ifndef GALAS_INSTANCES_H
#define GALAS_INSTANCES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define GALAS_MAX_LAMBDA 512
#define GALAS_MAX_LAMBDA_BYTES (GALAS_MAX_LAMBDA / 8)
#define GALAS_IV_SIZE 20
#define GALAS_MAX_DEPTH 16
#define GALAS_MAX_TAU 96

typedef enum {
    GALAS_INSTANCE_INVALID = 0,
    GALAS_160S, GALAS_160F,
    GALAS_256S, GALAS_256F,
    GALAS_384S, GALAS_384F,
    GALAS_512S, GALAS_512F,
    GALAS_INSTANCE_MAX_INDEX
} galas_instance_id_t;

typedef struct {
    uint16_t lambda;     /* security parameter in bits */
    uint8_t  tau;        /* number of GGM trees */
    uint8_t  w_grind;    /* grinding weight (zero bits in delta) */
    uint16_t T_open;     /* number of lambda-bit tree nodes revealed */
    /* derived (computed in instances.c) */
    uint16_t k;
    uint8_t  tau0;
    uint8_t  tau1;
    uint32_t L;          /* total leaves = sum over tau trees */
    /* sizes */
    uint16_t pk_bytes;
    uint16_t sk_bytes;   /* x || k, following the FAEST reference convention */
    uint32_t sig_bytes;  /* FAEST-style transcript size */
} galas_param_t;

typedef struct {
    galas_param_t p;
    galas_instance_id_t id;
} galas_paramset_t;

const galas_paramset_t* galas_get_paramset(galas_instance_id_t id);

/* lambda in bytes */
static inline unsigned galas_lambda_bytes(const galas_paramset_t* ps) {
    return ps->p.lambda / 8;
}

/* depth of tree i (trees 0..tau1-1 have depth k, the rest k-1) */
static inline unsigned galas_bavc_depth(unsigned i, const galas_paramset_t* ps) {
    return (i < ps->p.tau1) ? ps->p.k : (ps->p.k - 1);
}
/* number of leaves in tree i */
static inline unsigned galas_bavc_leaves(unsigned i, const galas_paramset_t* ps) {
    return 1u << galas_bavc_depth(i, ps);
}

#endif /* GALAS_INSTANCES_H */
