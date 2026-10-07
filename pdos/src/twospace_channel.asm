* SPDX-License-Identifier: MIT
* Completion-driven subchannel submission for K-owned CKD and 3270 I/O.
* R1 points to three C31 words: subchannel, K ORB pointer, K IRB pointer.
* ORB/CCW data addresses are separately encoded as low real addresses.
* STCK only bounds a failed operation; the TSCH status event decides success.
         CSECT
         ENTRY TSCIO
         ENTRY TSCDEV
         ENTRY TSCENABL
         ENTRY TSCSTART
         ENTRY TSCPOLL
         ENTRY TSCCLEAR
         ENTRY TSCWAITR
         ENTRY TSCWAITI
TSCIO    DS    0H
         STM   2,10,28(13)
* No armed wait state until SSCH succeeds
         SR    8,8
         L     2,0(,1)
         L     3,4(,1)
         L     4,8(,1)
         LR    1,2
         TSCH  0(4)
         SSCH  0(3)
         BRCL  7,TSCREJ
         SR    9,9
TSCARM   LA    5,128(4)
         STCK  0(5)
         BRCL  7,TSCFAIL
* Save clock-comparator and CR0 policy; only this synchronous K operation
* arms the watchdog. All scratch is before the separate CCW region.
* STCKC 224(R4)
         DC    X'B20740E0'
* STCTG C0,C0,184(R4)
         DC    X'EB0040B80025'
         MVC   208(8,4),184(4)
         LA    7,2
         CR    9,7
         BRCL  8,TSCNOPRM
* CR0 bit52 enables the clock-comparator interruption.
         OI    214(4),X'08'
* LCTLG C0,C0,208(R4)
         DC    X'EB0040D0002F'
         MVC   176(8,4),128(4)
         L     6,176(4)
         LA    7,64
         AR    6,7
         ST    6,176(4)
* SCKC 176(R4)
         DC    X'B20640B0'
TSCNOPRM DS    0H
* EPSW R6,R7
         DC    X'B98D0067'
         STM   6,7,192(4)
         MVC   160(8,4),192(4)
* Enable I/O and external interruption
         OI    160(4),X'03'
* Enabled WAIT; interruption entry clears it
         OI    162(4),X'02'
         XC    168(4,4),168(4)
         BASR  10,0
         USING *,10
         L     10,=A(TSCWAIT)
         DROP  10
         ST    10,172(4)
         LA    8,1
* First completion is delivered through the real I/O interruption path.
         DC    X'B2B240A0'
TSCWAIT  LA    7,2
         CR    9,7
* Prompt wait only notifies C of an interruption. C owns the status routing
* and still requires the exact channel completion before reporting success.
         BRCL  8,TSCOK
         LR    1,2
         TSCH  0(4)
         BRCL  8,TSCDONE
         BRCL  4,TSCNOST
         BRCL  15,TSCFAIL
TSCNOST  STCK  8(5)
         BRCL  7,TSCFAIL
         LA    7,2
         CR    9,7
         BRCL  8,TSCIDLE
         L     7,8(5)
         L     6,0(5)
         SR    7,6
         LA    10,64
         CR    7,10
* Deadline only reports a stalled operation
         BRCL  10,TSCFAIL
* Wake only on an actual interruption event
TSCIDLE  DS    0H
         DC    X'B2B240A0'
TSCFAIL  SR    15,15
         BCTR  15,0
         BCTR  15,0
         BCTR  15,0
         BRCL  15,TSCRET
TSCREJ   SR    15,15
         BCTR  15,0
         BCTR  15,0
         BRCL  15,TSCRET
TSCDONE  LA    7,1
         CR    9,7
         BRCL  7,TSCOK
* Clear completion is required before a cancelled workspace can be reused.
         TM    2(4),X'10'
         BRCL  8,TSCNOST
TSCOK    SR    15,15
TSCRET   LTR   8,8
         BRCL  8,TSCREST
* Restore saved clock comparator
         DC    X'B20640E0'
* LCTLG C0,C0,184(R4)
         DC    X'EB0040B8002F'
TSCREST  LM    2,10,28(13)
         BR    14
         LTORG
TSCDEV   DS    0H
         STM   2,3,28(13)
         L     2,0(,1)
         L     3,4(,1)
         LR    1,2
         STSCH 0(3)
         BRCL  7,TSDNONE
         SR    15,15
         ICM   15,B'0011',6(3)
         BRCL  15,TSDDONE
TSDNONE  SR    15,15
TSDDONE  LM    2,3,28(13)
         BR    14
* STSCH preserves device path state; byte 5 bit 7 enables this terminal.
TSCENABL DS    0H
         STM   2,3,28(13)
         L     2,0(,1)
         L     3,4(,1)
         LR    1,2
         STSCH 0(3)
         BRCL  7,TSEFAIL
         OI    5(3),X'80'
         MSCH  0(3)
         BRCL  7,TSEFAIL
         SR    15,15
         BRCL  15,TSERET
TSEFAIL  SR    15,15
         BCTR  15,0
TSERET   LM    2,3,28(13)
         BR    14
TSCSTART DS    0H
         STM   2,4,28(13)
         L     2,0(,1)
         L     3,4(,1)
         L     4,8(,1)
         LR    1,2
         TSCH  0(4)
         SSCH  0(3)
         BRCL  8,TSSOK
         SR    15,15
         BCTR  15,0
         BRCL  15,TSSRET
TSSOK    SR    15,15
TSSRET   LM    2,4,28(13)
         BR    14
TSCPOLL  DS    0H
         STM   2,3,28(13)
         L     2,0(,1)
         L     3,4(,1)
         LR    1,2
         TSCH  0(3)
         BRCL  8,TSPYES
         BRCL  4,TSPNONE
         SR    15,15
         BCTR  15,0
         BRCL  15,TSPRET
TSPNONE  DS    0H
         LA    15,1
         BRCL  15,TSPRET
TSPYES   SR    15,15
TSPRET   LM    2,3,28(13)
         BR    14
* CSCH is asynchronous. Do not recycle this real workspace until TSCH
* reports the clear-function completion bit in the returned SCSW.
* Await attention or the completion of an already submitted read.
TSCWAITR DS  0H
         STM   2,10,28(13)
         SR    8,8
         SR    9,9
         L     2,0(,1)
         L     4,4(,1)
         BRCL  15,TSCARM
* A line prompt may legitimately remain idle. This returns a wake notification;
* C routes status and judges completion. Qualification has its own watchdog.
TSCWAITI DS    0H
         STM   2,10,28(13)
         SR    8,8
         LA    9,2
         L     2,0(,1)
         L     4,4(,1)
         BRCL  15,TSCARM
TSCCLEAR DS    0H
         STM   2,10,28(13)
         SR    8,8
         L     2,0(,1)
         L     4,4(,1)
         LR    1,2
* CSCH (no storage operand). Share the interruption-woken completion wait.
         DC    X'B2300000'
         BRCL  7,TCLREJ
         LA    9,1
         BRCL  15,TSCARM
TCLREJ   SR    15,15
         BCTR  15,0
         BRCL  15,TSCRET
         END   TSCIO
