MVSSUPA  TITLE 'M V S S U P A  ***  MVS VERSION OF PDPCLIB SUPPORT'
***********************************************************************
*                                                                     *
*  This program written by Paul Edwards.                              *
*  Released to the public domain                                      *
*                                                                     *
*  Extensively modified by others                                     *
*                                                                     *
***********************************************************************
*                                                                     *
*  MVSSUPA - Support routines for PDPCLIB under MVS                   *
*    Additional macros in (EDWARDS.)PDPCLIB.MACLIB                    *
*  It is currently coded for GCC, but IBM C functionality is          *
*  still there, it's just not being tested after each change.         *
*                                                                     *
***********************************************************************
*                                                                     *
* Note that some of the functionality in here has not been exercised  *
* to any great extent, since it is dependent on whether the C code    *
* invokes it or not.                                                  *
*                                                                     *
* Note that this code issues WTOs. It should be changed to just       *
* set a return code and exit gracefully instead.                      *
*                                                                     *
***********************************************************************
*   Changes by Gerhard Postpischil:
*     EQU * for entry points deleted (placed labels on SAVE) to avoid
*       0C6 abends when EQU follows a LTORG
*     Fixed 0C4 abend in RECFM=Vxxx processing; fixed PUT length error.
*     Deleted unnecessary and duplicated instructions
*     Added @@SYSTEM and @@DYNAL routines                2008-06-10
*     Added @@IDCAMS non-reentrant, non-refreshable      2008-06-17
*     Modified I/O for BSAM, EXCP, and terminal I/O
*     Added checks to AOPEN to support unlike PDS BLDL   2014-03-03
*     Caller may use @@ADCBA any time to get current DCB attributes.
*     Added support for unlike PDS concatenation; requires member.
*     Fixed problems with sequential unlike concatenation:
*       When next DD is entered, AREAD returns with R15=4, no data.
*       Use @@ADCBA to get attributes for next DD.
*     Largest blocksize will be used for buffer.         2014-07-24
*     The program now supports reading the VTOC of a disk pack;
*     use @@AOPEN, @@ACLOSE, @@AREAD normally for a sequential data
*       set with record length 140 (44 key, 96 data). The DD card:
*       //ddname DD DISP=OLD,DSN=FORMAT4.DSCB,UNIT=SYSDA,
*       //           VOL=SER=serial                      2014-08-01
*
***********************************************************************
*
*   To facilitate cross-assembly (S390 on S370/380 system), some
*   OS/390 & zOS macros replaced. Affected old statements are flagged
*   *COMP*                                               2015-01-06
*
***********************************************************************
*
*
* Internal macros (these replace external macro library)
*
*
*
         MACRO ,
&NM      AMUSE &WRK1=R14,&WRK2=R15
         GBLC  &ZSYS
.*
.*   AMUSE sets addressing mode back to the caller's
.*         Expands nothing or label for S370 or S390
.*         Required after SAM24 call to return data to caller
.*
         AIF   ('&ZSYS' NE 'S380' AND '&ZSYS' NE 'ZARCH').OTHSYS
&NM      L     &WRK1,4(,R13)      Old save area
         L     &WRK1,12(,&WRK1)   Caller's mode in high bit
         N     &WRK1,=X'80000000'   Kill address
         LA    &WRK2,*+4+2+2      Get new mode and address
         OR    &WRK1,&WRK2
         BSM   R0,&WRK1           CONTINUE IN USER MODE
         MEXIT ,
.OTHSYS  AIF   ('&NM' EQ '').MEND
&NM      DS    0H            DEFINE LABEL ONLY
.MEND    MEND  ,
*
*
*
         MACRO ,
&NM      GAMOS
         GBLC  &STEPD
.*
.*   GAMOS sets addressing mode to 24 or 31 or
.*   potentially bypasses the BSM
.*
         AIF   ('&STEPD' NE 'YES').OTHSYS2
