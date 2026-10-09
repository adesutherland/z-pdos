/* SPDX-License-Identifier: MIT; execution-text labels to native display bytes. */
#ifndef PDOS_TWO_SPACE_DISPLAY_H
#define PDOS_TWO_SPACE_DISPLAY_H
int TWDLabel(const char *,unsigned char *,unsigned int,unsigned int *);
int TWDAscii(unsigned int,unsigned int *);
unsigned int TWDNative(unsigned int);
#endif
