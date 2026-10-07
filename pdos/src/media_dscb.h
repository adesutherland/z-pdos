/* PDOS dataset layout, originally by Paul Edwards. Inherited licence. */
#ifndef PDOS_MEDIA_DSCB_H
#define PDOS_MEDIA_DSCB_H
typedef struct {
    char ds1dsnam[44]; /* dataset name */
    char ds1fmtid; /* must be set to '1' */
    char ds1dssn[6]; /* volser */
    char ds1volsq[2]; /* volume sequence number */
    char ds1credt[3]; /* creation date */
    char ds1expdt[3]; /* expiry date */
    char ds1noepv; /* number of extents */
        /* The count does not include the label track, if any. If the
           first extent type is x'40', then the data set has LABEL=SUL, and
           in (MVS) limited to 15 extents. */

    char ds1nobdb; /* number of free bytes in the last block of a
                      PDS. Hercules currently using random values
                      like 0x78 */
    char ds1flag1;
    char ds1syscd[13]; /* system code - presumably name of operating system */
    char ds1refd[3]; /* reference date - ie last time accessed */
    char ds1smsfg;
    char ds1scxtf;
    short ds1scxtv;
    short ds1dsorg; /* dataset organization, e.g. PS or PO */
    char ds1recfm; /* record format, e.g. F/V/U */
    char ds1optcd; /* mainly used for BDAM and ISAM */
    short ds1blkl; /* block size */
    short ds1lrecl; /* logical record length */
    char ds1keyl;
    char ds1rkp[2];
    char ds1dsind; /* flags, e.g. is this the last volume for this dataset? */
    char ds1scal1; /* is this a cylinder request? */
    char ds1scal3[3]; /* size of secondary allocation */
    char ds1lstar[3]; /* last TTR actually used - ie the EOF record */
    char ds1trbal[2]; /* TRKCALC??? */
    char resv1; /* reserved */
    char ds1ttthi; /* a extra byte for TTR number, ie make lstart TTTR */
    char ds1ext1[2]; /* first extent - actualy 10 bytes, but we use
                        different names currently. First byte is an
                        extent indicator, second byte is the extent
                        sequence number. Then we have start CCHH and
                        end CCHH (4 bytes each) */
    char startcchh[4];
    char endcchh[4];
    char ds1ext2[10]; /* second extent */
    char ds1ext3[10]; /* third extent */
    char ds1ptrds[5]; /* CCHHR pointing to a format-3
                         DSCB which allows unlimited chaining so your
                         dataset can grow to fill the disk */
    /* format 3 layout can be found here:
https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.1.0/com.ibm.zos.v2r1.idas300/s3022.htm
    */
} DSCB1;
#endif
