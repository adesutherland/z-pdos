.COMSWA  SPACE 1
ZEROES   DS    F             CONSTANT
DDWBLOCK DS    F             MAXIMUM BUFFER SIZE NEEDED
DDWFLAGS DS    X             RESULT FLAGS FOR ALL
DDWFLAG1 DS    X             RESULT FLAGS FOR FIRST DD
DDWFLAG2 DS    X             RESULT FLAGS FOR ALL BUT FIRST
CWFDD    EQU   X'80'           FOUND A DD
CWFCONC  EQU   CWFDD           AFTER FLAG MERGE - CONCATENATED
CWFSEQ   EQU   X'40'           USE IS SEQUENTIAL
CWFPDQ   EQU   X'20'           DS IS PDS WITH MEMBER NAME
CWFPDS   EQU   X'10'           DS IS PDS (or PDS/E with S390)
CWFVSM   EQU   X'08'           DS IS VSAM (limited support)
CWFVTOC  EQU   X'04'           DS IS VTOC (limited support)
CWFBLK   EQU   X'02'           DD HAS FORCED BLKSIZE
OPERF    DS    X             ERROR CONDITIONS
ORFBADNM EQU   04            DD name <= blank
ORFNODD  EQU   08            DD name not found in TIOT
ORFNOJFC EQU   12            Error getting JFCB
ORFNODS1 EQU   16            Error getting DSCB 1
ORFBATIO EQU   20            Unusable TIOT entry
ORFBADSO EQU   24            Invalid or unsupported DSORG
ORFBADCB EQU   28            Invalid DCB parameters
ORFBATY3 EQU   32            Unsupported unit type (Graf, Comm, etc.)
ORFBACON EQU   36            Invalid concatenation
ORFBDMOD EQU   40            Invalid MODE for DD/function
ORFBDPDS EQU   44            PDS not initialized
ORFBDDIR EQU   48            PDS not initialized
ORFNOSTO EQU   52            Out of memory
ORFNOMEM EQU   68            Member not found (BLDL/FIND)
ORFBDMEM EQU   72            Member not permitted (seq.)
ORFTOBIG EQU   96            EXTEND to more than 64KIB tracks
         SPACE 1
TRUENAME DS    CL44               DS name for alias on DD
CATWORK  DS    ((265+7)/8)D'0'    LOCATE work area
         ORG   CATWORK            Redefine returned data
CAWCOUNT DS    H                  Number of entries returned
CAW#VOL  DS    H                  Number of volumes in this DS
CAWTYPE  DS    XL4                Unit type
CAWSER   DS    CL6                Volume serial
CAWFILE  DS    XL2                Tape file number
         ORG   ,
OPENLEN  EQU   *-WORKAREA         Length for @@AOPEN processing
         SPACE 2
***********************************************************************
*                                                                     *
* ZDCBAREA - the address of this memory is used by the C caller       *
* as a "handle". The block of memory has different contents depending *
* on what sort of file is being opened, but it will be whatever the   *
* assembler code is expecting, and the caller merely needs to         *
* provide the handle (returned from AOPEN) in subsequent calls so     *
* that the assembler can keep track of things.                        *
*                                                                     *
*   FILE ACCESS CONTROL BLOCK (N.B.-STARTS WITH DCBD DUE TO DSECT)    *
*   CONTAINS DCB, DECB, JFCB, DSCB 1, BUFFER POINTERS, FLAGS, ETC.    *
*                                                                     *
***********************************************************************
         DCBD  DSORG=PS,DEVD=(DA,TA)   Map Data Control Block
         ORG   IHADCB             Overlay the DCB DSECT
ZDCBAREA DS    0H
         DS    CL(BPAMDCBL)       Room for BPAM DCB
         READ  DECB,SF,IHADCB,2-2,3-3,MF=L  READ/WRITE BSAM
*DEFUNCT ORG   IHADCB             Only using one DCB
*DEFUNCT DS    CL(QSAMDCBL)         so overlay this one
         ORG   IHADCB             Only using one DCB
         DS    CL(BSAMDCBL)
         ORG   IHADCB             Only using one DCB
         AIF   ('&OS' EQ 'PDOS' OR '&OS' EQ 'MVSF').PDVAREA
ZAACB    DS    CL(VSAMDCBL)       VSAM ACB
ZARPL    RPL   ACB=ZAACB,OPTCD=(SEQ,SYN,LOC)
ZAMODCB  DS    XL(ZAMODCBL)  MODCB WORK AREA
ZASHOCB  DS    XL(ZASHOCBL)  SHOCB WORK AREA
ZAARG    DS    A                  Pointer
ZARRN    DS    F                  Relative record number
.PDVAREA ANOP
         SPACE 2
         ORG   IHADCB             Only using one DCB
