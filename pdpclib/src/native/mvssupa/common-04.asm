*
*
*
**********************************************************************
*                                                                    *
*  TEST31 - see if we are running in AMODE 31                        *
*                                                                    *
*  This function returns 1 if we are running in AMODE 31, else 0     *
*                                                                    *
*  This code works because in 31-bit mode, a BALR will set the       *
*  high bit of the first register to 1, with the remaining 31 bits   *
*  used for the address. While in 24-bit mode, the entire top byte   *
*  has information stored, with the remaining 3 bytes used for the   *
*  address. The first 2 bits of that top byte are the ILC, which     *
*  will be b'01' for a BALR, and b'10' for a BAL. We use BALR, so    *
*  we get b'01', hence the top bit is always 0. Note that BASR       *
*  could be used instead of BALR and we would still get the same     *
*  result. Note that the b'01' in BALR means "1 halfword", ie        *
*  the instruction (BALR) is 2 bytes long.                           *
*                                                                    *
**********************************************************************
         ENTRY @@TEST31
@@TEST31 DS    0H
         SAVE  (14,12),,@@TEST31
         LR    R12,R15
         USING @@TEST31,R12
*
         LA    R15,1
         BALR  R1,R0
         LTR   R1,R1
         BM    RETURNTS
         LA    R15,0
*
RETURNTS DS    0H
         RETURN (14,12),RC=(15)
         LTORG ,
*
*
*
**********************************************************************
*                                                                    *
*  GETAM - get the current AMODE                                     *
*                                                                    *
*  This function returns 24 if we are running in exactly AMODE 24,   *
*  31 if we are running in exactly AMODE 31, and 64 for anything     *
*  else (user-defined/infinity/16/32/64/37)                          *
*                                                                    *
*  Be aware that MVS 3.8j I/O routines require an AMODE of exactly   *
*  24 - nothing more, nothing less - so applications are required    *
*  to ensure they are in AM24 prior to executing any I/O routines,   *
*  and then they are free to return to whichever AMODE they were in  *
*  previously (ie anything from 17 to infinity), which is normally   *
*  done using a BSM to x'01', although this instruction was not      *
*  available in S/370-XA so much software does a BSM to x'80'        *
*  instead of the user-configurable x'01', which is unfortunate.     *
*                                                                    *
*  For traditional reasons, people refer to 24, 31 and 64, when what *
*  they should really be saying is 24, 31 and user-defined.          *
*                                                                    *
**********************************************************************
         PUSH  USING
         DROP  ,
         ENTRY @@GETAM
@@GETAM  DS    0H
         SAVE  (14,12),,@@GETAM
         LR    R12,R15
         USING @@GETAM,R12
*
         L     R2,=X'C1800000'
         LA    R2,0(,R2)
         CLM   R2,B'1100',=X'0080'
         BE    GAIS24
         CLM   R2,B'1000',=X'41'
         BE    GAIS31
         LA    R15,64
         B     RETURNGA
GAIS24   DS    0H
         LA    R15,24
         B     RETURNGA
GAIS31   LA    R15,31
*
RETURNGA DS    0H
         RETURN (14,12),RC=(15)
         LTORG ,
         POP   USING
         SPACE 2
***********************************************************************
*                                                                     *
*  ADDNUM - Add two numbers using 80386                               *
*                                                                     *
***********************************************************************
*
         PUSH  USING
         DROP  ,
         ENTRY @@ADDNUM
@@ADDNUM DS    0H
         SAVE  (14,12),,@@ADDNUM
         LR    R12,R15
         USING @@ADDNUM,R12
         LR    R2,R1  new register for parms
         L     R0,=X'FFFFFFFD' API for execute 80386
         LR    R1,R0
         LA    R3,CODE386
         LA    R14,ANRET
         L     R4,0(R2)
         L     R5,4(R2)
         L     R6,8(R2)
         SVC   120
ANRET    DS    0H
         RETURN (14,12),RC=(15)
*
         LTORG ,
