         CSECT ,
         PRINT GEN,ON
         SPACE 1
*-----------------------ASSEMBLY OPTIONS------------------------------*
SUBPOOL  EQU   0                                                      *
*---------------------------------------------------------------------*
         SPACE 1
*                                                                     *
*   Note: Variable @@BUGF controls various debugging options, and is  *
*   externally accessible. Low order 1 bit requests SNAPs of failed   *
*   OPEN information and bad records.                                 *
         ENTRY @@BUGF
*                                                                     *
*---------------------------------------------------------------------*
*                                                                     *
* Start of functions                                                  *
*                                                                     *
*                                                                     *
***********************************************************************
*                                                                     *
*  AOPEN - Open a data set                                            *
*                                                                     *
***********************************************************************
*                                                                     *
*  Parameters are:                                                    *
*1 DDNAME - space-padded, 8 character DDNAME to be opened             *
*                                                                     *
*2 MODE =  0 INPUT  1 OUTPUT  2 UPDAT   3 APPEND      Record mode     *
*  MODE =  4 INOUT  5 OUTIN     (6-7 reserved)                        *
*  MODE = 8/9 Use EXCP for tape, BSAM otherwise (or 32<=JFCPNCP<=65)  *
*  MODE + 10 = Use BLOCK mode (valid hex 10-15)                       *
*  MODE = 80 = GETLINE, 81 = PUTLINE (other bits ignored)             *
*    N.B.: see comments under Return value                            *
*                                                                     *
*3 RECFM - 0 = F, 1 = V, 2 = U. Default/preference set by caller;     *
*                               actual value returned from open.      *
*                                                                     *
*4 LRECL   - Default/preference set by caller; OPEN value returned.   *
*                                                                     *
*5 BLKSIZE - Default/preference set by caller; OPEN value returned.   *
*                                                                     *
* August 2009 revision - caller will pass preferred RECFM (coded 0-2) *
*    LRECL, and BLKSIZE values. DCB OPEN exit OCDCBEX will use these  *
*    defaults when not specified on JCL or via DSCB merge.            *
*                                                                     *
*6 ZBUFF2 - pointer to an area that may be written to (size is LRECL) *
*                                                                     *
*7 MEMBER - *pointer* to space-padded, 8 character member name.       *
*    A member name beginning with blank or hex zero is ignored.       *
*    If pointer is 0 (NULL), no member is requested                   *
*    For a DD card specifying a PDS with a member name, this parameter*
*    will replace the JCL member, unless the DD is concatenated, then *
*    all DDs are treated as sequential and a member operand will be   *
*    an error.                                                        *
*                                                                     *
*                                                                     *
*  Return value: In R15                                               *
*  An internal "handle" that allows the assembler routines to         *
*  keep track of what's what, when READ etc are subsequently          *
*  called.                                                            *
*                                                                     *
*                                                                     *
*  Return value: PARM2 MODE:                                          *
*    Byte 0 - major device type (defined by UCBTBYT3; JES sets 01)    *
*    Byte 1 - true RECFM as used in the DCB                           *
*    Byte 2 - processing mode (see IOSFLAGS)                          *
*    Byte 3 - modified user's MODE                                    *
*                                                                     *
*  All passed parameters are subject to overrides based on device     *
*  capabilities and capacities, e.g., blocking may be turned off.     *
*                                                                     *
*                                                                     *
*  Note - more documentation for this and other I/O functions can     *
*  be found halfway through the stdio.c file in (EDWARDS.)PDPCLIB.    *
*                                                                     *
* Here are some of the errors reported:                               *
*                                                                     *
*  Input  OPEN (SVC) failed; return code is: -37                      *
*  Output OPEN (SVC) failed; return code is: -39                      *
*    Also used for VSAM failure in OPEN, SHOWCB, or TESTCB            *
*                                                                     *
* FIND input member return codes are:                                 *
* Original, before the return and reason codes had                    *
* negative translations added refer to copyrighted:                   *
* DFSMS Macro Instructions for Data Sets.                             *
* RC = 0 Member was found.                                            *
*                                                                     *
*     The 1nnn group has not been implemented.                        *
* RC = -1024 Member not found. (replaced by 2068)                     *
* RC = -1028 RACF allows PDSE EXECUTE, not PDSE READ.                 *
* RC = -1032 PDSE share not available.                                *
* RC = -1036 PDSE is OPENed output to a different member.             *
*---------------------------------------------------------------------*
*   New OPEN validity checking added; return codes are:               *
* RC = -2004 DDname starts with blank or null                         *
* RC = -2008 DDname not found                                         *
* RC = -2012 Error in system control block (JFCB, JSCB, PSCB)         *
* RC = -2016 Error reading DSCB1                                      *
* RC = -2020 Invalid TIOT entry                                       *
* RC = -2024 Invalid or unsupported DSORG                             *
* RC = -2028 Invalid DCB parameters                                   *
* RC = -2032 Invalid unit type (Graphics, Communications...)          *
* RC = -2036 Invalid concatenation (not input; mixed sequential & PDS)*
* RC = -2040 Invalid MODE request for DD                              *
* RC = -2044 PDS has no directory blocks                              *
* RC = -2048 Directory I/O error.                                     *
* RC = -2052 Out of virtual storage.                                  *
* RC = -2056 Invalid DEB or DEB not on TCB or TCBs DEB chain.         *
* RC = -2060 PDSE I/O error flushing system buffers.                  *
* RC = -2064 Invalid FIND or BLDL.                                    *
* RC = -2068 Member not found                                         *
* RC = -2072 Member not allowed                                       *
* RC = -2096 Unable to extend data (>64KiB tracks)                    *
* RC = -3nnn VSAM OPEN failed with ACBERF=nn                          *
*                                                                     *
***********************************************************************
@@AOPEN  FUNHEAD SAVE=(WORKAREA,OPENLEN,SUBPOOL)
         SR    R10,R10            Indicate no ZDCB area gotten
         LA    R11,2048(R12)
         LA    R11,2048(R11)
         USING @@AOPEN+4096,R11
         MVC   PARM1(4*7),0(R1)   Move parameters to work area
         LDADD R3,PARM1           R3 POINTS TO DDNAME
         MVC   DWDDNAM,0(R3)      Move below the line
         PUSH  USING