TAPEDCB  DCB   DDNAME=TAPE,MACRF=E,DSORG=PS,REPOS=Y,BLKSIZE=0,         *
               DEVD=TA                 LARGE SIZE
         ORG   TAPEDCB+84    LEAVE ROOM FOR DCBLRECL
ZXCPVOLS DC    F'0'          VOLUME COUNT
TAPECCW  CCW   1,3-3,X'40',4-4
         CCW   3,3-3,X'20',1
TAPEXLEN EQU   *-TAPEDCB     PATTERN TO MOVE
TAPEECB  DC    A(0)
TAPEIOB  DC    X'42,00,00,00'
         DC    A(TAPEECB)
         DC    2A(0)
         DC    A(TAPECCW)
         DC    A(TAPEDCB)
         DC    2A(0)
         SPACE 1
         ORG   IHADCB
         AIF   ('&OS' EQ 'PDOS').PDTAREA
ZPUTLINE PUTLINE MF=L        PATTERN FOR TERMINAL I/O
*DSECT*  IKJIOPL ,
         SPACE 1
ZIOPL    DS    0A            MANUAL EXPANSION TO AVOID DSECT
IOPLUPT  DS    A        PTR TO UPT
IOPLECT  DS    A        PTR TO ECT
IOPLECB  DS    A        PTR TO USER'S ECB
IOPLIOPB DS    A        PTR TO THE I/O SERVICE RTN PARM BLOCK
ZIOECB   DS    A                   TPUT ECB
ZIOECT   DS    A                   ORIGINATING ECT
ZIOUPT   DS    A                   UPT
ZIODDNM  DS    CL8      DD NAME AT OFFSET X'28' FOR DCB COMPAT.
ZGETLINE GETLINE MF=L             TWO WORD GTPB
.PDTAREA ANOP
         SPACE 2
*   VTOC READ ACCESS - INTERLEAVE WITH BSAM DCB
*
         ORG   IHADCB
ZVCPVOL  DS    H                  Cylinder per volume
ZVTPCYL  DS    H                  Tracks per cylinder
ZVLOCCHH DS    XL4     1/3        CCHH of VTOC start
ZVHICCHH DS    XL4     2/3        CCHH of VTOC end
ZVHIREC  DS    X       3/3        High record on track
         DS    0H                 Align for STH
ZVUSCCHH DS    XL5                Address of current record
ZVSER    DS    CL6                Volume serial
ZVSEEK   CAMLST SEEK,1-1,2-2,3-3  CAMLST to SEEK by address
         SPACE 2
         ORG   ,
OPENCLOS DS    A                  OPEN/CLOSE parameter list
DCBXLST  DS    2A                 07 JFCB / 85 DCB EXIT
EOFR24   DS    CL(EOFRLEN)
         DS    0A                 Ensure correct DC A alignment
         AIF   ('&ZSYS' EQ 'S370').NOSB   Only S/380+90 needs a stub
A24STUB  DS    CL(PATSTUBL)       DCB open exit 24-bit code
.NOSB    ANOP  ,                  Only S/390 needs a stub
ZBUFF1   DS    A,F                Address, length of buffer
ZBUFF2   DS    A,F                Address, length of 2nd buffer
KEPTREC  DS    A,F                Address & length of saved rcd
*
         MAPSUPRM DSECT=NO,PFX=ZP      MAP MODE WORK AREA
BUFFADDR DS    A     1/3          Location of the BLOCK Buffer
BUFFCURR DS    A     2/3          Current record in the buffer
BUFFEND  DS    A     3/3          Address after end of current block
VBSADDR  DS    A                  Location of the VBS record build area
VBSEND   DS    A                  Addr. after end VBS record build area
         SPACE 1
ZWORK    DS    D             Below the line work storage
ZDDN     DS    CL8           DD NAME
ZMEM     DS    CL8           MEMBER NAME or nulls
DEVINFO  DS    2F                 UCB Type / Max block size
ZTTR     DS    A             Last TTR written (BSAM, EXCP)
         SPACE 1
RECFMIX  DS    X             Record format index: 0-F 4-V 8-U
IXFIX    EQU   0               Recfm = F
IXVAR    EQU   4               Recfm = V
IXUND    EQU   8               Recfm = U
         SPACE 1
ZDVTYPE  DS    X      1/4    Device type of first/only DD
         SPACE 1