*
CODE386  DS    0D
         DC    X'55' push ebp
         DC    X'8B' mov ebp,esp
         DC    X'EC'
         DC    X'8B' mov eax, ebp + 8
         DC    X'45'
         DC    X'08'
         DC    X'03' add eas, ebp + 12
         DC    X'45'
         DC    X'0C'
         DC    X'C9' leave
         DC    X'C3' return near
         DC    X'22' eyecatcher
         DC    X'22'
         DC    X'22'
         POP   USING
         SPACE 2
***********************************************************************
*                                                                     *
*  GETMSZ - Get memory size via DIAG                                  *
*                                                                     *
***********************************************************************
*
         PUSH  USING
         DROP  ,
         ENTRY @@GETMSZ
@@GETMSZ DS    0H
         SAVE  (14,12),,@@GETMSZ
         LR    R12,R15
         USING @@GETMSZ,R12
*         DIAGNOSE X'60'
         DC    X'83',X'000060'
         LR    R15,R0
         RETURN (14,12),RC=(15)
*
         LTORG ,
         POP   USING
         SPACE 2
*
*
*
***********************************************************************
*                                                                     *
*  GOSUP - go into supervisor mode                                    *
*                                                                     *
***********************************************************************
*
         PUSH  USING
         DROP  ,
         ENTRY @@GOSUP
@@GOSUP  DS    0H
         SAVE  (14,12),,@@GOSUP
         LR    R12,R15
         USING @@GOSUP,R12
         AIF   ('&OS' NE 'PDOS').PDGOSUP
         LA    R15,12             MODESET IS NOT A PDOS SERVICE
         AGO   .PDGOSEND
.PDGOSUP ANOP
         MODESET MODE=SUP
         LA    R15,0
.PDGOSEND ANOP
         RETURN (14,12),RC=(15)
*
         LTORG ,
         POP   USING
         SPACE 2
***********************************************************************
*                                                                     *
*  GOPROB - go into problem mode                                      *
*                                                                     *
***********************************************************************
*
         PUSH  USING
         DROP  ,
         ENTRY @@GOPROB
@@GOPROB DS    0H
         SAVE  (14,12),,@@GOPROB
         LR    R12,R15
         USING @@GOPROB,R12
         AIF   ('&OS' NE 'PDOS').PDGOPRB
         LA    R15,12             MODESET IS NOT A PDOS SERVICE
         AGO   .PDGOPEND
.PDGOPRB ANOP
         MODESET MODE=PROB
         LA    R15,0
.PDGOPEND ANOP
         RETURN (14,12),RC=(15)
*
         LTORG ,
         POP   USING
         SPACE 2
***********************************************************************
*                                                                     *
*  CALL @@SVC99,(rb)                                                  *
*                                                                     *
*  Execute DYNALLOC (SVC 99)                                          *
*                                                                     *
*  Caller must provide a request block, in conformance with the       *
*  MVS documentation for this (which is very complicated)             *
*                                                                     *
***********************************************************************
         PUSH  USING
         DROP  ,
         ENTRY @@SVC99
@@SVC99  DS    0H
         SAVE  (14,12),,@@SVC99   Save caller's regs.
         LR    R12,R15
         USING @@SVC99,R12
         LR    R11,R1
*
         AIF ('&ZSYS' EQ 'S370').NOBEL9
         GETMAIN RU,LV=WORKLEN,SP=SUBPOOL,LOC=BELOW
         AGO .GETFIN9
.NOBEL9  ANOP  ,
         GETMAIN RU,LV=WORKLEN,SP=SUBPOOL
.GETFIN9 ANOP  ,
         ST    R13,4(,R1)
         ST    R1,8(,R13)
         LR    R13,R1
         LR    R1,R11
         USING WORKAREA,R13
*
* Note that the SVC requires a pointer to the pointer to the RB.
* Because this function (not SVC) expects to receive a standard
* parameter list, where R1 so happens to be a pointer to the
* first parameter, which happens to be the address of the RB,
* then we already have in R1 exactly what SVC 99 needs.
*
* Except for one thing. Technically, you're meant to have the
* high bit of the pointer on. So we rely on the caller to have
* the parameter in writable storage so that we can ensure that
* we set that bit.
*
         L     R2,0(,R1)
         O     R2,=X'80000000'
         ST    R2,0(,R1)
         SVC   99
         LR    R2,R15
