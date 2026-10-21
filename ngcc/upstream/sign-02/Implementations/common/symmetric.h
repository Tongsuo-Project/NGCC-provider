/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"

#ifndef BIT_USE_SHAKE
#define BIT_USE_SHAKE 1
#endif

#if BIT_USE_SHAKE
#include "fips202.h"
#define BIT_XOF256_RATE SHAKE256_RATE
#define BIT_XOF128_RATE SHAKE128_RATE
#else
#define BIT_XOF256_RATE 32
#define BIT_XOF128_RATE 32
#endif

/* Refill the byte stream in SHAKE256-rate blocks, independently of read size. */
#define BIT_XOF256_STREAM_BLOCKBYTES 136

/* SHAKE256 stateful stream */
typedef struct {
#if BIT_USE_SHAKE
    shake256ctx state;
#else
    const unsigned char *seed;
    size_t seed_len_bytes;
    uint32_t block;
#endif
    uint8_t leftover[BIT_XOF256_STREAM_BLOCKBYTES];
    size_t leftover_len;
    size_t leftover_pos;
    uint16_t nonce;
} bit_xof256_state;

/* SHAKE128 stateful stream */
typedef struct {
#if BIT_USE_SHAKE
    shake128ctx state;
    uint8_t leftover[SHAKE128_RATE];
    size_t leftover_len;
    size_t leftover_pos;
#else
    const unsigned char *seed;
    size_t seed_len_bytes;
    uint32_t block;
#endif
} bit_xof128_state;
/* SHAKE256 block-level context (challenge expansion) */
typedef struct {
#if BIT_USE_SHAKE
    shake256ctx state;
#else
    const unsigned char *seed;
    size_t seed_len_bytes;
    uint32_t block;
#endif
} bit_xof256_ctx;

#ifdef __cplusplus
extern "C" {
#endif

void bit_secure_zero(void *ptr, size_t len);

/* Hash widths follow BIT_TRBYTES and BIT_MESSAGEBYTES. */
#define bit_h256 bit_hash
#define bit_h256_2 bit_hash_message
int bit_hash(uint8_t out[BIT_TRBYTES], const uint8_t *in, size_t inlen);
int bit_hash_message(uint8_t out[BIT_MESSAGEBYTES], const uint8_t *in0, size_t inlen0,
                     const uint8_t *in1, size_t inlen1);

#if BIT_SECURITY_BITS == 512
/* Raw-message prefix followed by the encoded commitment. */
typedef struct {
#if BIT_USE_SHAKE
    sha3_512incctx prefix;
#else
    uint8_t *input;
    size_t input_len;
#endif
} bit_challenge_ctx;

int bit_challenge_init(bit_challenge_ctx *ctx, const uint8_t tr[BIT_TRBYTES],
                       const uint8_t *message, size_t message_len);
int bit_challenge_hash(uint8_t out[BIT_CHALLENGEBYTES], const bit_challenge_ctx *ctx,
                       const uint8_t commitment[BIT_POLYVECK_W1_BYTES]);
void bit_challenge_release(bit_challenge_ctx *ctx);
#endif

/* Fallible hashes and XOF reads return 0 on success, -1 on failure.
 * Discard partial output and release the stream after a failed read. */
int bit_xof256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen);

/* Seed must be BIT_SEEDBYTES; returns -1 for an invalid length. */
int bit_xof256_nonce(uint8_t *out, size_t outlen, const uint8_t *seed, size_t seedlen,
                     uint16_t nonce);
/* Inputs must be seed, seed, message lengths; returns -1 otherwise. */
int bit_xof256_3(uint8_t *out, size_t outlen, const uint8_t *in0, size_t inlen0, const uint8_t *in1,
                 size_t inlen1, const uint8_t *in2, size_t inlen2);

/* SHAKE256 stateful stream */
void bit_xof256_init(bit_xof256_state *st, const uint8_t *seed, size_t seedlen, uint16_t nonce);
int bit_xof256_squeeze(bit_xof256_state *st, uint8_t *out, size_t outlen);
void bit_xof256_zeroize(bit_xof256_state *st);

/* SHAKE256 block-level context */
void bit_xof256_ctx_init(bit_xof256_ctx *ctx, const uint8_t *seed, size_t seedlen);
int bit_xof256_ctx_squeezeblocks(bit_xof256_ctx *ctx, uint8_t *out, size_t nblocks);
void bit_xof256_ctx_zeroize(bit_xof256_ctx *ctx);

/* SHAKE128 stateful stream (matrix expansion) */
void bit_xof128_init(bit_xof128_state *st, const uint8_t *in, size_t inlen);
int bit_xof128_squeeze(bit_xof128_state *st, uint8_t *out, size_t outlen);
void bit_xof128_zeroize(bit_xof128_state *st);

#ifdef __cplusplus
}
#endif
#endif