ZRECFM   DS    X      2/4    Equivalent RECFM bits
         SPACE 1
IOSFLAGS DS    X      3/4    Additional MODE related flags
IOFVSAM  EQU   X'04'           Use VSAM
         SPACE 1
IOMFLAGS DS    X      4/4    Remember open MODE
IOFTERM  EQU   X'80'           Using GETLINE/PUTLINE
IOFBPAM  EQU   X'20'           Unlike BPAM concat - special handling
IOFBLOCK EQU   X'10'           Using BSAM READ/WRITE mode
IOFEXCP  EQU   X'08'           Use EXCP for TAPE
IOFOUT   EQU   X'01'           Output mode
         SPACE 1
IOPFLAGS DS    X             Remember prior events
IOFLEOF  EQU   X'80'           Encountered an End-of-File
IOFLSDW  EQU   X'40'           Spanned record incomplete
IOFLDATA EQU   X'20'           Output buffer has data
IOFLIOWR EQU   X'10'           Last I/O was Write type
IOFCURSE EQU   X'08'           Write buffer recursion
IOFDCBEX EQU   X'04'           DCB exit entered
IOFCONCT EQU   X'02'           Reread - unlike concatenation
IOFKEPT  EQU   X'01'           Record info kept
         SPACE 1
FILEMODE DS    X             AOPEN requested record format default
FMFIX    EQU   0               Fixed RECFM (blocked)
FMVAR    EQU   1               Variable (blocked)
FMUND    EQU   2               Undefined
ZDDFLAGS DS    X             RESULT FLAGS FOR ALL
TRKLIST  TRKCALC FUNCTN=TRKCAP,UCB=(R3),BALANCE=*,RKDD=TKRKDD,         *
               REGSAVE=YES,MF=L            GET BLOCKS PER TRACK
ZIOSAVE2 DS    18F           Save area for physical write
SAVEADCB DS    18F                Register save area for PUT
ZDCBLEN  EQU   *-ZDCBAREA
*
* End of handle/DCB area
*
*
*
         SPACE 2
         PRINT NOGEN
         IHAPSA ,            MAP LOW STORAGE
         CVT DSECT=YES
         IKJTCB ,            MAP TASK CONTROL BLOCK
         AIF   ('&OS' EQ 'PDOS').PDTSMAP
         IKJECT ,            MAP ENV. CONTROL BLOCK
         IKJPTPB ,           PUTLINE PARAMETER BLOCK
         IKJCPPL ,
         IKJPSCB ,
.PDTSMAP ANOP
         IEZJSCB ,
         IEZIOB ,
         IEFZB4D0 ,          MAP SVC 99 PARAMETER LIST
         IEFZB4D2 ,          MAP SVC 99 PARAMETERS
MYUCB    DSECT ,
         IEFUCBOB ,
MYTIOT   DSECT ,
         IEFTIOT1 ,
         IEZDEB ,
         IHAPDS PDSBLDL=YES
         SPACE 1
         AIF   ('&OS' EQ 'PDOS' OR '&OS' EQ 'MVSF').PDVSMAP
         IFGACB ,
         SPACE 1
         IFGRPL ,
.PDVSMAP ANOP
         AIF   ('&OS' EQ 'PDOS').PDJSMAP
         IEFJESCT ,
.PDJSMAP ANOP
         AIF   ('&OS' EQ 'PDOS').PDUPEND
         IKJUPT ,
.PDUPEND ANOP
R0       EQU   0             NO STANDARD REGEQU MACRO
R1       EQU   1             NO STANDARD REGEQU MACRO
R2       EQU   2             NO STANDARD REGEQU MACRO
R3       EQU   3             NO STANDARD REGEQU MACRO
R4       EQU   4             NO STANDARD REGEQU MACRO
R5       EQU   5             NO STANDARD REGEQU MACRO
R6       EQU   6             NO STANDARD REGEQU MACRO
R7       EQU   7             NO STANDARD REGEQU MACRO
R8       EQU   8             NO STANDARD REGEQU MACRO
R9       EQU   9             NO STANDARD REGEQU MACRO
R10      EQU   10            NO STANDARD REGEQU MACRO
R11      EQU   11            NO STANDARD REGEQU MACRO
R12      EQU   12            NO STANDARD REGEQU MACRO
R13      EQU   13            NO STANDARD REGEQU MACRO
R14      EQU   14            NO STANDARD REGEQU MACRO
R15      EQU   15            NO STANDARD REGEQU MACRO
         END   ,
