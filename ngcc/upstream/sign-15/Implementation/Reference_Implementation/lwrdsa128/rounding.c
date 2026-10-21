#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "params.h"

/*************************************************
* Name:        decompose
*
* Description: For element a, compute high and low bits a0, a1 such
*              that a = a1*ALPHA + a0 with -ALPHA/2 < a0 <= ALPHA/2.
*              Assumes a to be standard representative.
*
* Arguments:   - uint32_t a: input element
*              - uint32_t *a0: pointer to output element a0
*
* Returns a1
**************************************************/

uint32_t decompose_keygen(uint32_t a, uint32_t *a0) {
  uint32_t a1 = (a - 1 + (1 << (D - 1))) >> D;
  *a0 = Q + a - (a1 << D);
  return a1;
}


/*************************************************
* Name:        decompose
*
* Description: For element a, compute high and low bits a0, a1 such
*              that a = a1*ALPHA + a0 with -ALPHA/2 < a0 <= ALPHA/2.
*              Assumes a to be standard representative.
*
* Arguments:   - uint32_t a: input element
*              - uint32_t *a0: pointer to output element a0
*
* Returns a1
**************************************************/

uint32_t decompose_sign(uint32_t a, uint32_t *a0) {
  uint32_t a1 = (a - 1 + GAMMA2_BAR) >> GAMMA1_BAR_BITS;
  *a0 = P + a - (a1 << GAMMA1_BAR_BITS);
  return a1 & ((P >> GAMMA1_BAR_BITS) - 1);
}



/*************************************************
* Name:        make_hint
*
* Description: Compute hint bit indicating whether high bits of two elements
*              differ or not
*
* Arguments:   - uint32_t a: first input element
*              - uint32_t b: second input element
*
* Returns 1 if high bits of a and b differ and 0 otherwise
**************************************************/
unsigned int make_hint(const uint32_t a, const uint32_t b) {
  uint32_t t;
  uint32_t temp = (a + b) & (P - 1);
  uint32_t a1 = decompose_sign(a, &t);
  uint32_t a2 = decompose_sign(temp, &t);
  return a1 != a2;
}

/*************************************************
* Name:        use_hint
*
* Description: Correct high bits according to hint
*
* Arguments:   - uint32_t a: input element
*              - unsigned int hint: hint bit
*
* Returns corrected high bits
**************************************************/
uint32_t use_hint(const uint32_t a, const unsigned int hint) {
  uint32_t a0, a1;

  a1 = decompose_sign(a & (P - 1), &a0);
  if(hint == 0)
    return a1;

  if(a0 > P)
    return (a1 + 1) & ((P >> GAMMA1_BAR_BITS) - 1);
  else
    return (a1 - 1) & ((P >> GAMMA1_BAR_BITS) - 1);
}