&NM      DS    0H
         L     R15,=A(NEEDBF)
         TM    0(R15),NEEDBANY   Need AM switching?
*         TM    NEEDBF,NEEDBANY   Need AM switching?
         BZ    ZZ&SYSNDX.X
         LA    R14,ZZ&SYSNDX.X
         L     R15,=A(NEEDBOO)
         L     R15,0(,R15)
         OR    R14,R15    whatever mode OS requires
*         O     R14,NEEDBOO whatever mode OS requires
         BSM   0,R14
ZZ&SYSNDX.X DS 0H
         MEXIT ,
.OTHSYS2 AIF   ('&NM' EQ '').MEND
&NM      DS    0H            DEFINE LABEL ONLY
.MEND    MEND  ,
*
*
*
         MACRO ,
&NM      GAMAPP
         GBLC  &STEPD
.*
.*   GAMAPP sets addressing mode to 31 or 64 or
.*   potentially bypasses the BSM
.*
         AIF   ('&STEPD' NE 'YES').OTHSYS3
&NM      DS    0H
         L     R15,=A(NEEDBF)
         TM    0(R15),NEEDBANY   Did we previously need a switch?
*         TM    NEEDBF,NEEDBANY   Did we previously need a switch?
         BZ    ZZ&SYSNDX.X       No, no switching required
.* We don't know whether we need to switch to 31 or 64
         LA    R14,ZZ&SYSNDX.X
         L     R15,=A(NEEDBOA)
         L     R15,0(,R15)
         OR    R14,R15      This decides 31/64
*         O     R14,NEEDBOA  This decides 31/64
         BSM   R0,R14
ZZ&SYSNDX.X DS 0H
         MEXIT ,
.OTHSYS3 AIF   ('&NM' EQ '').MEND
&NM      DS    0H            DEFINE LABEL ONLY
.MEND    MEND  ,
*
*
*
         MACRO ,                  FIXED 2010.293
&NM      FUNEXIT &RC=
         GBLC  &ZSYS,&ZZSETSA,&ZZSETSL,&ZZSETSP
         GBLB  &ZZSETAM
         LCLC  &LBL
&LBL     SETC  '&NM'
         AIF   ('&ZZSETSL' NE '' AND '&RC' EQ '').JUSTF
         AIF   ('&ZZSETSA' EQ '').SAMESA
         AIF   ('&ZZSETSL' NE '').SAMESA
&LBL     L     R13,4(,R13)        RESTORE HIGHER SA
&LBL     SETC  ''
.SAMESA  AIF   ('&RC' EQ '').LMALL
         AIF   ('&RC' EQ '(15)' OR '&RC' EQ '(R15)').NORC
         AIF   (K'&RC LT 3).LA
         AIF   ('&RC'(1,1) NE '(' OR '&RC'(2,1) EQ '(').LA
         AIF   ('&RC'(K'&RC,1) NE ')' OR '&RC'(K'&RC-1,1) EQ ')').LA
&LBL     LR    R15,&RC(1)
&LBL     SETC  ''
         AGO   .NORC
.LA      ANOP  ,
&LBL     LA    R15,&RC            SET RETURN CODE
&LBL     SETC  ''
.NORC    AIF   ('&ZZSETSL' EQ '').NOFRM
         LR    R1,R13             SAVE CURRENT SA
         L     R13,4(,R13)        REGAIN CALLER'S SA
         ST    R15,16(,R13)       SAVE RETURN CODE
         FREEMAIN R,A=(1),LV=&ZZSETSL,SP=&ZZSETSP
         AGO   .LMALL             GOTTA LOVE SPAGHETTI CODE
.NOFRM   ANOP  ,
&LBL     L     R14,12(,R13)
         LM    R0,R12,20(R13)
         AGO   .EXMODE
.JUSTF   ANOP  ,
&LBL     LR    R1,R13             SAVE CURRENT SA
&LBL     SETC  ''
         L     R13,4(,R13)        REGAIN CALLER'S SA
         FREEMAIN R,A=(1),LV=&ZZSETSL,SP=&ZZSETSP