***********************************************************************
**                                                                   **
**  Code added to support unlike concatenation for both sequential   **
**  and partitioned access (one member from FB or VB library DDs).   **
**  Determines maximum buffer size need for both cases (except tape) **
**                                                                   **
**  Added validity checking and error codes.                         **
**                                                                   **
**  Does not use R3, R11-R13                                         **
**                                                                   **
***********************************************************************
         MVI   OPERF,ORFBADNM     PRESET FOR BAD DD NAME
         CLI   DWDDNAM,C' '       VALID NAME ?
         BNH   OPSERR               NO; REJECT IT
         MVI   OPERF,ORFNODD      PRESET FOR MISSING DD NAME
         LA    R8,DWDDNAM         COPY DDNAME POINTER TO SCRATCH REG.
* If running on MVS/XA or above, this code must be executed in AM31.
* This means that the module must be marked AM31. AM24 is only
* supported on MVS 3.8j. Hopefully one day via some mechanism
* such as dynamic allocation, this code can be executed in AM24
* on MVS/XA+, but until then, this restriction is in place.
* We do not force AM31, as that is contrary to the AMODE
* switching doctrine used by PDPCLIB.
*         GAM31 ,                 AM31 FOR S380
         LA    R4,DDWATTR         POINTER TO DD ATTRIBUTES
         USING DDATTRIB,R4        DECLARE TABLE
         L     R14,PSATOLD-PSA    GET MY TCB
         L     R9,TCBTIO-TCB(,R14) GET TIOT
         USING TIOT1,R9           DECLARE IT
         LA    R0,TIOENTRY-TIOT1  INCREMENT TO FIRST ENTRY
*---------------------------------------------------------------------*
*   LOOK FOR FIRST (OR ONLY) DD                                       *
*---------------------------------------------------------------------*
DDCFDD1  AR    R9,R0              NEXT ENTRY
         MVI   OPERF,ORFNODD      PRESET FOR NO TIOT ENTRY
         USING TIOENTRY,R9        DECLARE IT
         ICM   R0,1,TIOELNGH      GET ENTRY LENGTH
         BZ    DDCTDONE             TOO BAD
         TM    TIOESTTA,TIOSLTYP  SCRATCHED ENTRY?
         BNZ   DDCFDD1              YES; IGNORE
         CLC   TIOEDDNM,0(R8)     MATCHES USER REQUEST?
         BNE   DDCFDD1              NO; TRY AGAIN
         SR    R7,R7
         ICM   R7,7,TIOEFSRT      LOAD UCB ADDRESS (COULD BE ZERO)
         USING UCBOB,R7
         MVI   OPERF,ORFBATIO     SET FOR INVALID TIOT
         CLI   TIOELNGH,20        SINGLE UCB ?
         BL    OPSERR               NOT EVEN
*---------------------------------------------------------------------*
* EXAMINE ONE DD ENTRY, AND SET FLAGS AND BUFFER SIZE HIGH-WATER MARK *
*---------------------------------------------------------------------*
         SPACE 1
DDCHECK  MVI   OPERF,ORFNOJFC     PRESET FOR BAD JFCB
         ICM   R1,7,TIOEJFCB      GET JFCB ADDRESS OR TOKEN
         BZ    OPSERR               NO JFCB ?