*
RETURN99 DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
         FREEMAIN RU,LV=WORKLEN,A=(1),SP=SUBPOOL
*
         LR    R15,R2             Return success
         RETURN (14,12),RC=(15)   Return to caller
*
         POP   USING
         SPACE 2
***********************************************************************
*                                                                     *
*    CALL @@SNAP,snaplist                                             *
*                                                                     *
*    snaplist is the expansion produced by SNAP options,MF=L          *
*    Examples of use are in AOPEN and AREAD.                          *
*                                                                     *
*    Dump data are written to the SYSTERM DD, with predetermined      *
*        DCB values (required by SVC 51).                             *
*                                                                     *
*    according to my macro manual, SNAP will operate correctly with   *
*        addresses above the line, and only the DCB must be in 24-bit *
*        storage. If the MVS version doesn't work that way, code must *
*        be added to copy the caller's parm list to the DCB work area *
*                                                                     *
*    No output is produced unless the debug flag is on.               *
*                                                                     *
*    CODE IS NON-REENTRANT, NON-REFRESHABLE, but REUSABLE.            *
*                                                                     *
*---------------------------------------------------------------------*
*                                                                     *
*     Author:  Gerhard Postpischil                                    *
*                                                                     *
*     This code is placed in the public domain.                       *
*                                                                     *
*---------------------------------------------------------------------*
*                                                                     *
*     Return codes:  as set by SNAP macro/SVC                         *
*                                                                     *
***********************************************************************
*  Maintenance:                                     new on 2014-08-31 *
*                                                                     *
***********************************************************************
         SPACE 1
         PUSH  USING
         PUSH  PRINT
         PRINT NOGEN         DON'T NEED TWO COPIES
         DROP  ,
@@SNAP   FUNHEAD SAVE=(SNAPAREA,SNAPALEN,SUBPOOL)
         L     R15,4(,R13)        GET CALLER'S SAVE AREA
         LA    R11,16(,R15)       REMEMBER RETURN CODE ADDRESS
         SLR   R0,R0
         ST    R0,0(,R11)         PRESET
         LA    R9,0(,R1)          SAVE PARAMETER LIST ADDRESS
         LTR   R9,R9         REQUEST TO CLOSE/FREE?
         BZ    SNAPCLOS        YES
         SPACE 1
         L     R6,=A(@@BUGF)      GET DEBUGGING FLAG
         TM    3(R6),X'01'        SNAP REQUESTED?
         BZ    SNAPRET              NO; RETURN
         ICM   R10,15,@SNAPDCB    PREVIOUSLY GOTTEN?
         BNZ   SNAPGOT
         USING SNAPDCB,R10   DECLARE DYNAMIC WORK AREA
SNAPGET  LA    R0,SNAPSLEN   GET LENGTH OF SAVE AND WORK AREA
         AIF ('&ZSYS' EQ 'S370').NOBELA
         GETMAIN RU,LV=(0),LOC=BELOW
         AGO .GETFINA
.NOBELA  ANOP  ,
         GETMAIN RU,LV=(0)
.GETFINA ANOP  ,
         STM   R0,R1,#SNAPDCB     SAVE FOR RELEASE
         LR    R10,R1
         MVC   SNAPDCB(PATSNAPL),PATSNAP   INIT DCB, ETC.
         OPEN  (SNAPDCB,OUTPUT),MF=(E,SNAPOCL)
         SPACE 1
         LTR   R9,R9         ANY ADDRESS ?
         BZ    SNAPCLOS        NO; CLOSE REQUEST
SNAPGOT  LA    R7,1          INCREMENT DUMP COUNTER
         AL    R7,SNAPCTR    INCREMENT DUMP COUNTER
         ST    R7,SNAPCTR    INCREMENT DUMP COUNTER
         SNAP  DCB=SNAPDCB,ID=(R7),MF=(E,(R9))
         ST    R15,0(,R11)   PROPAGATE RETUNR CODE
         B     SNAPRET
         SPACE 1
