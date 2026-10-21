/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
/* Symmetric primitive abstraction for the SHAKE and SM3 backends. */
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "symmetric.h"
#if BIT_USE_SHAKE
#include "fips202.h"
#else
#include "auxfunc.h"
#endif
#include "params.h"
#include "endian.h"

void bit_secure_zero(void *ptr, size_t len)
{
#if defined(__GNUC__) || defined(__clang__)
    memset(ptr, 0, len);
    __asm__ __volatile__("" : : "r"(ptr) : "memory");
#else
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (len--)
        *p++ = 0;
#endif
}

#if !BIT_USE_SHAKE
static int bit_sm3_fixed_hash(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
    if (inlen > ULLONG_MAX / 8ULL)
        return -1;
    if (outlen == 32)
        return sm3hash(256, in, (unsigned long long)inlen * 8ULL, out) == 0 ? 0 : -1;
    /* pseudohash adds a two-byte domain and one HMAC input block. */
    if (outlen != 64 || inlen > SIZE_MAX - 66 || inlen > (ULLONG_MAX - 528ULL) / 8ULL)
        return -1;
    return pseudohash(512, in, (unsigned long long)inlen * 8ULL, out) == 0 ? 0 : -1;
}
#endif

int bit_hash(uint8_t out[BIT_TRBYTES], const uint8_t *in, size_t inlen)
{
#if BIT_USE_SHAKE
#if BIT_TRBYTES == 32
    sha3_256(out, in, inlen);
#else
    sha3_512(out, in, inlen);
#endif
#else
    return bit_sm3_fixed_hash(out, BIT_TRBYTES, in, inlen);
#endif
    return 0;
}

int bit_hash_message(uint8_t out[BIT_MESSAGEBYTES], const uint8_t *in0, size_t inlen0,
                     const uint8_t *in1, size_t inlen1)
{
#if BIT_USE_SHAKE
#if BIT_MESSAGEBYTES == 32
    sha3_256incctx state;
    sha3_256_inc_init(&state);
    sha3_256_inc_absorb(&state, in0, inlen0);
    sha3_256_inc_absorb(&state, in1, inlen1);
    sha3_256_inc_finalize(out, &state);
#else
    sha3_512incctx state;
    sha3_512_inc_init(&state);
    sha3_512_inc_absorb(&state, in0, inlen0);
    sha3_512_inc_absorb(&state, in1, inlen1);
    sha3_512_inc_finalize(out, &state);
#endif
    return 0;
#else
    unsigned char buf[4096];
    size_t total = inlen0 + inlen1;
    int status;

    if (inlen0 > SIZE_MAX - inlen1 || total > ULLONG_MAX / 8ULL)
        return -1;
    if (total > sizeof(buf)) {
        unsigned char *msg = malloc(total);
        if (msg == NULL)
            return -1;
        memcpy(msg, in0, inlen0);
        memcpy(msg + inlen0, in1, inlen1);
        status = bit_sm3_fixed_hash(out, BIT_MESSAGEBYTES, msg, total);
        bit_secure_zero(msg, total);
        free(msg);
    } else {
        memcpy(buf, in0, inlen0);
        memcpy(buf + inlen0, in1, inlen1);
        status = bit_sm3_fixed_hash(out, BIT_MESSAGEBYTES, buf, total);
        bit_secure_zero(buf, total);
    }
    return status == 0 ? 0 : -1;
#endif
}

#if BIT_SECURITY_BITS == 512
int bit_challenge_init(bit_challenge_ctx *ctx, const uint8_t tr[BIT_TRBYTES],
                       const uint8_t *message, size_t message_len)
{
    uint8_t length[8];
    if (ctx == NULL)
        return -1;
    memset(ctx, 0, sizeof(*ctx));
    if (tr == NULL || (message == NULL && message_len != 0))
        return -1;
    store64_le(length, (uint64_t)message_len);
#if BIT_USE_SHAKE
    sha3_512_inc_init(&ctx->prefix);
    sha3_512_inc_absorb(&ctx->prefix, tr, BIT_TRBYTES);
    sha3_512_inc_absorb(&ctx->prefix, length, sizeof(length));
    if (message_len != 0)
        sha3_512_inc_absorb(&ctx->prefix, message, message_len);
#else
    const size_t overhead = BIT_TRBYTES + sizeof(length) + BIT_POLYVECK_W1_BYTES;
    if (message_len > SIZE_MAX - overhead || message_len + overhead > ULLONG_MAX / 8ULL)
        return -1;
    ctx->input_len = message_len + overhead;
    ctx->input = malloc(ctx->input_len);
    if (ctx->input == NULL) {
        ctx->input_len = 0;
        return -1;
    }
    uint8_t *p = ctx->input;
    memcpy(p, tr, BIT_TRBYTES);
    p += BIT_TRBYTES;
    memcpy(p, length, sizeof(length));
    p += sizeof(length);
    if (message_len != 0)
        memcpy(p, message, message_len);
#endif
    return 0;
}

