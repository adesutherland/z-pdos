/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pdosutil - utilities used by PDOS and possibly PLOAD             */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "pdosutil.h"

#define MAXBLKSZ 32767

int int_rdblock(int dev, int cyl, int head, int rec,
                void *buf, int len, int cmd);

#define rdblock(dev, cyl, head, rec, buf, len, cmd) \
    int_rdblock(dev, cyl, head, rec, buf, len, cmd)


/* find a file on disk */
/* 0 = success, else negative return code */

int findFileExtent(int ipldev, char *dsn, int *c, int *h, int *r,
                   int *endc, int *endh)
{
    char *raw;
    char *initial;
    char *load;
    /* Standard C programs can start at a predictable offset */
    int (*entry)(void *);
    int cyl;
    int head;
    int rec;
    int i;
    int j;
    char tbuf[MAXBLKSZ];
    char srchdsn[FILENAME_MAX+10]; /* give an extra space */
    int cnt = -1;
    int lastcnt = 0;
    int ret = 0;
    struct {
        char ds1dsnam[44];
        char ds1fmtid;
        char unused1[60];
        char unused2;
        char unused3;
        char startcchh[4];
        char endcchh[4];
    } dscb1;
    int len;
    int errcnt = 0;
    int found = 0;

    if (memchr(dsn, '\0', FILENAME_MAX) == NULL)
    {
        len = FILENAME_MAX;
    }
    else
    {
        len = strlen(dsn);
    }
    memcpy(srchdsn, dsn, len);
    strcpy(srchdsn + len, " ");
    len++; /* force a search for the blank */
    
    /* read VOL1 record which starts on cylinder 0, head 0, record 3 */
    cnt = rdblock(ipldev, 0, 0, 3, tbuf, MAXBLKSZ, 0x0e);
    if (cnt >= 20)
    {
        cyl = head = rec = 0;
        /* +++ probably time to create some macros for this */
        memcpy((char *)&cyl + sizeof(int) - 2, tbuf + 15, 2);
        memcpy((char *)&head + sizeof(int) - 2, tbuf + 17, 2);
        memcpy((char *)&rec + sizeof(int) - 1, tbuf + 19, 1);
        
        while (errcnt < 4)
        {
            cnt = rdblock(ipldev, cyl, head, rec, &dscb1, sizeof dscb1, 0x0e);
            if (cnt < 0)
            {
                errcnt++;
                if (errcnt == 1)
                {
                    rec++;
                }
                else if (errcnt == 2)
                {
                    rec = 1;
                    head++;
                }
                else if (errcnt == 3)
                {
                    rec = 1;
                    head = 0;
                    cyl++;
                }
                continue;
            }
            errcnt = 0;
            if (cnt >= sizeof dscb1)
            {
                if (dscb1.ds1fmtid == '1')
                {
                    dscb1.ds1fmtid = ' '; /* for easy comparison */
                    if (memcmp(dscb1.ds1dsnam,
                               srchdsn,
                               len) == 0)
                    {
                        cyl = head = 0;
                        rec = 1;
                        /* +++ more macros needed here */
                        memcpy((char *)&cyl + sizeof(int) - 2, 
                               dscb1.startcchh, 2);
                        memcpy((char *)&head + sizeof(int) - 2,
                               dscb1.startcchh + 2, 2);
                        found = 1;
                        break;
                    }
                }
                else if (dscb1.ds1dsnam[0] == '\0')
                {
                    cnt = -1;
                    break;
                }
            }
            rec++;
        }        
    }
    
    if (cnt <= 0 || !found)
    {
        /* not found */
        return (-1);
    }
    *c = cyl;
    *h = head;
    *r = rec;
    if (endc != NULL && endh != NULL)
    {
        *endc = *endh = 0;
        memcpy((char *)endc + sizeof(int) - 2, dscb1.endcchh, 2);
        memcpy((char *)endh + sizeof(int) - 2, dscb1.endcchh + 2, 2);
    }
    return (0);
}

int findFile(int ipldev, char *dsn, int *c, int *h, int *r)
{
    return findFileExtent(ipldev, dsn, c, h, r, NULL, NULL);
}


/* Structures here are documented in Appendix B and E of
   MVS Program Management: Advanced Facilities SA22-7644-14:
   https://publib.boulder.ibm.com/epubs/pdf/iea2b2b1.pdf
   and the IEBCOPY component is documented here:
   Appendix B of OS/390 DFSMSdfp Utilities SC26-7343-00
   or Appendix B of the z/OS equivalent SC26-7414-05 available here at
   http://publib.boulder.ibm.com/epubs/pdf/dgt2u140.pdf
   Also PDS directory blocks are documented in Chapter 26 of
   z/OS DFSMS Using Data Sets here:
   https://www.ibm.com/docs/en/SSLTBW_3.1.0/pdf/idad400_v3r1.pdf
*/