.LMALL   ANOP  ,
&LBL     LM    R14,R12,12(R13)    RELOAD ALL
.EXMODE  AIF   (&ZZSETAM).BSM
         BR    R14
         MEXIT ,
.BSM     BSM   R0,R14
         MEND  ,
*
*
*
         MACRO ,             UPDATED 2010.293
&NM      FUNHEAD &ID=YES,&IO=NO,&AM=NO,&SAVE=,&US=YES
.*
.*   MACRO TO BEGIN EACH FUNCTION
.*     HANDLES STANDARD OS ENTRY CONVENTIONS
.*   ID=  YES | NO      YES GENERATES DC WITH FUNCTION NAME
.*   IO=  YES | NO      YES GENERATES LOAD / USING FOR ZDCBAREA
.*   AM=  YES | NO      YES USES BSM TO PRESERVE CALLER'S AMODE
.*   SAVE=name          USES STATIC SAVE AREA OF THAT NAME,
.*                           SETS R13, AND DECLARES ON USING
.*   SAVE=(name,len{,subpool})   CREATES SAVE AREA WITH GETMAIN,
.*                           SETS R13, AND DECLARES ON USING
.*   US=  YES | NO      YES - want a USING for R13
.*   Options used here are remembered and handled properly by
.*     subsequent FUNEXIT macros
.*
         GBLC  &ZSYS,&ZZSETSA,&ZZSETSL,&ZZSETSP
         GBLB  &ZZSETAM
         LCLC  &LBL
         LCLA  &I
&I       SETA  K'&NM
&I       SETA  ((&I)/2*2+1)       NEED ODD LENGTH FOR STM ALIGN
&LBL     SETC  '&NM'
&ZZSETAM SETB  ('&AM' NE 'NO')
&ZZSETAM SETB  (&ZZSETAM AND ('&ZSYS' EQ 'S380' OR '&ZSYS' EQ 'ZARCH'))
&ZZSETSA SETC  ''
&ZZSETSL SETC  ''
&ZZSETSP SETC  ''
         ENTRY &NM
         DROP  ,                  Isolate from other code
         AIF   ('&ID' EQ 'NO').SKIPID
&LBL     B     *+4+1+&I-&NM.(,R15)    SKIP LABEL
         DC    AL1(&I),CL(&I)'&NM'    EXPAND LABEL
&LBL     SETC  ''
.SKIPID  AIF   (NOT &ZZSETAM).SKIPAM
&LBL     BSM   R14,R0                 PRESERVE AMODE
&LBL     SETC  ''
.SKIPAM  ANOP  ,
&LBL     STM   R14,R12,12(R13)    SAVE CALLER'S REGISTERS
         LR    R12,R15
         USING &NM,R12
         AIF   ('&IO' EQ 'NO').SAVE
         L     R10,0(,R1)         LOAD FILE WORK AREA
         USING IHADCB,R10
