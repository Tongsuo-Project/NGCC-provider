/* Copyright (c) 2026 Hang Zhang. */
#ifndef BIT_API_INPUT_CHECKS_H
#define BIT_API_INPUT_CHECKS_H

#include "symmetric.h"

/* Exercise invalid arguments without relying on the DRNG state. */
static int test_api_input_validation(void)
{
    unsigned char byte = 0;
    unsigned long long length = 0;

    if (sig_keygen(NULL, &length, &byte, &length) != -1) return 1;
    if (sig_keygen(&byte, NULL, &byte, &length) != -1) return 2;
    if (sig_keygen(&byte, &length, NULL, &length) != -1) return 3;
    if (sig_keygen(&byte, &length, &byte, NULL) != -1) return 4;

    if (sig_sign(NULL, BIT_SECRETKEYBYTES, &byte, 1,
                 &byte, &length) != -1) return 5;
    if (sig_sign(&byte, BIT_SECRETKEYBYTES, NULL, 1,
                 &byte, &length) != -1) return 6;
    if (sig_sign(&byte, BIT_SECRETKEYBYTES, &byte, 1,
                 NULL, &length) != -1) return 7;
    if (sig_sign(&byte, BIT_SECRETKEYBYTES, &byte, 1,
                 &byte, NULL) != -1) return 8;
    if (sig_sign(&byte, 0, &byte, 1,
                 &byte, &length) != -1) return 9;

    if (sig_verify(NULL, BIT_PUBLICKEYBYTES, &byte, BIT_SIGNBYTES,
                   &byte, 1) != -1) return 10;
    if (sig_verify(&byte, BIT_PUBLICKEYBYTES, NULL, BIT_SIGNBYTES,
                   &byte, 1) != -1) return 11;
    if (sig_verify(&byte, BIT_PUBLICKEYBYTES, &byte, BIT_SIGNBYTES,
                   NULL, 1) != -1) return 12;
    if (sig_verify(&byte, 0, &byte, BIT_SIGNBYTES,
                   &byte, 1) != -1) return 13;
    if (sig_verify(&byte, BIT_PUBLICKEYBYTES, &byte, 0,
                   &byte, 1) != -1) return 14;
#if !BIT_USE_SHAKE
    {
        unsigned char digest[BIT_MESSAGEBYTES];

        if (bit_h256_2(digest, &byte, SIZE_MAX, &byte, 1) != -1)
            return 15;
    }
#endif
    return 0;
}

#endif
