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
TSCIO    DS    0H
         STM   2,8,28(13)
         L     2,0(,1)
         L     3,4(,1)
         L     4,8(,1)
         LR    1,2
         TSCH  0(4)             Clear any prior pending status
         SSCH  0(3)
         BRCL  7,TSCREJ
         LA    5,128(4)         Two aligned TOD slots after the IRB
         STCK  0(5)
         BRCL  7,TSCFAIL
         L     6,0(5)           Upper TOD word; wrap-safe subtraction
TSCWAIT  TSCH  0(4)
         BRCL  8,TSCDONE
         BRCL  4,TSCNOST
* TSCH CC3 is not operational; CC2 is architecturally undefined here.
* Neither can become success by waiting for a clock deadline.
         BRCL  15,TSCFAIL
TSCNOST  DS    0H
         STCK  8(5)
         BRCL  7,TSCFAIL
         L     7,8(5)
         SR    7,6
         LA    8,64
         CR    7,8              About 67 seconds: failure watchdog only
         BRCL  4,TSCWAIT
TSCFAIL  SR    15,15
         BCTR  15,0
         BCTR  15,0
         BCTR  15,0
         BRCL  15,TSCRET
TSCREJ   SR    15,15
         BCTR  15,0
         BCTR  15,0
         BRCL  15,TSCRET
TSCDONE  SR    15,15
TSCRET   LM    2,8,28(13)
         BR    14
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
         END   TSCIO
