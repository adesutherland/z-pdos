/* SPDX-License-Identifier: MIT; UTF8 model / explicit IBM1047 boundary. */
#ifndef PDOS_EDIT_TEXT_H
#define PDOS_EDIT_TEXT_H
int TEDUTF8(const unsigned char *,unsigned int,unsigned char *,unsigned int,unsigned int *);
int TEDEBC(const unsigned char *,unsigned int,unsigned char *,unsigned int,unsigned int *);
int TEDCENC(const char *,unsigned int,unsigned char *);
int TEDCDEC(const unsigned char *,unsigned int,char *);
#endif
