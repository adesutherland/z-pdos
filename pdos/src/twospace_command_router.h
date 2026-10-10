/* SPDX-License-Identifier: MIT; stateless U command policy and checked K facts. */
#ifndef PDOS_COMMAND_ROUTER_H
#define PDOS_COMMAND_ROUTER_H
#define TCM_CMS 1U
#define TCM_TSO 2U
#define TCM_PDOS 3U
#define TCM_MAX 32767U
#define TCM_UNHANDLED 1U
#define TCM_CLOCK 17U
#define TCM_STAT_CMS 18U
#define TCM_STAT_TSO 19U
#define TCM_WRITE 20U
#define TCM_MEDIA 21U
#define TCM_RUN 22U
#define TCM_DONE 23U
#define TCM_SVC 197U
#define TCM_DESC_SIGNATURE 6U
#define TCM_DESC_ENTRIES 12U
#define TCM_DESC_COUNT 5U
#define TCM_DESC_BYTES 32U
typedef unsigned int (*TCMHOST)(unsigned int, unsigned char *, unsigned int);
/* All bytes use the selected C execution character set (IBM1047 on PDOS).
 * status and signed command RC are separate. No shell globals or C stdio. */
unsigned int TCMDISP(unsigned int, const unsigned char *, unsigned int,
                     TCMHOST, int *);
unsigned int TCMIO(unsigned int, unsigned char *, unsigned int);
int TCMROUTE(unsigned int *);
#endif
