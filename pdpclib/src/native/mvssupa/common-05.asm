         OI    NEEDBF,NEEDBANY  set flag to say we need BSM switching
         B     RETURNSU
*
.NOSETUP ANOP  ,     Mode switching only relevant to S/380-style
RETURNSU DS    0H
         LA    R15,0
         RETURN (14,12),RC=(15)
         LTORG ,
NEEDBF   DC    X'00'   flag bits for whether BSM needed
NEEDBANY EQU   X'01'   need any amode switching at all?
NEEDBOA  DC    A(0)    amode bits to be ORed in to return APP to
*                      original amode
NEEDBOO  DC    A(0)    amode bits to be ORed in to set OS amode
*
***********************************************************************
***********************************************************************
*                                                                     *
* End of functions, start of data areas                               *
*                                                                     *
***********************************************************************
***********************************************************************
         SPACE 2
*
***********************************************************************
*                                                                     *
*  The work area includes both a register save area and various       *
*  variables used by the different routines.                          *
*                                                                     *
***********************************************************************
WORKAREA DSECT
SAVEAREA DS    18F
DWORK    DS    D                  Extra work space
WWORK    DS    D                  Extra work space
DWDDNAM  DS    D                  Extra work space
WORKLEN  EQU   *-WORKAREA
PARM1    DS    A             FIRST PARM     DD NAME
PARM2    DS    A              NEXT PARM     I/O MODE
PARM3    DS    A              NEXT PARM     FORMAT (F, V, U)
PARM4    DS    A              NEXT PARM     RECORD LEN
PARM5    DS    A              NEXT PARM     BLOCK SIZE
PARM6    DS    A              NEXT PARM     opt. BUFFER
PARM7    DS    A              NEXT PARM     MEMBER NAME
SAVOSUB  DS    6A         R10-R15 Return saver for AOPEN subs
MYJFCB   DS    0F
         IEFJFCBN LIST=YES        Job File Control Block
CAMLST   DS    XL(CAMLEN)         CAMLST for OBTAIN to get VTOC entry
* Format 1 Data Set Control Block
*   N.B. Current program logic does not use DS1DSNAM, leaving 44 bytes
*     of available space
         IECSDSL1 1               Map the Format 1 DSCB
DSCBCCHH DS    CL5                CCHHR of DSCB returned by OBTAIN
         DS    CL47               Rest of OBTAIN's 148 byte work area
         ORG   DS1FMTID
         IECSDSL1 4               Redefine for VTOC
         ORG   ,
         SPACE 1
*   DEFINITIONS TO ALLOW ASSEMBLY AND TESTING OF SMS, ETC. UNDER
*   MVS 3.n
*
FM1FLAG  EQU   DS1NOBDB+1,1,C'X'  MORE FLAGS
FM1COMPR EQU   X'80'           COMPRESSABLE EXTENDED IF DS1STRP
FM1CPOIT EQU   X'40'           CHECKPOINTED D S
FM1SMSFG EQU   FM1FLAG+17,1,C'X' SMS flag at DSCB byte 78
FM1SMSDS EQU   X'80'           SMS D S
FM1SMSUC EQU   X'40'           NO BCS ENTRY
FM1REBLK EQU   X'20'           MAY BE REBLOCKED
FM1CRSDB EQU   X'10'           BLKSZ BY DADSM
FM1PDSE  EQU   X'08'           PDS/E
FM1STRP  EQU   X'04'           EXTENDED FORMAT D S
FM1PDSEX EQU   X'02'           HFS D S
FM1DSAE  EQU   X'01'           EXTENDED ATTRIBUTES EXISY
FM1SCEXT EQU   FM1SMSFG+1,3,C'X'  SECONDARY SPACE EXTENSION
FM1SCXTF EQU   FM1SCEXT,1,C'X'  -"- FLAG
FM1SCAVB EQU   X'80'           SCXTV IS AVG BLOCK LEN
FM1SCMB  EQU   X'40'                 IS IN MEGBYTES
FM1SCKB  EQU   X'20'                 IS IN KILOBYTES
FM1SCUB  EQU   X'10'                 IS IN BYTES
FM1SCCP1 EQU   X'08'           SCXTV COMPACTED BY 256
FM1SCCP2 EQU   X'04'                 COMPACTED BY 65536
FM1SCXTV EQU   FM1SCXTF+1,2,C'X'  SEC SPACE EXTNSION VALUE
FM1ORGAM EQU   DS1ACBM         CONSISTENT NAMING - VSAM D S
FM1RECFF EQU   X'80'           RECFM F
FM1RECFV EQU   X'40'           RECFM V
FM1RECFU EQU   X'C0'           RECFM U
FM1RECFT EQU   X'20'           RECFM T   001X XXXX IS D
FM1RECFB EQU   X'10'           RECFM B
FM1RECFS EQU   X'08'           RECFM S
FM1RECFA EQU   X'04'           RECFM A
FM1RECMC EQU   X'02'           RECFM M
*   OPTCD DEFINITIONS   BDAM    W.EFA..R
*                       ISAM    WUMIY.LR
*             BPAM/BSAM/QSAM    WUCHBZTJ
FM1OPTIC EQU   X'80'  FOR DS1ORGAM - CATLG IN ICF CAT
FM1OPTBC EQU   X'40'           ICF CATALOG
FM1RACDF EQU   DS1IND40
FM1SECTY EQU   DS1IND10
FM1WRSEC EQU   DS1IND04
FM1SCAL1 EQU   DS1SCALO,1,C'X'    SEC. ALLOC FLAGS
FM1DSPAC EQU   X'C0'         SPACE REQUEST MASK
FM1CYL   EQU   X'C0'           CYLINDER BOUND
FM1TRK   EQU   X'80'           TRACK
FM1AVRND EQU   X'41'           AVG BLOCK + ROUND
FM1AVR   EQU   X'40'           AVG BLOCK LEN
FM1MSGP  EQU   X'20'
FM1EXT   EQU   X'10'           SEC. EXTENSION EXISTS
FM1CONTG EQU   X'08'           REQ. CONTIGUOUS
FM1MXIG  EQU   X'04'           MAX
FM1ALX   EQU   X'02'           ALX
FM1DSABS EQU   X'00'           ABSOLUTE TRACK
FM1SCAL3 EQU   FM1SCAL1+1,3,C'X'  SEC ALLOC QUANTITY
         SPACE 1
DDWATTR  DS    16XL8         DS ATTRIBUTES (DSORG,RECFM,X,LRECL,BLKSI)
BLDLLIST DS    Y(1,12+2+31*2)     BLDL LIST HEADER
BLDLNAME DS    CL8' ',XL(4+2+31*2)    MEMBER NAME AND DATA
