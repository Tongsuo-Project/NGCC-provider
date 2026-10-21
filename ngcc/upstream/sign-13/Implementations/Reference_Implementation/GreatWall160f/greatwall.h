#ifndef GREATWALL160_H
#define GREATWALL160_H

#include "gf163.h"

#define PYLON160_LAMBDA 160
#define PYLON160_N 163
#define PYLON160_E1 79
#define PYLON160_E2 86

/* Row-major binary matrices in the polynomial basis, low word first.
 * Bit j of row i is the coefficient taking input bit j to output bit i.
 * Only the low 35 bits of each row's third word are used.
 * Production bindings use the reproducible generated tables described in
 * constants-manifest.json. Test-only fixtures replace the complete object.
 */
extern const uint64_t (*const greatwall_rc_163)[3];
extern const uint64_t (*const greatwall_mat_163_0)[3];
extern const uint64_t (*const greatwall_mat_163_1)[3];
extern const uint64_t (*const greatwall_mat_163_2)[3];
extern const uint64_t (*const greatwall_mat_163_2_inv)[3];

bool greatwall160_constants_available(void);

#endif