int bit_challenge_hash(uint8_t out[BIT_CHALLENGEBYTES], const bit_challenge_ctx *ctx,
                       const uint8_t commitment[BIT_POLYVECK_W1_BYTES])
{
#if BIT_USE_SHAKE
    sha3_512incctx state;
    sha3_512_inc_ctx_clone(&state, &ctx->prefix);
    sha3_512_inc_absorb(&state, commitment, BIT_POLYVECK_W1_BYTES);
    sha3_512_inc_finalize(out, &state);
    bit_secure_zero(&state, sizeof(state));
    return 0;
#else
    if (ctx->input == NULL)
        return -1;
    memcpy(ctx->input + ctx->input_len - BIT_POLYVECK_W1_BYTES, commitment,
           BIT_POLYVECK_W1_BYTES);
    return bit_sm3_fixed_hash(out, BIT_CHALLENGEBYTES, ctx->input, ctx->input_len);
#endif
}

void bit_challenge_release(bit_challenge_ctx *ctx)
{
#if !BIT_USE_SHAKE
    if (ctx->input != NULL) {
        bit_secure_zero(ctx->input, ctx->input_len);
        free(ctx->input);
    }
#endif
    bit_secure_zero(ctx, sizeof(*ctx));
}
#endif

int bit_xof256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
#if BIT_USE_SHAKE
    shake256(out, outlen, in, inlen);
#else
    if (inlen > ULLONG_MAX / 8ULL || outlen > ULLONG_MAX / 8ULL)
        return -1;
    return pseudoXOF((unsigned long long)outlen * 8, in, (unsigned long long)inlen * 8, out) == 0
               ? 0
               : -1;
#endif
    return 0;
}

/* One-shot XOF over seed || nonce (16-bit little-endian nonce). */
int bit_xof256_nonce(uint8_t *out, size_t outlen, const uint8_t *seed, size_t seedlen,
                     uint16_t nonce)
{
    if (seedlen != BIT_SEEDBYTES)
        return -1;
#if BIT_USE_SHAKE
    uint8_t extseed[BIT_SEEDBYTES + sizeof(uint16_t)];

    memcpy(extseed, seed, seedlen);
    store16_le(extseed + seedlen, nonce);
    shake256(out, outlen, extseed, seedlen + sizeof(uint16_t));
    bit_secure_zero(extseed, sizeof(extseed));
#else
    uint8_t extseed[BIT_SEEDBYTES + sizeof(uint16_t)];
    memcpy(extseed, seed, seedlen);
    store16_le(extseed + seedlen, nonce);
    int status = bit_xof256(out, outlen, extseed, sizeof(extseed));
    bit_secure_zero(extseed, sizeof(extseed));
    return status;
#endif
    return 0;
}

