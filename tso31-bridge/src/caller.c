/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Small real C caller for later qualification through the exact target route.
 * Host tests establish only its C behavior; they do not prove target linkage.
 */
#include "abi.h"
#include <stddef.h>

int ELFPOC(const unsigned char *argument, unsigned long length,
    const struct lab_tso31_services *services)
{
    static const unsigned char message[] = {
        0x54,0x53,0x4f,0x33,0x31,0x20,0x43,0x41,0x4c,
        0x4c,0x45,0x52,0x20,0x50,0x41,0x53,0x53
    };
    unsigned char *buffer;
    unsigned long i;
    int output_status, release_status;
    if ((length && !argument) || !services || services->version != 4 ||
        !services->putline || !services->allocate || !services->release)
        return 16;
    buffer = (unsigned char *)services->allocate(64);
    if (!buffer) return 20;
    for (i = 0; i < sizeof message; ++i) buffer[i] = message[i];
    output_status = services->putline(buffer, sizeof message);
    release_status = services->release(buffer, 64);
    if (release_status) return 16;
    return output_status ? 12 : 0;
}
