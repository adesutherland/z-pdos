/* SPDX-License-Identifier: MIT; portable editable3270 page codec. */
#ifndef PDOS_EDIT_SCREEN_H
#define PDOS_EDIT_SCREEN_H
#include "edit_session.h"
#include "twospace_3270.h"
#define TES_MAX 40000U
int TESPAINT(const TEDSTATE *,const T27GEOMETRY *,unsigned char *,unsigned int,unsigned int *,unsigned int);
int TESINPUT(TEDSTATE *,const T27GEOMETRY *,const unsigned char *,unsigned int,char [257],unsigned int *,unsigned int *);
/* Recover exact positions (including interior nulls) for modified fields. */
int TESFULL(const T27GEOMETRY *,const unsigned char *,unsigned int,const unsigned char *,unsigned int,unsigned char *,unsigned int,unsigned int *);
#endif
