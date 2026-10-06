/* SPDX-License-Identifier: MIT
 * Console-only native boundary for the diagnostic U PCOMM profile.
 * PDPCLIB's maintained C stdio and system() remain the implementation.
 * Non-console datasets fail explicitly; this is not the file-service port.
 */
#include <stddef.h>
#include <string.h>

extern unsigned int TSUTPUT(unsigned int bytes, const unsigned char *data);
extern unsigned int TSUTGET(unsigned int bytes, unsigned char *data);
typedef struct { unsigned int open, input; unsigned char data[264]; } TSUFILE;
static TSUFILE files[3];

void *tsuopen(const char *dd, int *mode, int *recfm, int *lrecl,
              int *blksize, void **asmbuf, const char *member)
{
    unsigned int slot;
    if (!dd || !mode || !recfm || !lrecl || !blksize || !asmbuf || member)
        return (void *)-8;
    if (memcmp(dd,"SYSIN   ",8)==0 && (*mode&3)==0) slot=0;
    else if (memcmp(dd,"SYSPRINT",8)==0 && (*mode&3)==1) slot=1;
    else if (memcmp(dd,"SYSTERM ",8)==0 && (*mode&3)==1) slot=2;
    else return (void *)-8;
    if (files[slot].open) return (void *)-8;
    files[slot].open=1; files[slot].input=slot==0;
    *mode=(*mode&3)|(0x40<<16); *recfm=1; *lrecl=256; *blksize=256;
    *asmbuf=files[slot].data;
    return &files[slot];
}

int tsuread(void *handle, void *buffer, size_t *length)
{
    TSUFILE *file=(TSUFILE *)handle;
    unsigned int bytes;
    if (file!=&files[0] || !file->open || !buffer || !length) return 8;
    bytes=TSUTGET(252U,file->data+4);
    if (bytes>252U) return 8;
    file->data[0]=(unsigned char)((bytes+4U)>>8);
    file->data[1]=(unsigned char)(bytes+4U);
    file->data[2]=file->data[3]=0;
    *(unsigned char **)buffer=file->data; *length=bytes+4U;
    return 0;
}

int tsuwrite(void *handle, unsigned char **buffer, size_t *length)
{
    TSUFILE *file=(TSUFILE *)handle;
    unsigned int bytes, at;
    if ((file!=&files[1] && file!=&files[2]) || !file->open ||
        !buffer || !*buffer || !length || *length<4U || *length>256U)
        return 8;
    bytes=((unsigned int)(*buffer)[0]<<8)|(*buffer)[1];
    if (bytes<4U || bytes>*length || bytes>256U) return 8;
    bytes-=4U;
    for (at=0U; at<bytes; at+=130U)
        if (TSUTPUT(bytes-at>130U ? 130U : bytes-at,*buffer+4U+at)) return 8;
    return 0;
}

void tsuclose(void *handle)
{
    unsigned int i;
    for (i=0U; i<3U; ++i) if (handle==&files[i]) files[i].open=0;
}

int tsudyn(size_t ddbytes, const char *dd, size_t namebytes, const char *name)
{ (void)ddbytes; (void)dd; (void)namebytes; (void)name; return 4; }