SNAPCLOS ICM   R10,15,@SNAPDCB    EVER GOTTEN STORAGE ?
         BZ    SNAPRET              NO; JUST RETURN
         TM    SNAPDCB+(DCBOFLGS-IHADCB),DCBOFOPN  OPEN ?
         BZ    SNAPFREE             NO; JUST FREE STORAGE
         CLOSE MF=(E,SNAPOCL)
SNAPFREE L     R0,#SNAPDCB
         FREEMAIN R,LV=(0),A=(R10)
         XC    #SNAPDCB(L'#SNAPDCB+L'@SNAPDCB),#SNAPDCB
         SPACE 1
SNAPRET  FUNEXIT ,           RESTORE REGS; SET RETURN CODES
         SPACE 1
         LTORG ,
         SPACE 1
#SNAPDCB DC    F'0'    1/2   LENGTH OF PERSISTENT DCB WORK AREA
@SNAPDCB DC    A(0)    2/2   ADDR. OF PERSISTENT DCB WORK AREA
         SPACE 1
PATSNAP  DCB   DDNAME=SYSTERM,MACRF=(W),DSORG=PS,                      *
               RECFM=VBA,LRECL=125,BLKSIZE=1632  882
PATSNOC  DC    X'8F000000'   OPEN MF=L
*OLD*OC  OPEN  (PATSNAP,OUTPUT),MF=L
PATSCTR  DC    F'0'          DUMP ID; WRAPS AT 256->0
PATSNAPL EQU   *-PATSNAP
         SPACE 1
         SPACE 1
SNAPSAVE DSECT ,
SNAPDCB  DCB   DDNAME=SYSTERM,MACRF=(W),DSORG=PS,                      *
               RECFM=VBA,LRECL=125,BLKSIZE=1632  882
*OLDOCL  OPEN  (SNAPDCB,OUTPUT),MF=L
SNAPOCL  DC    A(0)          OPEN MF=L
SNAPCTR  DC    F'0'          DUMP ID; WRAPS AT 256->0
SNAPSLEN EQU   *-SNAPSAVE
         SPACE 1
SNAPAREA DSECT ,
         DS    18A           STANDARD SAVE AREA ONLY
SNAPALEN EQU   *-SNAPAREA    LENGTH TO GET
         POP   USING
         POP   PRINT
         CSECT ,             RESTORE CSECT
         SPACE 2
*
* Keep this code last because it makes no difference - no USINGs
*
***********************************************************************
*                                                                     *
*  SETJ - SAVE REGISTERS INTO ENV                                     *
*                                                                     *
***********************************************************************
         ENTRY @@SETJ
@@SETJ   L     R15,0(,R1)         get the env variable
         STM   R0,R14,0(R15)      save registers to be restored
         LA    R15,0              setjmp needs to return 0
         BR    R14                return to caller
         SPACE 1
***********************************************************************
*                                                                     *
*  LONGJ - RESTORE REGISTERS FROM ENV                                 *
*                                                                     *
***********************************************************************
         ENTRY @@LONGJ
@@LONGJ  L     R2,0(,R1)          get the env variable
         L     R15,60(,R2)        get the return code
         LM    R0,R14,0(R2)       restore registers
         BR    R14                return to caller
         SPACE 2
*
* S/370 doesn't support switching modes so this code is useless,
* and won't compile anyway because "BSM" is not known.
*
         AIF   ('&ZSYS' EQ 'S370').NOMODE If S/370 we can't switch mode
         PUSH  USING
         DROP  ,
***********************************************************************
*                                                                     *
*  SETM24 - Set AMODE to 24                                           *
*                                                                     *
***********************************************************************
         ENTRY @@SETM24
         USING @@SETM24,R15
@@SETM24 LA    R14,0(,R14)        Sure hope caller is below the line
         BSM   0,R14              Return in amode 24
         POP   USING