.SAVE    AIF   ('&SAVE' EQ '').MEND
         AIF   (N'&SAVE EQ 1).STATIC
         AIF   (N'&SAVE EQ 2).DYNAM
&ZZSETSP SETC  '&SAVE(3)'
.DYNAM   ANOP  ,
&ZZSETSL SETC  '&SAVE(2)'
&ZZSETSA SETC  '&SAVE(1)'
         AIF ('&ZSYS' EQ 'S370').NOBEL2
         GETMAIN RU,LV=&ZZSETSL,SP=&ZZSETSP,LOC=BELOW
         AGO .GETFIN2
.NOBEL2  ANOP  ,
         GETMAIN RU,LV=&ZZSETSL,SP=&ZZSETSP
.GETFIN2 ANOP  ,
         LR    R14,R1             START OF NEW AREA
         LA    R15,&ZZSETSL       LENGTH
         SR    R3,R3              ZERO FILL
         MVCL  R14,R2             CLEAR GOTTEN STORAGE
         ST    R1,8(,R13)         POINT DOWN
         ST    R13,4(,R1)         POINT UP
         LR    R2,R13             SAVE OLD SAVE
         LR    R13,R1             NEW SAVE AREA
         USING &SAVE(1),R13       DECLARE IT
         LM    R14,R3,12(R2)      RESTORE FROM ENTRY
         MEXIT ,
.STATIC  LA    R15,&SAVE(1)
         ST    R15,8(,R13)
         ST    R13,4(,R15)
         LR    R13,R15
&ZZSETSA SETC  '&SAVE(1)'
         AIF   ('&US' EQ 'NO').MEND
         USING &SAVE(1),R13       DECLARE IT
.MEND    MEND  ,
*
*
*
         MACRO ,
&NM      GAM24 &WORK=R15
         GBLC  &ZSYS
.*
.*   GAM24 sets addressing mode to 24 for S380
.*         expands nothing or label for S370 AND S390
.*
         AIF   ('&ZSYS' NE 'S380' AND '&ZSYS' NE 'ZARCH').OLDSYS
&NM      LA    &WORK,*+6     GET PAST BSM WITH BIT 0 OFF
         BSM   R0,&WORK      CONTINUE IN 24-BIT MODE
         MEXIT ,
.OLDSYS  AIF   ('&NM' EQ '').MEND
&NM      DS    0H            DEFINE LABEL ONLY
.MEND    MEND  ,
*
*
*
         MACRO ,
&NM      GAM31 &WORK=R15
         GBLC  &ZSYS
.*
.*   GAM31 sets addressing mode to 31 for S380.
.*         expands nothing or label for S370  AND S390
.*
         AIF   ('&ZSYS' NE 'S380' AND '&ZSYS' NE 'ZARCH').OLDSYS
&NM      LA    &WORK,*+10    GET PAST BSM WITH BIT 0 ON
         O     &WORK,=X'80000000'  SET MODE BIT
         BSM   R0,&WORK            CONTINUE IN 31-BIT MODE
         MEXIT ,
.OLDSYS  AIF   ('&NM' EQ '').MEND
&NM      DS    0H            DEFINE LABEL ONLY
.MEND    MEND  ,
*
*
*
         MACRO ,             COMPILER DEPENDENT LOAD INTEGER
&NM      LDVAL &R,&A         LOAD VALUE FROM PARM LIST
&NM      L     &R,&A         LOAD PARM VALUE
         L     &R,0(,&R)     LOAD VALUE
.MEND    MEND  ,
*
*
*
         MACRO ,             COMPILER DEPENDENT LOAD PARM ADDRESS
&NM      LDADD &R,&A         GET ADDRESS FROM PARM LIST
&NM      L     &R,&A         LOAD PARM ADDRESS
.MEND    MEND  ,
*
*
*
         MACRO ,             COMPILER DEPENDENT LOAD INT ONLY
&NM      LDINT &R,&A         LOAD INTEGER FROM PARM LIST
         GBLC  &COMP         COMPILER GCC OR IBM C
&NM      L     &R,&A         LOAD PARM VALUE
         AIF   ('&COMP' EQ 'GCC').MEND
.* THIS LINE IS FOR ANYTHING NOT GCC: IBM C
         L     &R,0(,&R)     LOAD VALUE
.MEND    MEND  ,
*
*
*
         MACRO ,
&NM      QBSM  &F1,&F2
         GBLC  &ZSYS
.*
.*   QBSM expands as BSM on environments that require such
.*   mode switch (S380-only)
.*   Otherwise it expands as BALR r1,r2 (instead of BSM r1,r2)
.*   Unless r1 = 0, in which case, a simple BR r2 is done instead
.*
         AIF   ('&ZSYS' NE 'S380' AND '&ZSYS' NE 'ZARCH').OTHSYS
&NM      BSM   &F1,&F2
         MEXIT ,
.OTHSYS  AIF   ('&F1' EQ '0' OR '&F1' EQ 'R0').BR
&NM      BALR  &F1,&F2
         MEXIT ,
.BR      ANOP  ,
&NM      BR    &F2
.MEND    MEND  ,
*
*
*
         MACRO ,
&NM      MAPSUPRM &PFX=ZP,&DSECT=
.*  THIS MACRO DESCRIBES/DEFINES THE OPEN I/O MODE AND ASSOCIATED WORK
.*  AREA USED BY THE MVSSUPA SERVICE ROUTINE.
.*  MODE IS INPUT AND RETURNED, POSSIBLY MODIFIED.
.*  ID IS INPUT AND UPDATED.
.*  REST ARE OUTPUT ONLY, AND MAY DIFFER FROM THE @@AOPEN REQUEST.
         LCLC  &P,&N
&P       SETC  '&PFX'
&N       SETC  '&NM'
         AIF   ('&N' NE '').HAVSECT
&N       SETC  'MAP'.'&P'
.HAVSECT AIF   ('&DSECT' EQ 'NO').NOSEC
&N       DSECT ,
         AGO   .COMSEC
.NOSEC   AIF   ('&NM' EQ '').COMSEC
&NM      DS    0F
.COMSEC  ANOP  ,
&P.MODE  DC    F'0' I/O MODE (0-IN,1-OUT,2-UPD,3-APP,4-INOUT,5-OUTIN)
.*                  +8-USE EXCP FOR TAPE
.*                  +10-USE BLOCK MODE (BSAM RATHER THAN QSAM MODE)
.*                  +80-TERMINAL GETLINE  +81-TERMINAL PUTLINE
.*                  RETURNS 40-VSAM; 20-BPAM UNLIKE CONCAT
&P.MIN   EQU   0    I/O MODE quick definitions
&P.MOUT  EQU   1
&P.MUPD  EQU   2
&P.MAPP  EQU   3
&P.MINO  EQU   4
&P.MOIN  EQU   5
&P.MBLK  EQU   16
&P.MTRM  EQU   128
.*
&P.DVTYP EQU   &P.MODE,1     DEVICE TYPE OF FIRST/ONLY DD
.*
&P.RECFM EQU   &P.MODE+1,1   EQUIVALENT RECFM BITS
&P.RFU   EQU   X'C0'           UNDEFINED
&P.RFF   EQU   X'80'           FIXED
&P.RFV   EQU   X'40'           VARIABLE (WITH BDW/RDW)
&P.RFD   EQU   X'20'           ASCII VARIABLE (WITH BIT 0-1 OFF)
&P.RFT   EQU   X'20'           TRACK OVERFLOW (WITH 0-1 NOT OFF)
&P.RFB   EQU   X'10'           BLOCKED
&P.RFS   EQU   X'08'           STANDARD(F), SPANNED(V)
&P.RFA   EQU   X'04'           ANSI CONTROL CHARACTERS
&P.RFM   EQU   X'02'           MACHINE CONTROL CHARACTERS (1403 CCW)
.*
&P.SFLGS EQU   &P.MODE+2,1   ADDITIONAL MODE RELATED FLAGS
&P.FUPDT EQU   X'80'           UPDATE MODE (BSAM/VSAM)
&P.FUPIN EQU   X'40'           LAST WAS INPUT (GET/READ)
&P.FUPOU EQU   X'20'           LAST WAS OUTPUT (PUT/WRITE)
&P.FVSAM EQU   X'08'           USE VSAM
&P.FVSRR EQU   X'04'           RRDS
&P.FVSES EQU   X'02'           ESDS
&P.FVSKS EQU   X'01'           KSDS
.*
&P.MFLGS EQU   &P.MODE+3,1   REMEMBER OPEN MODE
&P.FTERM EQU   X'80'           USING GETLINE/PUTLINE
&P.FBPAM EQU   X'20'           UNLIKE BPAM CONCAT - SPECIAL HANDL
&P.FBLOK EQU   X'10'           USING BSAM READ/WRITE MODE
&P.FEXCP EQU   X'08'           USE EXCP FOR TAPE
.*.FUPD  EQU   X'06'           UPDATE IN PLACE (XSAM, VSAM)
.*.      EQU                   (RESERVED)
&P.FOUT  EQU   X'01'           OUTPUT MODE
.*
.*
&P.LEN   DC    AL2(&P.SIZE)  CONTROL BLOCK LENGTH
&P.ID    DC    H'42'         BLOCK IDENTIFIER (0 ON RETURN)
.*
&P.FLAGS DC    X'0'          DD SCAN FLAG
&P.FDD   EQU   X'80'           FOUND A DD - LATER CONCAT FLAG
&P.FSEQ  EQU   X'40'           USE IS SEQUENTIAL
&P.FPDQ  EQU   X'20'           DS IS PDS WITH MEMBER NAME
&P.FPDS  EQU   X'10'           DS IS PDS (OR PDS/E WITH S390)
&P.FVSM  EQU   X'08'           DS IS VSAM (LIMITED SUPPORT)
&P.FVTOC EQU   X'04'           DS IS VTOC (LIMITED SUPPORT)
&P.FBLK  EQU   X'02'           DD HAS FORCED BLKSIZE
.*
&P.PIX   DC    X'0'          PROCESSING INDEX
&P.IXSAM EQU   0               BSAM/BPAM - DEFAULT
&P.IXQSM EQU   4               QSAM - SAME AS BSAM, WITH DEBLOCKING
&P.IXVSM EQU   8               VSAM DATA SET
&P.IXVTC EQU   12              VTOC READER
&P.IXTRM EQU   16              TSO TERMINAL
.*.IXVSK EQU   20              (RESERVED) VSAM KEYED I/O
.*.IXVSU EQU   24              (RESERVED) VSAM UPDATE (GET/PUT/RELEASE)
.*
&P.BLKPT DC    X'00'         BLOCKS PER TRACK (MAX BLKSI)
.*
&P.OPC   DC    X'0'          DCB OPTCD
.*
&P.DEVT  DC    XL2'0'        UCBTBYT3/4
&P.ORG   DC    XL2'0'        DSORG
&P.KEYL  DC    F'0'          KEY LENGTH
&P.KEYP  DC    F'0'          KEY POSITION
&P.LRECL DC    F'0'          RECORD LENGTH
&P.BLKSZ DC    F'0'          BLOCK SIZE
&P.MAXRC DC    F'0'          MAXIMUM RECORD NUMBER
.*
.*
&P.FMOD  DC    X'00'         CALLER'S MODE: 0-F  1-V  2-U
&P.FMFIX EQU   0               FIXED RECFM (BLOCKED)
&P.FMVAR EQU   1               VARIABLE (BLOCKED)
&P.FMUND EQU   2               UNDEFINED
.*
&P.TTR   DC    XL3'00'       (MISC. USE)  TTR
.*
&P.SIZE  EQU   *-&P.MODE     SIZE TO CLEAR
         MEND  ,
*
*
*
         MACRO ,             COMPILER DEPENDENT LOAD INTEGER
&NM      STVAL &R,&A,&S=R14  STORE VALUE FROM PARM LIST
&NM      L     &S,&A         LOAD PARM VALUE
         ST    &R,0(,&S)     RETURN VALUE
.MEND    MEND  ,
*
*
*
         MACRO ,             PATTERN FOR @@DYNAL'S DYNAMIC WORK AREA
&NM      DYNPAT &P=MISSING-PFX
.*   NOTE THAT EXTRA FIELDS ARE DEFINED FOR FUTURE EXPANSION
.*
&NM      DS    0D            ALLOCATION FIELDS
&P.ARBP  DC    0F'0',A(X'80000000'+&P.ARB) RB POINTER
&P.ARB   DC    0F'0',AL1(20,S99VRBAL,0,0)
         DC    A(0,&P.ATXTP,0,0)       SVC 99 REQUEST BLOCK
&P.ATXTP DC    10A(0)
&P.AXVOL DC    Y(DALVLSER,1,6)
&P.AVOL  DC    CL6' '
&P.AXDSN DC    Y(DALDSNAM,1,44)
&P.ADSN  DC    CL44' '
&P.AXMEM DC    Y(DALMEMBR,1,8)
&P.AMEM  DC    CL8' '
&P.AXDSP DC    Y(DALSTATS,1,1)
&P.ADSP  DC    X'08'         DISP=SHR
&P.AXFRE DC    Y(DALCLOSE,0)   FREE=CLOSE
&P.AXDDN DC    Y(DALDDNAM,1,8)    DALDDNAM OR DALRTDDN
&P.ADDN  DC    CL8' '        SUPPLIED OR RETURNED DDNAME
&P.ALEN  EQU   *-&P.ARBP       LENGTH OF REQUEST BLOCK
         SPACE 1
&P.URBP  DC    0F'0',A(X'80000000'+&P.URB) RB POINTER
&P.URB   DC    0F'0',AL1(20,S99VRBUN,0,0)
         DC    A(0,&P.UTXTP,0,0)       SVC 99 REQUEST BLOCK
&P.UTXTP DC    A(X'80000000'+&P.UXDDN)
&P.UXDDN DC    Y(DUNDDNAM,1,8)
&P.UDDN  DC    CL8' '        RETURNED DDNAME
&P.ULEN  EQU   *-&P.URBP       LENGTH OF REQUEST BLOCK
&P.DYNLN EQU   *-&P.ARBP     LENGTH OF ALL DATA
         MEND  ,
*
*
*
         MACRO ,
&NM      FIXWRITE ,
&NM      L     R15,=A(TRUNCOUT)
         BALR  R14,R15       TRUNCATE CURRENT WRITE BLOCK
         MEND  ,
*
*
*
         MACRO ,
&NM      OSUBHEAD ,
         PUSH  USING
         DROP  ,
         USING WORKAREA,R13
         USING ZDCBAREA,R10
&NM      STM   R10,R15,SAVOSUB    Save registers
         BALR  R12,0
         USING *,R12
         MEND  ,
         SPACE 1
*
*
*
         MACRO ,
&NM      OSUBRET &ROUTE=
         LCLC  &T
&T       SETC  '&NM'
         AIF   (T'&ROUTE EQ 'O').BACK
         AIF   ('&ROUTE' EQ '(14)' OR '&ROUTE' EQ '(R14)').ROUT14
&T       LA    R14,=A(&ROUTE)     Return point in AOPEN
         AGO   .BACK
&T       SETC  ''                 Set label used
.ROUT14  ANOP  ,
&T       ST    R14,SAVOSUB+4*4    Update R14
&T       SETC  ''                 Set label used
.BACK    ANOP  ,
&T       LM    R10,R15,SAVOSUB    Load registers
         BR    R14                Return to caller
         MEND  ,
         SPACE 1
*
*
*
         MACRO ,
&NM      OPENCALL &WHOM
&NM      L     R15,=A(&WHOM)      Extension routine
         BALR  R14,R15            Invoke it
         MEXIT ,
         MEND  ,
         SPACE 1
         MACRO ,
&NM      OBRAN &WHERE,&OP=B,&EXIT=VECTOR
&NM      L     R14,=A(&WHERE)     Return point
         &OP   &EXIT              Branch to alternate return
         MEND  ,
         SPACE 1
*
*
*
         SPACE 1
         COPY  PDPTOP
         SPACE 1
* For non-S/370 we need to deliberately request LOC=BELOW storage
* in most places. We can't use GETMAIN R because that is not
* AM32/AM64 clean. For the main storage we deliberately request
* LOC=ANY storage. Fortunately those flags are ignored for S/370.
*
