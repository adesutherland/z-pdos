* SPDX-License-Identifier: MIT
* Native U64 neutral capability request above 4 GiB.
         CSECT
         ENTRY @@MAIN
@@MAIN   BASR  12,0
         USING *,12
         LGHI  0,4096
         LGHI  1,0
         LGHI  2,0
         SVC   223
         LTR   15,15
         BNZ   P4BAD
         LGR   6,1
         SRLG  3,6,32
         LTR   3,3
         BZ    P4BAD
         LA    0,1
         ST    0,0(1)
         LA    0,64
         ST    0,4(1)
         SR    2,2
         SVC   200
         LTR   15,15
         BNZ   P4BAD
         LGR   1,6
         L     0,0(1)
         C     0,P4VERSION
         BNE   P4BAD
         L     0,4(1)
         C     0,P4LENGTH
         BNE   P4BAD
         L     0,8(6)
         LTR   0,0
         BZ    P4BAD
         L     0,60(6)
         LTR   0,0
         BZ    P4BAD
         LGR   1,6
         LGHI  0,0
         LGHI  2,0
         SVC   223
         LTR   15,15
         BNZ   P4BAD
         SR    15,15
         SVC   3
P4BAD    LA    15,39
         SVC   3
         DS    0D
P4VERSION DC   F'1'
P4LENGTH DC    F'64'
         END   @@MAIN
