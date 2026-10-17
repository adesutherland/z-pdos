/* SPDX-License-Identifier: MIT; native Programs fields over the shared controller. */
#ifndef PDOS_PROGRAMS_H
#define PDOS_PROGRAMS_H
#include "twospace_develop.h"
#include "twospace_3270.h"
unsigned int TDVPAGES(const TDVSTATE *,const T27GEOMETRY *);
int TDVPAINT(const TDVSTATE *,const T27GEOMETRY *,unsigned char *,unsigned int,unsigned int *,unsigned int,unsigned int);
int TDVINPUT(TDVSTATE *,const T27GEOMETRY *,const unsigned char *,unsigned int,const unsigned char *,unsigned int,char [199],unsigned int *);
#endif
