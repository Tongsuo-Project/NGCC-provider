#ifndef OWF_PROOF_H
#define OWF_PROOF_H

#include "config.h"
#include "gf163.h"
#include "greatwall.h"
#include "quicksilver.h"

typedef block163 owf_block;

#define OWF_KEY_SCHEDULE_CONSTRAINTS 0
#define OWF_BLOCKS 1
#define OWF_ROUNDS 2
#define OWF_KEY_WITNESS_BITS 168
#define OWF_BLOCK_SIZE 21
#define OWF_CONSTRAINTS_PER_ROUND 163
#define OWF_ALPHA_CONSTRAINTS 2

#define OWF_NUM_CONSTRAINTS (OWF_BLOCKS * OWF_CONSTRAINTS_PER_ROUND * OWF_ROUNDS + OWF_KEY_SCHEDULE_CONSTRAINTS + OWF_ALPHA_CONSTRAINTS)
#define WITNESS_BITS (8 * OWF_BLOCKS * OWF_BLOCK_SIZE * (OWF_ROUNDS - 1) + OWF_KEY_WITNESS_BITS)

struct public_key;
typedef struct public_key public_key;

void owf_constraints_prover(quicksilver_state* state, const public_key* pk);
void owf_constraints_verifier(quicksilver_state* state, const public_key* pk);

#endif