#define PE_DEBUG 0

typedef struct {
    int id;
    int base;
} PESECTION;

typedef struct {
    int start;
    int end;
} PEGAP;

static int pe16(const unsigned char *p)
{
    return ((int)p[0] << 8) | p[1];
}

static int pe24(const unsigned char *p)
{
    return ((int)p[0] << 16) | ((int)p[1] << 8) | p[2];
}

static unsigned int pe32(const unsigned char *p)
{
    return ((unsigned int)p[0] << 24) | ((unsigned int)p[1] << 16)
        | ((unsigned int)p[2] << 8) | p[3];
}

static void peput32(unsigned char *p, unsigned int value)
{
    p[0] = value >> 24;
    p[1] = value >> 16;
    p[2] = value >> 8;
    p[3] = value;
}

typedef struct {
    int offset;
} PEHIGHRELOC;

typedef struct {
    PEHIGHRELOC *items;
    int count;
    int capacity;
} PEHIGHRELOCS;

static int peHighRelocCompare(const void *left, const void *right)
{
    const PEHIGHRELOC *a = left;
    const PEHIGHRELOC *b = right;
    return (a->offset > b->offset) - (a->offset < b->offset);
}

static int processRLD(char *buf, int textlen, int rlad, char *rld, int len,
                      const PESECTION *sections, int nsections,
                      PEHIGHRELOCS *high);

