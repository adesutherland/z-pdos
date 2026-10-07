/* SPDX-License-Identifier: MIT
 * Portable C model/3270 encoding shared by K and the U presentation library.
 * Privileged I/O and U translation belong to the owning K endpoint. */
#ifndef PDOS_TWO_SPACE_CONSOLE_H
#define PDOS_TWO_SPACE_CONSOLE_H
#include "twospace_abi.h"
#define TTC_OK 0
#define TTC_BAD -1
#define TTC_UNSUPPORTED -2
#define TTC_MAX_CELLS 3564U
#define TTC_MAX_STREAM TSA_SCREEN_BYTES
#define TTC_BASIC_ATTRIBUTES 1U
#define TTC_PROTECTED 0x30U
#define TTC_BRIGHT 0x38U
#define TTC_INPUT 0U
#define TTC_CONFIG_REAL 0x95000U
#define TTC_MONITOR_REAL 0x60000U
#define TTC_CONFIG_MAGIC 0x434f4e31U
typedef struct {
    unsigned int device_class,address,model,rows,columns,encoding,generation;
    unsigned int monitor,features;
} TTCCAP;
typedef struct {
    unsigned int row,column,attributes,length;
    const unsigned char *text;
} TTCFIELD;
typedef struct {
    TTCCAP cap;
    unsigned int body_row,body_rows,entry_row,entry_capacity,row,column;
    unsigned char cells[TTC_MAX_CELLS];
} TTCVIEW;
int TTCCONFIG(TTCCAP *cap,unsigned int device_class,unsigned int address,
               unsigned int model,unsigned int monitor);
int TTCCAPS(const TTCCAP *cap,unsigned char output[TSA_CAP_BYTES]);
int TTCADDR(const TTCCAP *cap,unsigned int address,unsigned char output[2]);
int TTCDECODE(const TTCCAP *cap,const unsigned char input[2],unsigned int *address);
int TTCENCODE(const TTCCAP *cap,const TTCFIELD *fields,unsigned int count,
               unsigned int cursor_row,unsigned int cursor_column,
               unsigned char *output,unsigned int capacity,unsigned int *length);
int TTCRAW(const TTCCAP *cap,const unsigned char *input,unsigned int length,
            unsigned int *command);
int TTCINPUT(const TTCCAP *cap,const unsigned char *input,unsigned int bytes,
              unsigned int field_address,unsigned char *output,unsigned int capacity,
              unsigned int *length,unsigned int *aid);
int TTCVINIT(TTCVIEW *view,const TTCCAP *cap);
int TTCVLINE(TTCVIEW *view,const unsigned char *text,unsigned int length);
int TTCVSCREEN(const TTCVIEW *view,const unsigned char *header,unsigned int header_bytes,
                const unsigned char *footer,unsigned int footer_bytes,
                unsigned char *output,unsigned int capacity,unsigned int *length);
#endif
