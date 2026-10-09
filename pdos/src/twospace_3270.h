/* SPDX-License-Identifier: MIT
 * Supplied-storage 3270 family wire services. These are host-driver codecs,
 * not a terminal emulator. Device execution and optional facilities remain
 * distinct from framing and transfer. See CONSOLE-STANDARD.md. */
#ifndef PDOS_TWO_SPACE_3270_H
#define PDOS_TWO_SPACE_3270_H
#define T27_OK 0
#define T27_BAD -1
#define T27_UNSUPPORTED -2
#define T27_END 1
#define T27_CODED12 1U
#define T27_BINARY14 2U
#define T27_BINARY16 3U
#define T27_EVENT_KEY 1U
#define T27_EVENT_STRUCTURED 2U
#define T27_EVENT_UNFORMATTED 3U
typedef struct {
    unsigned int rows,columns,cells,addressing;
} T27GEOMETRY;
typedef struct {
    unsigned int id,offset,length,header;
    const unsigned char *data;
} T27FIELD;
typedef struct {
    unsigned int address,offset,length;
} T27INPUTFIELD;
typedef struct {
    unsigned int kind,aid,cursor_valid,cursor,fields;
} T27INPUTEVENT;
typedef struct {
    unsigned char replies[32];
    unsigned int usable_valid,rows,columns,addressing,hardcopy,page_printer;
    unsigned int implicit_valid,default_rows,default_columns;
    unsigned int alternate_rows,alternate_columns;
    unsigned int printer_valid,default_buffer,alternate_buffer;
} T27CAPABILITIES;
int T27GEOM(T27GEOMETRY *,unsigned int,unsigned int,unsigned int);
int T27ADDR(const T27GEOMETRY *,unsigned int,unsigned char[2]);
int T27DECD(const T27GEOMETRY *,const unsigned char[2],unsigned int *);
int T27CMAP(unsigned int,unsigned int *,unsigned int *);
/* A zero length SF extends to the end of the enclosing transmission.
 * Iteration retains unknown fields; interpretation must be explicit. */
int T27WALK(const unsigned char *,unsigned int,unsigned int *,T27FIELD *);
int T27PACK(unsigned int,const unsigned char *,unsigned int,
              unsigned char *,unsigned int,unsigned int *);
int T27WRITE(const T27GEOMETRY *,const unsigned char *,unsigned int);
int T27QUERY(unsigned char *,unsigned int,unsigned int *);
/* Returned capability bits are device replies, not PDOS implementation bits.
 * Preserve the complete raw reply separately for feature-specific clients. */
int T27QRPLY(const unsigned char *,unsigned int,T27CAPABILITIES *);
int T27HAS(const T27CAPABILITIES *,unsigned int);
/* Input fields describe ranges in the original record, retaining SA/GE and
 * DBCS bytes. The caller owns that record. Validation precedes publication. */
int T27INPUT(const T27GEOMETRY *,const unsigned char *,unsigned int,
               T27INPUTEVENT *,T27INPUTFIELD *,unsigned int);
#endif