*
         SPACE 1
         PUSH  USING
         DROP  ,
***********************************************************************
*                                                                     *
*  SETM31 - Set AMODE to 31                                           *
*                                                                     *
***********************************************************************
         ENTRY @@SETM31
         USING @@SETM31,R15
@@SETM31 ICM   R14,8,=X'80'       Clobber entire high byte of R14
*                                 This is necessary because if people
*                                 use BALR in 24-bit mode, the address
*                                 will have rubbish in the high byte.
*                                 People switching between 24-bit and
*                                 31-bit will be RMODE 24 anyway, so
*                                 there is nothing to preserve in the
*                                 high byte.
         BSM   0,R14              Return in amode 31
         LTORG ,
         POP   USING
*
         SPACE 1
         PUSH  USING
         DROP  ,
***********************************************************************
*                                                                     *
*  SETM64 - Set AMODE to 64                                           *
*                                                                     *
***********************************************************************
* The caller is likely to be using R12 as a base register, so we
* need to clean that too. As such, we can't use it as the base
* register here (not conveniently, anyway).
         ENTRY @@SETM64
@@SETM64 DS    0H
         SAVE  (0,11),,@@SETM64
         LR    R7,R15
         USING @@SETM64,R7
         LA    R7,0(,R7)          Clean base register
         LA    R12,0(,R12)        Clean caller's base register
         LA    R14,0(,R14)        Clean return address
         LA    R2,NXT64
         LA    R2,1(R2)           Set AM64 bit
         BSM   0,R2
NXT64    DS    0H
         LA    R15,0
         RETURN (0,11),RC=(15)
         LTORG ,
         POP   USING
*
.NOMODE  ANOP  ,                  S/370 doesn't support MODE switching
*
*
*
*
**********************************************************************
*                                                                    *
* SVCRL - do a real SVC                                              *
*                                                                    *
**********************************************************************
         PUSH  USING
         DROP  ,
         ENTRY @@SVCRL
@@SVCRL  DS    0H
         USING @@SVCRL,R14
         STM   R14,R12,12(R13)
         LR    R14,R15
         L     R12,0(,R1)
         L     R11,4(,R1)
         L     R0,0(,R11)
         L     R1,4(,R11)
         L     R2,8(,R11)
         L     R3,12(,R11)
         L     R4,16(,R11)
         L     R5,20(,R11)
         L     R6,24(,R11)
         L     R7,28(,R11)
         L     R8,32(,R11)
         L     R9,36(,R11)
         L     R10,40(,R11)
         L     R15,60(,R11)
         L     R11,44(,R11)
         EX    R12,SVC1
         B     SVC2
SVC1     DS    0H
         SVC   0
SVC2     DS    0H
         L     R14,12(R13)
         LM    R0,R12,20(R13)
         BR    R14
*
         LTORG
         DROP  R14
         POP   USING
*
*
*
*
**********************************************************************
*                                                                    *
* DOLOOP - go into a hard loop                                       *
*                                                                    *
**********************************************************************
         ENTRY @@DOLOOP
@@DOLOOP DS    0H
         LR    R12,R15
         USING @@DOLOOP,R12
*
         LA    R3,3
         LA    R4,4
         LA    R5,5