/* One-shot XOF absorbing three inputs in sequence (in0 || in1 || in2). */
int bit_xof256_3(uint8_t *out, size_t outlen, const uint8_t *in0, size_t inlen0, const uint8_t *in1,
                 size_t inlen1, const uint8_t *in2, size_t inlen2)
{
    if (inlen0 != BIT_SEEDBYTES || inlen1 != BIT_SEEDBYTES || inlen2 != BIT_MESSAGEBYTES)
        return -1;
#if BIT_USE_SHAKE
    shake256incctx state;
    shake256_inc_init(&state);
    shake256_inc_absorb(&state, in0, inlen0);
    shake256_inc_absorb(&state, in1, inlen1);
    shake256_inc_absorb(&state, in2, inlen2);
    shake256_inc_finalize(&state);
    shake256_inc_squeeze(out, outlen, &state);
    bit_secure_zero(state.ctx, PQC_SHAKEINCCTX_BYTES);
    shake256_inc_ctx_release(&state);
#else
    uint8_t msg[BIT_SEEDBYTES + BIT_SEEDBYTES + BIT_MESSAGEBYTES];
    memcpy(msg, in0, inlen0);
    memcpy(msg + inlen0, in1, inlen1);
    memcpy(msg + inlen0 + inlen1, in2, inlen2);
    size_t msg_len = inlen0 + inlen1 + inlen2;
    int status = bit_xof256(out, outlen, msg, msg_len);
    bit_secure_zero(msg, msg_len);
    return status;
#endif
    return 0;
}

/* Initialize a block stream keyed by seed || nonce. */

void bit_xof256_init(bit_xof256_state *st, const uint8_t *seed, size_t seedlen, uint16_t nonce)
{
#if BIT_USE_SHAKE
    uint8_t extseed[seedlen + sizeof(uint16_t)];

    memcpy(extseed, seed, seedlen);
    store16_le(extseed + seedlen, nonce);
    shake256_absorb(&st->state, extseed, sizeof(extseed));
    bit_secure_zero(extseed, sizeof(extseed));
#else
    st->seed = seed;
    st->seed_len_bytes = seedlen;
    st->block = 0;
#endif
    st->leftover_len = 0;
    st->leftover_pos = 0;
    st->nonce = nonce;
}

/* Produce complete backend blocks for the generic XOF256 stream. */
static int bit_xof256_stream_blocks(bit_xof256_state *st, uint8_t *out, size_t nblocks)
{
#if BIT_USE_SHAKE
    shake256_squeezeblocks(out, nblocks, &st->state);
#else
    while (nblocks-- != 0) {
        uint8_t extseed[st->seed_len_bytes + 2 + 4];

        memcpy(extseed, st->seed, st->seed_len_bytes);
        store16_le(extseed + st->seed_len_bytes, st->nonce);
        store32_be(extseed + st->seed_len_bytes + 2, st->block);
        st->block++;
        int status = bit_xof256(out, BIT_XOF256_STREAM_BLOCKBYTES, extseed, sizeof(extseed));
        bit_secure_zero(extseed, sizeof(extseed));
        if (status != 0)
            return -1;
        out += BIT_XOF256_STREAM_BLOCKBYTES;
    }
#endif
    return 0;
}

int bit_xof256_squeeze(bit_xof256_state *st, uint8_t *out, size_t outlen)
{
    if (st->leftover_pos < st->leftover_len) {
        size_t available = st->leftover_len - st->leftover_pos;
        size_t take = outlen < available ? outlen : available;
        memcpy(out, st->leftover + st->leftover_pos, take);
        st->leftover_pos += take;
        out += take;
        outlen -= take;
    }

    if (outlen >= BIT_XOF256_STREAM_BLOCKBYTES) {
        size_t nblocks = outlen / BIT_XOF256_STREAM_BLOCKBYTES;
        size_t nbytes = nblocks * BIT_XOF256_STREAM_BLOCKBYTES;
        if (bit_xof256_stream_blocks(st, out, nblocks) != 0) {
            return -1;
        }
        out += nbytes;
        outlen -= nbytes;
    }

    if (outlen != 0) {
        if (bit_xof256_stream_blocks(st, st->leftover, 1) != 0) {
            return -1;
        }
        memcpy(out, st->leftover, outlen);
        st->leftover_pos = outlen;
        st->leftover_len = BIT_XOF256_STREAM_BLOCKBYTES;
    } else if (st->leftover_pos == st->leftover_len) {
        st->leftover_pos = 0;
        st->leftover_len = 0;
    }
    return 0;
}

void bit_xof256_zeroize(bit_xof256_state *st)
{
#if BIT_USE_SHAKE
    bit_secure_zero(st->state.ctx, PQC_SHAKECTX_BYTES);
    shake256_ctx_release(&st->state);
#else
    st->seed = NULL;
    st->seed_len_bytes = 0;
    st->block = 0;
#endif
    bit_secure_zero(st->leftover, sizeof(st->leftover));
    st->leftover_len = 0;
    st->leftover_pos = 0;
    st->nonce = 0;
}

