#include "greatwall.h"
#include "greatwall-160-matrix-data.h"

const uint64_t (*const greatwall_rc_163)[3] = greatwall_rc_163_data;
const uint64_t (*const greatwall_mat_163_0)[3] = greatwall_mat_163_0_data;
const uint64_t (*const greatwall_mat_163_1)[3] = greatwall_mat_163_1_data;
const uint64_t (*const greatwall_mat_163_2)[3] = greatwall_mat_163_2_data;
const uint64_t (*const greatwall_mat_163_2_inv)[3] = greatwall_mat_163_2_inv_data;

bool greatwall160_constants_available(void)
{
    return greatwall_rc_163 && greatwall_mat_163_0 && greatwall_mat_163_1 &&
           greatwall_mat_163_2 && greatwall_mat_163_2_inv;
}

/* Generated under Sage/Linux from the user-designated AIMer generator.
 * See constants-manifest.json for the upstream generator, seed,
 * preserved stream/output conventions and validation. These are experimental
 * parameter tables, not an externally standardized or security-certified set.
 * The separate Frobenius maps remain determined only by the native modulus. */