HARDLOOP B     HARDLOOP
*
*
*
**********************************************************************
*                                                                    *
*  SETUP - do initialization. I used the word "setup" instead of     *
*  "init" in case someone imagines that "init" is some sort of       *
*  complicated compiler-generated function.                          *
*                                                                    *
*  This routine figures out the amode switching strategy given that  *
*  the operating system may require a lower amode that the           *
*  application, and this will be reflected in the fact that the      *
*  rmode will be lower than the amode, to allow this switch to       *
*  occur. It is left to the user to use a utility to set the RMODE   *
*  to something that their current operating system supports. E.g.   *
*  a future version of z/OS may allow execution of READ in AM64 in   *
*  which case the z/OS user is free to change this module from RM31  *
*  to RM64, with a view to having the 32-bit load module loaded in   *
*  the 2 GiB to 4 GiB region.                                        *
*                                                                    *
*  Note that AMODE switching is not required, and thus doesn't even  *
*  need time to be wasted, if you are targeting a "pure" environment *
*  such as S370 where everything is in AM24 and the OS can handle    *
*  that, or S390 where everything is in AM31 and the OS can handle   *
*  that, and possibly in the future there will be such a thing as    *
*  Z999 where the OS can handle being called in AM64, so there is    *
*  no need to waste time checking to see if an amode switch is       *
*  required. However, it is strongly advised that instead of coding  *
*  for such pure environments, you instead select STEPD,             *
*  which will work optimally for 32-bit applications on all          *
*  environments, ie AM24 in MVS 3.8j, switch between AM31 and AM24   *
*  on MVS/XA, remain in AM31 on late MVS/ESA and above, and switch   *
*  between AM32 (aka AM64) and AM24 on MVS/380, while attempting to  *
*  obtain RM32 memory on MVS/380.                                    *
*                                                                    *
*  Note that this function should be the last in the source file,    *
*  so that when the test is done to see where the function has been  *
*  loaded, it will err on the side of caution when e.g. the load     *
*  module spans the 2 GiB bar, and only activate step-down           *
*  processing if it finds the SETUP function itself is below the     *
*  2 GiB bar which means the other functions will succeed in         *
*  switching to AM31.                                                *
*                                                                    *
*  Exception - to work on z/PDOS which is pure AM64, if you discover *
*  you are in AM64 then no step-down will be done.                   *
*                                                                    *
**********************************************************************
         ENTRY @@SETUP
@@SETUP  DS    0H
         SAVE  (14,12),,@@SETUP
         LR    R12,R15
         USING @@SETUP,R12
*
         AIF   ('&STEPD' NE 'YES').NOSETUP
*
* If we are running in a pure 24-bit environment, where
* the AMODE and RMODE are the same, there is no need to
* ever do AMODE switching, so none of this AMODE
* switching code is required at all
*
         L     R2,=X'C1800000'
         LA    R2,0(,R2)
         CLM   R2,B'1100',=X'0080'
* If we are currently in AM24, there is nothing
* to ever do, as we will stay in that mode forever
         BE    RETURNSU
*
* Now see if we are running AM31
         CLM   R2,B'1000',=X'41'
         BNE   IS32
* We are running AM31. If we are also located in
* RM31 space we do not need to do BSM switching
         LR    R2,R12
         N     R2,=X'7F000000'
         BNZ   RETURNSU No amode switching possible
* The app is AM31 but the OS is AM24
* An OS of AM24 is default, so just go and set the
* application AMODE now
         B     COMM3164
* Note that we say "32" here, but it is actually
* any value other than 24 or 31.
IS32     DS    0H
*
* At this stage we know we are running in AM64
* aka AM32 aka AM-infinity (we don't know which one)
* First we need to know if we are running in RM32,
* highly unlikely.
*
         LR    R2,R12
         N     R2,=X'80000000'
         BNZ   RETURNSU No amode switching possible
* !!! EXCEPTION !!!
* Don't step down when we are in AM64 AND in RM31
* space so that z/PDOS works (S/380 model only)
         AIF   ('&ZSYS' NE 'S380').ZSTEPD
         LR    R2,R12
         N     R2,=X'7F000000'
         BNZ   RETURNSU     Don't switch amodes
.ZSTEPD  ANOP
* !!! EXCEPTION !!!
* Now see if we are running in RM31 space
         LR    R2,R12
         N     R2,=X'7F000000'
         BZ    COMM3164 RM24 so just set the app amode bits
* We are indeed running in RM31 so we need the high bit
* set whenever we switch to OS mode
         OI    NEEDBOO,X'80'
COMM3164 DS    0H
* We have dealt with the appropriate bits to set
* the OS mode, now we need to set the return to
* application mode. That is easy, it is the current
* amode, either AM64 or AM31
         LA    R2,0
         BSM   R2,0
         ST    R2,NEEDBOA this will be suitable for ORing
