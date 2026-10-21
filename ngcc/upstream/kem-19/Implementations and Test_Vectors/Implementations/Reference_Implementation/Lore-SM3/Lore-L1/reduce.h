#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

#define montgomery_reduce LORE_NAMESPACE(montgomery_reduce)
int16_t montgomery_reduce(int64_t a);

#define barrett_reduce LORE_NAMESPACE(barrett_reduce)
int16_t barrett_reduce(int16_t a);

#define poly_reduce LORE_NAMESPACE(poly_reduce)
void poly_reduce(poly *r);

#endif