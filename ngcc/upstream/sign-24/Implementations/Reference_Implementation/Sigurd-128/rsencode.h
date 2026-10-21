/*
 * Minimal Reed-Solomon encoder API used by the reference signature core.
 * The encoder works over GF(2^16) shards and exposes a scalar fallback plus
 * retained fixed-shape transforms for older packed dimensions.
 *
 * A "block" is one shard buffer containing buffer_bytes bytes.  The caller
 * provides original_count source blocks and recovery_count writable work blocks;
 * successful encoding fills the first recovery_count work blocks.
 */

#ifndef RSENCODE_H
#define RSENCODE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RS_VERSION 2

/*
 * Initialize GF tables and retained fixed-transform self tests.
 * Must be called before rs_encode(); the version parameter guards against
 * accidentally linking incompatible headers and objects.
 */
int rs_init_(int version);
#define rs_init() rs_init_(RS_VERSION)

/* Negative values are explicit validation or initialization failures. */
typedef enum RsencodeResultT
{
    Rsencode_Success = 0,
    Rsencode_NeedMoreData = -1,
    Rsencode_TooMuchData = -2,
    Rsencode_InvalidSize = -3,
    Rsencode_InvalidCounts = -4,
    Rsencode_InvalidInput = -5,
    Rsencode_Platform = -6,
    Rsencode_CallInitialize = -7
} RsencodeResult;

const char *rs_result_string(RsencodeResult result);

/* Return the number of recovery buffers the caller must provide. */
unsigned rs_encode_work_count(unsigned original_count, unsigned recovery_count);

/*
 * Encode original_data shards into work_data recovery shards.
 * All buffers must point to buffer_bytes bytes, and buffer_bytes must be a
 * multiple of sizeof(uint16_t) because symbols are GF(2^16) elements.
 */
RsencodeResult rs_encode(
    uint64_t buffer_bytes,
    unsigned original_count,
    unsigned recovery_count,
    unsigned work_count,
    const void *const *original_data,
    void **work_data);

/*
 * ---- Vector Reed-Solomon evaluation over the submitted evaluation domain ----
 *
 * The signature core does not use the shard API above.  It commits to the
 * evaluation of a single degree-<d polynomial over F_{2^(16*RAW_POLY_LEN)} on
 * the domain
 *
 *     L_p = omega^(p mod 51) * g^(p div 51),    omega = g^1285,  g = 2,
 *
 * i.e. the first ceil(N / 51) multiplicative cosets of the order-51 subgroup
 * H = <omega> of GF(2^16)^*.  Every coset cH is handled by one 51-point
 * Cooley-Tukey NTT (51 = 3 * 17), because
 *
 *     f(c * omega^i) = sum_{r<51} a_r * omega^(i r),
 *     a_r = c^r * sum_t f[r + 51t] * (c^51)^t,
 *
 * so the encoding costs O((N/51) * 51 log 51) instead of O(N * d).  Because the
 * evaluation points lie in the subfield GF(2^16), an F_{2^(16*RAW_POLY_LEN)}
 * polynomial is evaluated independently on each of its RAW_POLY_LEN GF(2^16)
 * coefficient lanes, which is what the interleaved `lanes` argument expresses.
 *
 * |H| = 51 is chosen to balance the two users of this evaluator: the prover
 * pays one transform per coset, while the verifier pays ~|H| products per
 * opened position, so a smaller subgroup trades prover cost for verifier cost.
 *
 * This is the single global [N, d] Reed-Solomon code of minimum distance
 * N - d + 1.  The domain must be documented in full in the specification: a
 * codeword that is a direct sum of short codes has far smaller minimum
 * distance, and the commitment layer's knowledge-soundness argument depends on
 * that distance.
 *
 * Buffers hold `lanes` interleaved GF(2^16) lanes: msg[i * lanes + l] is
 * coefficient i of lane l.  Requires N <= 51 * kEvalMaxCosets and
 * d <= 51 * kEvalMaxFold.
 */

/* Evaluate every codeword position 0..N-1. */
RsencodeResult rs_eval_codeword(
    uint16_t *out,
    const uint16_t *msg,
    unsigned d,
    unsigned n,
    unsigned lanes);

/*
 * Evaluate an arbitrary set of codeword positions.  out[q * lanes + l] holds
 * the value at pos[q] for lane l; the requested positions need not be sorted
 * and may repeat.
 */
RsencodeResult rs_eval_positions(
    uint16_t *out,
    const uint16_t *msg,
    unsigned d,
    unsigned lanes,
    const unsigned *pos,
    unsigned count);

/* Size of H; a codeword position p names coset p / rs_eval_coset_order(). */
unsigned rs_eval_coset_order(void);

#ifdef __cplusplus
}
#endif

#endif
