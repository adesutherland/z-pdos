* SPDX-License-Identifier: MIT
* Bounded synchronous subchannel submission for K-owned CKD and 3270 I/O.
* R1 points to three C31 words: subchannel, K ORB pointer, K IRB pointer.
* ORB/CCW data addresses are separately encoded as low real addresses.
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
         LA    6,4095
         BASR  8,0
TSCOUTER LA    5,4095
         BASR  7,0
TSCWAIT  TSCH  0(4)
         BRCL  8,TSCDONE
         BCTR  5,7
         BCTR  6,8
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
         SR    15,15
         BRCL  7,TSDDONE
         ICM   15,B'0011',6(3)
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
         SR    15,15
         BRCL  8,TSSRET
         BCTR  15,0
TSSRET   LM    2,4,28(13)
         BR    14
TSCPOLL  DS    0H
         STM   2,3,28(13)
         L     2,0(,1)
         L     3,4(,1)
         LR    1,2
         TSCH  0(3)
         SR    15,15
         BRCL  8,TSPRET
         LA    15,1
TSPRET   LM    2,3,28(13)
         BR    14
         END   TSCIO
