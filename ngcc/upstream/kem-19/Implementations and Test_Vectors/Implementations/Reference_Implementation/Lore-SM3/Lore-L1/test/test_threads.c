#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../api.h"
#include "../poly.h"

static pthread_barrier_t barrier;

static void *worker(void *arg)
{
    uintptr_t id = (uintptr_t)arg;
    uint8_t message[LORE_MSG_BYTES], decoded[LORE_MSG_BYTES];
    poly encoded;
    pthread_barrier_wait(&barrier);
    for (unsigned int trial = 0; trial < 1000; ++trial) {
        for (unsigned int i = 0; i < sizeof message; ++i)
            message[i] = (uint8_t)(id * 37 + trial + i * 13);
        poly_encode_msg(&encoded, message);
        /* Force decoder scratch-space use, not only the no-error fast path. */
        for (unsigned int i = 0; i < LORE_BCH_T; ++i) {
            unsigned int pos = (i * 17 + trial) % (LORE_MSG_BYTES * 8);
            encoded.coeffs[pos] ^= (LORE_T * LORE_Q) / 2;
        }
        poly_decode_msg(decoded, &encoded);
        if (memcmp(message, decoded, sizeof message)) return (void *)1;
        poly_crt crt;
        poly_crt_decompose(&crt, &encoded);
        poly_decode_msg_crt(decoded, &crt);
        if (memcmp(message, decoded, sizeof message)) return (void *)1;
    }
    return NULL;
}

int main(void)
{
    pthread_t threads[4];
    if (pthread_barrier_init(&barrier, NULL, 4)) return 1;
    for (uintptr_t i = 0; i < 4; ++i)
        if (pthread_create(&threads[i], NULL, worker, (void *)i)) return 1;
    int failed = 0;
    for (unsigned int i = 0; i < 4; ++i) {
        void *result;
        if (pthread_join(threads[i], &result) || result != NULL) failed = 1;
    }
    pthread_barrier_destroy(&barrier);
    if (failed) return 1;
    puts("BCH: concurrent initialization and 8000 error-correcting decodes passed");
    return 0;
}