static int fixPEModeBase(char *buf, int *len, int *entry, int rlad,
                         int capacity, int *module_amode,
                         int *module_rmode_any, int high)
{
    char *p;
    char *q;
    int z;
    typedef struct {
        char pds2name[8];
        char unused1[19];
        char pds2epa[3];
        char pds2ftb1;
        char pds2ftb2;
        char pds2ftb3;
    } IHAPDS;
    IHAPDS *ihapds;
    int rmode;
    int amode;
    int ent;
    int rec = 0;
    int corrupt = 1;
    int rem = *len;
    int l;
    int l2;
    int lastt = -1;
    char *lasttxt = NULL;
    char *upto = buf;
    PESECTION *sections = NULL;
    int nsections = 0;
    int sectioncap = 0;
    PEGAP *gaps = NULL;
    int ngaps = 0;
    int gapcap = 0;
    int nexttext = 0;
    int lastend = 0;
    int imageend = 0;
    PEHIGHRELOCS highrelocs = {NULL, 0, 0};
    
    if (module_amode != NULL) *module_amode = -1;
    if (module_rmode_any != NULL) *module_rmode_any = -1;
    if ((*len <= 8) || capacity < *len
        || memcmp(buf + 4, "\x00\xca\x6d\x0f", 4) != 0)
    {
        printf("Not an MVS PE executable\n");
        return (-1);
    }
#if PE_DEBUG
    printf("MVS PE total length is %d\n", *len);
#endif
    p = buf;
    while (1)
    {
        if (rem < 2) break;
        rec++;
        l = pe16((unsigned char *)p);
        /* keep track of remaining bytes, and ensure they really exist */
        if (l < 2 || l > rem)
        {
            break;
        }
        rem -= l;
#if PE_DEBUG
        printf("rec %d, offset %d is len %d\n", rec, p - buf, l);
#endif
#if 0
        if (1)
        {
            for (z = 0; z < l; z++)
            {
                printf("z %d %x %c\n", z, p[z], isprint(p[z]) ? p[z] : ' ');
            }
        }
#endif
        if (rec == 3) /* directory record */
        {
            /* there should only be one directory entry, 
               which is 4 + 276 + 12 */
            if (l < 292)
            {
                break;
            }
            q = p + 24;
            l2 = pe16((unsigned char *)q);
            if (l2 < 32) break;
            ihapds = (IHAPDS *)(q + 2);
            rmode = ihapds->pds2ftb2 & 0x10;
            /* V1R5 native directory: low two bits are AMODE; bit 4 is
               RMODE ANY. The old diagnostic-only mask read bits 2-3. */
            amode = ihapds->pds2ftb2 & 0x03;
            if ((high && (amode != 1
                          || (ihapds->pds2ftb2 & 0x30) != 0x30))
                || (module_amode != NULL
                    && (amode == 3 || (amode == 1 && !rmode))))
            {
                printf("unsupported native AMODE/RMODE directory byte %x\n",
                       ihapds->pds2ftb2 & 0xff);
                break;
            }
            if (module_amode != NULL) *module_amode = amode;
            if (module_rmode_any != NULL) *module_rmode_any = !!rmode;
            ent = pe24((unsigned char *)ihapds->pds2epa);
            *entry = high ? ent : (int)(buf + ent);
#if PE_DEBUG
            printf("module name is %.8s\n", ihapds->pds2name);
            printf("rmode is %s\n", rmode ? "ANY" : "24");
            printf("amode is ");
            if (amode == 0)
            {
                printf("24");
            }
            else if (amode == 2)
            {
                printf("31");
            }
            else if (amode == 1)
            {
                printf("64");
            }
            else if (amode == 3)
            {
                printf("ANY");
            }
            printf("\n");
            printf("entry point is %x\n", ent);
#endif            
        }
        else if (rec > 3)
        {
            int t;
            int r2;
            int l2;
            int term = 0;
            
            if (l < (4 + 12))
            {
                break;
            }
            q = p + 4 + 10;
            r2 = l - 4 - 10;
            while (1)
            {
                if (r2 < 2)
                {
                    term = 1;
                    break;
                }
                l2 = pe16((unsigned char *)q);
                r2 -= sizeof(short);
                if (l2 > r2)
                {
                    term = 1;
                    break;
                }
                r2 -= l2;

                if (l2 == 0) break;
                q += sizeof(short);
#if PE_DEBUG
                printf("load module record is of type %2x (len %5d)"
                       " offset %d\n", 
                       *q, l2, q - p);
#endif

                t = (unsigned char)*q;
                if ((lastt == 1) || (lastt == 3)
                    || (lastt == 0x0d) || (lastt == 0x0f))
                {
#if PE_DEBUG
                    printf("rectype: program text\n");
#endif
                    PEGAP *grown;

                    /* A leftward overlapping move is safe: it cannot
                     * overwrite any unconsumed input after this text. */
                    if (nexttext < lastend || nexttext > capacity
                        || l2 > capacity - nexttext
                        || buf + nexttext > q)
                    {
                        term = 1;
                        break;
                    }
                    if (nexttext > lastend)
                    {
                        if (ngaps == gapcap)
                        {
                            int newcap = gapcap ? gapcap * 2 : 8;
                            grown = realloc(gaps, newcap * sizeof *gaps);
                            if (grown == NULL)
                            {
                                term = 1;
                                break;
                            }
                            gaps = grown;
                            gapcap = newcap;
                        }
                        gaps[ngaps].start = lastend;
                        gaps[ngaps].end = nexttext;
                        ngaps++;
                    }
                    lasttxt = q;
                    memmove(buf + nexttext, q, l2);
                    lastend = nexttext + l2;
                    if (lastend > imageend) imageend = lastend;
                    upto = buf + imageend;
                    t = -1;
                    if (lastt == 0x0d || lastt == 0x0f)
                    {
                        term = 1;
                        corrupt = 0;
                        break;
                    }
                }
                else if (t == 0x20)
                {
                    int first;
                    int count;
                    int i;

                    if (l2 < 8 || pe16((unsigned char *)q + 6) > l2 - 8
                        || (pe16((unsigned char *)q + 6) % 16) != 0)
                    {
                        term = 1;
                        break;
                    }
                    first = pe16((unsigned char *)q + 4);
                    count = pe16((unsigned char *)q + 6) / 16;
                    for (i = 0; i < count; i++)
                    {
                        unsigned char *item = (unsigned char *)q + 8 + i * 16;
                        PESECTION *grown;

                        /* Native V1R5 binder emits named section type 0
                           as well as unnamed section type 4. Both own RLD
                           P indexes; ignoring type 0 loses CSECT 1. */
                        if (item[8] != 0 && item[8] != 4) continue;
                        if (nsections == sectioncap)
                        {
                            int newcap = sectioncap ? sectioncap * 2 : 16;
                            grown = realloc(sections, newcap * sizeof *sections);
                            if (grown == NULL)
                            {
                                term = 1;
                                break;
                            }
                            sections = grown;
                            sectioncap = newcap;
                        }
                        sections[nsections].id = first + i;
                        sections[nsections].base = pe24(item + 9);
                        if (sections[nsections].base > capacity
                            || pe24(item + 13)
                               > capacity - sections[nsections].base)
                        {
                            term = 1;
                            break;
                        }
                        if (sections[nsections].base + pe24(item + 13)
                            > imageend)
                        {
                            imageend = sections[nsections].base
                                + pe24(item + 13);
                        }
                        nsections++;
                    }
                    if (term) break;
                }
                else if (t == 1)
                {
                    if (l2 < 16 || (unsigned char)q[8] != 6)
                    {
                        term = 1;
                        break;
                    }
                    nexttext = pe24((unsigned char *)q + 9);
                }
                else if (t == 0x0d)
                {
                    if (l2 < 16 || (unsigned char)q[8] != 6)
                    {
                        term = 1;
                        break;
                    }
                    nexttext = pe24((unsigned char *)q + 9);
                }
                else if (t == 2)
                {
                    /* printf("rectype: RLD\n"); */
                    if (processRLD(buf, upto - buf, rlad, q, l2,
                                   sections, nsections,
                                   high ? &highrelocs : NULL) != 0)
                    {
                        term = 1;
                        break;
                    }
                }
                else if (t == 3 || t == 0x0f)
                {
                    int l3;
                    
                    /* printf("rectype: Dicionary = Control + RLD\n"); */
                    if (l2 < 16)
                    {
                        term = 1;
                        break;
                    }
                    l3 = pe16((unsigned char *)q + 6) + 16;
                    if (l3 > l2)
                    {
                        term = 1;
                        break;
                    }
#if 0
                    printf("l3 is %d\n", l3);
#endif
                    if (processRLD(buf, upto - buf, rlad, q, l3,
                                   sections, nsections,
                                   high ? &highrelocs : NULL) != 0)
                    {
                        term = 1;
                        break;
                    }
                    if (l2 < 16 || (unsigned char)q[8] != 6)
                    {
                        term = 1;
                        break;
                    }
                    nexttext = pe24((unsigned char *)q + 9);
                }
                else if (t == 0x0e)
                {
                    /* printf("rectype: Last record of module\n"); */
                    if (processRLD(buf, upto - buf, rlad, q, l2,
                                   sections, nsections,
                                   high ? &highrelocs : NULL) != 0)
                    {
                        term = 1;
                        break;
                    }
                    term = 1;
                    corrupt = 0;
                    break;
                }
                else if (t == 0x80)
                {
                    /* printf("rectype: CSECT\n"); */
                }
                else
                {
                    if (high)
                    {
                        printf("unsupported high module record %x\n", t);
                        term = 1;
                        break;
                    }
                    /* Retain the legacy low-loader record tolerance. */
                }
#if 0
                if ((t == 0x20) || (t == 2))
                {
                    for (z = 0; z < l; z++)
                    {
                        printf("z %d %x %c\n", z, q[z], 
                               isprint(q[z]) ? q[z] : ' ');
                    }
                }
#endif
                lastt = t;

                q += l2;
                if (r2 == 0)
                {
#if PE_DEBUG
                    printf("another clean exit\n");
#endif
                    break;
                }
                else if (r2 < (10 + sizeof(short)))
                {
                    /* printf("another unclean exit\n"); */
                    term = 1;
                    break;
                }
                r2 -= 10;
                q += 10;
            }
            if (term) break;            
        }
        p = p + l;
        if (rem == 0)
        {
#if PE_DEBUG
            printf("breaking cleanly\n");
#endif
        }
        else if (rem < 2)
        {
            break;
        }
    }
    free(sections);
    if (corrupt)
    {
        free(highrelocs.items);
        free(gaps);
        printf("corrupt module\n");
        return (-1);
    }
    if (imageend > capacity || imageend < lastend)
    {
        free(highrelocs.items);
        free(gaps);
        printf("module image exceeds input buffer\n");
        return (-1);
    }
    for (z = 0; z < ngaps; z++)
    {
        memset(buf + gaps[z].start, 0, gaps[z].end - gaps[z].start);
    }
    free(gaps);
    memset(buf + lastend, 0, imageend - lastend);
    if (high)
    {
        int i;
        unsigned int base = (unsigned int)rlad;
        if (*entry < 0 || *entry >= imageend || (*entry & 1))
        {
            free(highrelocs.items);
            printf("invalid high module entry offset\n");
            return (-1);
        }
        qsort(highrelocs.items, highrelocs.count,
              sizeof *highrelocs.items, peHighRelocCompare);
        /* Preflight every target before modifying even one relocated field. */
        for (i = 0; i < highrelocs.count; i++)
        {
            int off = highrelocs.items[i].offset;
            if (off < 0 || off > imageend - 8
                || (i && off < highrelocs.items[i - 1].offset + 8)
                || pe32((unsigned char *)buf + off) != 0
                || pe32((unsigned char *)buf + off + 4)
                   >= (unsigned int)imageend)
            {
                free(highrelocs.items);
                printf("invalid or overlapping high module relocation\n");
                return (-1);
            }
        }
        for (i = 0; i < highrelocs.count; i++)
        {
            unsigned char *where = (unsigned char *)buf
                + highrelocs.items[i].offset;
            unsigned int oldlo = pe32(where + 4);
            unsigned int newlo = oldlo + base;
            peput32(where, 1U + (newlo < oldlo));
            peput32(where + 4, newlo);
        }
        free(highrelocs.items);
    }
#if 0
    printf("dumping new module\n");
#endif
    *len = imageend; /* include zero-filled sections and alignment gaps */
    return (0);
}

