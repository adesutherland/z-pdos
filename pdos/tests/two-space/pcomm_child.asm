* SPDX-License-Identifier: MIT
* Native AMODE31 diagnostic member; no z/PDOS-specific entry or return SVC.
* Parameters are the normal flagged MVS list and halfword-length tail.
         CSECT
         ENTRY @@MAIN
@@MAIN   BASR  12,0
         USING *,12
         LR    11,14
         L     1,0(,1)
         N     1,PDTESTMASK
         CLI   2(1),C'F'
         BE    PDTESTFAULT
         CLI   2(1),C'B'
         BE    PDTESTBUFFER
         CLI   2(1),C'A'
         BE    PDTESTALLOC
         LA    15,37
         BR    11
PDTESTBUFFER LA 0,4
         L     1,PDTESTUFAIL
         SVC   93
         BR    11
PDTESTALLOC L  0,PDTESTHUGE
         SR    1,1
         LA    15,48
         SVC   120
         BR    11
PDTESTFAULT L 0,PDTESTPAGE
         SR    1,1
         LA    15,48
         SVC   120
         LTR   15,15
         BNZ   PDTESTRET
         MVI   0(1),X'5A'
         L     1,PDTESTBAD
         L     2,0(1)
PDTESTRET BR 11
         DS    0F
PDTESTMASK DC X'7FFFFFFF'
PDTESTHUGE DC X'10000000'
PDTESTPAGE DC F'4096'
PDTESTBAD DC F'4096'
PDTESTUFAIL DC X'7FFFF000'
         END   @@MAIN
