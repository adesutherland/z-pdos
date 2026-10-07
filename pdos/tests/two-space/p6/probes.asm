* SPDX-License-Identifier: MIT; bounded normal-image storage/pointer controls.
         CSECT
         ENTRY @@MAIN
@@MAIN   BASR  12,0
         USING *,12
         L     0,LOWBIG
         SR    1,1
         LA    15,16
         SVC   120
         C     15,RCFOUR
         BNE   BAD
         LTR   1,1
         BNZ   BAD
         L     0,REALBIG
         SR    1,1
         LA    15,48
         SVC   120
         C     15,RCFOUR
         BNE   BAD
         LTR   1,1
         BNZ   BAD
         LA    0,64
         L     1,KALIAS
         SR    2,2
         SVC   200
         C     15,RCEIGHT
         BNE   BAD
         LA    0,32
         LA    1,CAPS
         SR    2,2
         SVC   200
         C     15,RCEIGHT
         BNE   BAD
         LA    15,37
         SVC   3
BAD      LA    15,39
         SVC   3
         DS    0F
LOWBIG   DC    X'01000000'
REALBIG  DC    X'20000000'
KALIAS   DC    X'05000000'
RCFOUR   DC    F'4'
RCEIGHT  DC    F'8'
CAPS     DC    F'1',F'64'
         DS    56C
         END   @@MAIN
