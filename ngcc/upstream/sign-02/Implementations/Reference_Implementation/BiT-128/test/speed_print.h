#ifndef PRINT_SPEED_H
#define PRINT_SPEED_H

#include <stddef.h>
#include <stdint.h>

/* Define CSV_PRINT for CSV output. */

void print_results(const char *s, uint64_t *t, size_t tlen);

#endif