int fixPEMode(char *buf, int *len, int *entry, int rlad, int capacity,
              int *module_amode, int *module_rmode_any)
{
    return fixPEModeBase(buf, len, entry, rlad, capacity,
                         module_amode, module_rmode_any, 0);
}

int fixPEHigh(char *buf, int *len, int *entry_offset,
              unsigned int base_low, int capacity)
{
    return fixPEModeBase(buf, len, entry_offset, (int)base_low,
                         capacity, NULL, NULL, 1);
}

int fixPE(char *buf, int *len, int *entry, int rlad, int capacity)
{
    return fixPEMode(buf, len, entry, rlad, capacity, NULL, NULL);
}


static int processRLD(char *buf, int textlen, int rlad, char *rld, int len,
                      const PESECTION *sections, int nsections,
                      PEHIGHRELOCS *high)
{
    int l;
    char *r;
    int cont = 0;
    char *fin;
    int negative;
    int ll;
    int a;
    unsigned int newval;
    unsigned char *zaploc;
    int source;
    int base;
    int i;
    
    if (len < 16) return (-1);
    r = rld + 16;
    fin = rld + len;
    while (r != fin)
    {
        if (!cont)
        {
            if (r + 4 > fin)
            {
                printf("corrupt RLD section indexes\n");
                return (-1);
            }
            source = pe16((unsigned char *)r + 2);
            base = -1;
            for (i = 0; i < nsections; i++)
            {
                if (sections[i].id == source)
                {
                    base = sections[i].base;
                    break;
                }
            }
            if (base < 0)
            {
                printf("RLD source CSECT %d has no CESD entry\n", source);
                return (-1);
            }
            r += 4; /* R and P indexes; validate P's CSECT identity */
            if (r >= fin)
            {
                printf("corrupt1 at position %x\n", r - rld - 4);
                return (-1);
            }
        }
        negative = *r & 0x02;
        if (negative)
        {
            printf("got a negative adjustment - unsupported\n");
            return (-1);
        }
        ll = ((*r & 0x0c) >> 2) + 1;
        if (*r & 0x40) ll += 4;
        if (high != NULL ? (ll != 8 || ((*r & 0x30) != 0x10
                                       && (*r & 0x30) != 0))
                         : (ll != 4 && ll != 3))
        {
            printf("untested and unsupported relocation %d\n", ll);
            return (-1);
        }
        if (ll == 3)
        {
            if (rlad > 0xffffff)
            {
                printf("AL3 prevents relocating this module to %x\n", rlad);
                return (-1);
            }
        }
        cont = *r & 0x01; /* do we have A & F continous? */
        r++;
        if ((r + 3) > fin)
        {
            printf("corrupt2 at position %x\n", r - rld);
            return (-1);
        }
        a = pe24((unsigned char *)r);
        if (a > textlen || ll > textlen - a
            || (ll == 3 && a == 0))
        {
            printf("RLD address outside program text: CSECT %d base %x offset %x size %d text %x\n",
                   source, base, a, ll, textlen);
            return (-1);
        }
        if (high != NULL)
        {
            PEHIGHRELOC *grown;
            if (high->count == high->capacity)
            {
                int newcap = high->capacity ? high->capacity * 2 : 128;
                grown = realloc(high->items, newcap * sizeof *grown);
                if (grown == NULL) return (-1);
                high->items = grown;
                high->capacity = newcap;
            }
            high->items[high->count++].offset = a;
            r += 3;
            continue;
        }
        zaploc = (unsigned char *)(buf + a - ((ll == 3) ? 1 : 0));
        newval = pe32(zaploc);
        /* printf("which means that %8x ", newval); */
        newval += (unsigned int)rlad;
        /* printf("becomes %8x\n", newval); */
        peput32(zaploc, newval);
        r += 3;
    }
    return (0);
}