void bit_xof256_ctx_init(bit_xof256_ctx *ctx, const uint8_t *seed, size_t seedlen)
{
#if BIT_USE_SHAKE
    shake256_absorb(&ctx->state, seed, seedlen);
#else
    ctx->seed = seed;
    ctx->seed_len_bytes = seedlen;
    ctx->block = 0;
#endif
}

int bit_xof256_ctx_squeezeblocks(bit_xof256_ctx *ctx, uint8_t *out, size_t nblocks)
{
#if BIT_USE_SHAKE
    shake256_squeezeblocks(out, nblocks, &ctx->state);
#else
    for (size_t i = 0; i < nblocks; i++) {
        uint8_t extseed[ctx->seed_len_bytes + 4];
        memcpy(extseed, ctx->seed, ctx->seed_len_bytes);
        store32_be(extseed + ctx->seed_len_bytes, ctx->block);
        ctx->block++;
        int status =
            bit_xof256(out + i * BIT_XOF256_RATE, BIT_XOF256_RATE, extseed, sizeof(extseed));
        bit_secure_zero(extseed, sizeof(extseed));
        if (status != 0)
            return -1;
    }
#endif
    return 0;
}

void bit_xof256_ctx_zeroize(bit_xof256_ctx *ctx)
{
#if BIT_USE_SHAKE
    bit_secure_zero(ctx->state.ctx, PQC_SHAKECTX_BYTES);
    shake256_ctx_release(&ctx->state);
#else
    ctx->seed = NULL;
    ctx->seed_len_bytes = 0;
    ctx->block = 0;
#endif
}

void bit_xof128_init(bit_xof128_state *st, const uint8_t *in, size_t inlen)
{
#if BIT_USE_SHAKE
    shake128_absorb(&st->state, in, inlen);
    st->leftover_len = 0;
    st->leftover_pos = 0;
#else
    st->seed = in;
    st->seed_len_bytes = inlen;
    st->block = 0;
#endif
}

int bit_xof128_squeeze(bit_xof128_state *st, uint8_t *out, size_t outlen)
{
#if BIT_USE_SHAKE
    if (st->leftover_pos < st->leftover_len) {
        size_t left = st->leftover_len - st->leftover_pos;
        size_t take = outlen < left ? outlen : left;
        memcpy(out, st->leftover + st->leftover_pos, take);
        st->leftover_pos += take;
        out += take;
        outlen -= take;
    }

    size_t nblocks = outlen / SHAKE128_RATE;
    if (nblocks != 0) {
        shake128_squeezeblocks(out, nblocks, &st->state);
        out += nblocks * SHAKE128_RATE;
        outlen -= nblocks * SHAKE128_RATE;
    }

    if (outlen != 0) {
        shake128_squeezeblocks(st->leftover, 1, &st->state);
        memcpy(out, st->leftover, outlen);
        st->leftover_pos = outlen;
        st->leftover_len = SHAKE128_RATE;
    } else if (st->leftover_pos == st->leftover_len) {
        st->leftover_pos = 0;
        st->leftover_len = 0;
    }
#else
    uint8_t extseed[st->seed_len_bytes + 6];
    memcpy(extseed, st->seed, st->seed_len_bytes);
    extseed[st->seed_len_bytes] = 0;
    extseed[st->seed_len_bytes + 1] = 0;
    store32_be(extseed + st->seed_len_bytes + 2, st->block);
    st->block++;
    return bit_xof256(out, outlen, extseed, sizeof(extseed));
#endif
    return 0;
}

void bit_xof128_zeroize(bit_xof128_state *st)
{
#if BIT_USE_SHAKE
    bit_secure_zero(st->state.ctx, PQC_SHAKECTX_BYTES);
    bit_secure_zero(st->leftover, sizeof(st->leftover));
    st->leftover_len = 0;
    st->leftover_pos = 0;
    shake128_ctx_release(&st->state);
#else
    st->seed = NULL;
    st->seed_len_bytes = 0;
    st->block = 0;
#endif
}
