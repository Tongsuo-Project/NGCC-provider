#ifndef CONFIG_H
#define CONFIG_H

/* C99 compile-time assertion; no storage or runtime evaluation. */
#define GW_STATIC_ASSERT(condition, message) \
    extern void greatwall_static_assertion(char (*)[(condition) ? 1 : -1])
/* Experimental profile, matching Optimized GreatWall160s. */
#define SECURITY_PARAM 160
#define GREATWALL_INSTANCE_NAME "GreatWall160s"
#define OWF_GREATWALL
#define PRG_AES_CTR
#define TREE_PRG_AES_CTR
#define LEAF_PRG_SHAKE
#define BITS_PER_WITNESS 14
#define ZERO_BITS_IN_CHALLENGE_3 0
#define USE_IMPROVED_VECTOR_COMMITMENTS 1
#define BATCH_VECTOR_OPENING_SEEDS_TRESHOLD 160
#define RANDOM_ORACLE_SHA3
#define XKCP_has_KeccakP1600
#endif
