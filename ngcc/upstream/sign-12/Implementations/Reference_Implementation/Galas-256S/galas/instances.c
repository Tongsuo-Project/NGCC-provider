/*
 * instances.c — GALAS parameter sets, with FAEST-style derived fields.
 */
#include "instances.h"

#define K_OF(lambda, w_grind, tau)   (((lambda) - (w_grind)) / (tau) + 1)
/* T_open upper bound = tau*k (refined down later via FAEST nh-2*tau+1 accounting). */
#define TOPEN(tau, k) ((uint16_t)((tau) * (k)))
#define TAU1_OF(lambda, w_grind, tau) (((lambda) - (w_grind)) % (tau))
#define TAU0_OF(lambda, w_grind, tau) ((tau) - TAU1_OF(lambda, w_grind, tau))
#define L_OF(lambda, w_grind, tau)    \
    (TAU1_OF(lambda,w_grind,tau) * (1u << K_OF(lambda,w_grind,tau)) + \
     TAU0_OF(lambda,w_grind,tau) * (1u << (K_OF(lambda,w_grind,tau) - 1)))

#define PS(LAMBDA, TAU, WGRIND, ID, PK, SK, SIG)                                   \
    {                                                                              \
        { (LAMBDA), (TAU), (WGRIND), TOPEN((TAU), K_OF((LAMBDA), (WGRIND), (TAU))),\
          K_OF((LAMBDA), (WGRIND), (TAU)),                                         \
          TAU0_OF((LAMBDA), (WGRIND), (TAU)),                                      \
          TAU1_OF((LAMBDA), (WGRIND), (TAU)),                                      \
          L_OF((LAMBDA), (WGRIND), (TAU)),                                         \
          (PK), (SK), (SIG) },                                                     \
        (ID)                                                                       \
    }

/* (lambda, tau, w_grind, T_open) for the 8 instances.  The tau/w/T_open
   triples match the optimized one-tree instances.  Public keys are x||y and
   secret keys follow the FAEST reference convention x||k, so pk = sk =
   2*lambda/8.  sig_bytes is the FAEST-style transcript size used by the
   optimized implementation. */
static const galas_paramset_t GALAS_160S_PS =
    { { 160, 14, 7, 132, K_OF(160,7,14), TAU0_OF(160,7,14), TAU1_OF(160,7,14),
        L_OF(160,7,14), 40, 40, 4812 }, GALAS_160S };
static const galas_paramset_t GALAS_160F_PS =
    { { 160, 20, 8, 144, K_OF(160,8,20), TAU0_OF(160,8,20), TAU1_OF(160,8,20),
        L_OF(160,8,20), 40, 40, 5964 }, GALAS_160F };
static const galas_paramset_t GALAS_256S_PS =
    { { 256, 21, 7, 218, K_OF(256,7,21), TAU0_OF(256,7,21), TAU1_OF(256,7,21),
        L_OF(256,7,21), 64, 64, 12114 }, GALAS_256S };
static const galas_paramset_t GALAS_256F_PS =
    { { 256, 35, 12, 232, K_OF(256,12,35), TAU0_OF(256,12,35), TAU1_OF(256,12,35),
        L_OF(256,12,35), 64, 64, 15950 }, GALAS_256F };
static const galas_paramset_t GALAS_384S_PS =
    { { 384, 32, 6, 336, K_OF(384,6,32), TAU0_OF(384,6,32), TAU1_OF(384,6,32),
        L_OF(384,6,32), 96, 96, 27784 }, GALAS_384S };
static const galas_paramset_t GALAS_384F_PS =
    { { 384, 48, 6, 332, K_OF(384,6,48), TAU0_OF(384,6,48), TAU1_OF(384,6,48),
        L_OF(384,6,48), 96, 96, 33384 }, GALAS_384F };
static const galas_paramset_t GALAS_512S_PS =
    { { 512, 42, 8, 456, K_OF(512,8,42), TAU0_OF(512,8,42), TAU1_OF(512,8,42),
        L_OF(512,8,42), 128, 128, 49516 }, GALAS_512S };
static const galas_paramset_t GALAS_512F_PS =
    { { 512, 64, 6, 445, K_OF(512,6,64), TAU0_OF(512,6,64), TAU1_OF(512,6,64),
        L_OF(512,6,64), 128, 128, 59416 }, GALAS_512F };

const galas_paramset_t* galas_get_paramset(galas_instance_id_t id) {
    switch (id) {
        case GALAS_160S: return &GALAS_160S_PS;
        case GALAS_160F: return &GALAS_160F_PS;
        case GALAS_256S: return &GALAS_256S_PS;
        case GALAS_256F: return &GALAS_256F_PS;
        case GALAS_384S: return &GALAS_384S_PS;
        case GALAS_384F: return &GALAS_384F_PS;
        case GALAS_512S: return &GALAS_512S_PS;
        case GALAS_512F: return &GALAS_512F_PS;
        default: return 0;
    }
}
