* SPDX-License-Identifier: MIT
* Synchronous one-CPU subchannel submission for the K-only CKD buffer.
* R1 points to three C31 words: subchannel, K ORB pointer, K IRB pointer.
* ORB/CCW data addresses are separately encoded as low real addresses.
         CSECT
         ENTRY TSCIO
TSCIO    DS    0H
         STM   2,8,28(13)
         L     2,0(,1)
         L     3,4(,1)
         L     4,8(,1)
         LR    1,2
         TSCH  0(4)             Clear any prior pending status
         SSCH  0(3)
         BRCL  7,TSCFAIL
         LA    6,64
         BASR  8,0
TSCOUTER LA    5,4095
         BASR  7,0
TSCWAIT  TSCH  0(4)
         BRCL  8,TSCDONE
         BCTR  5,7
         BCTR  6,8
TSCFAIL  SR    15,15
         BCTR  15,0
         BRCL  15,TSCRET
TSCDONE  SR    15,15
TSCRET   LM    2,8,28(13)
         BR    14
         END   TSCIO
