* SPDX-License-Identifier: MIT
* Native U64 neutral capability request above 4 GiB.
         CSECT
         ENTRY @@MAIN
@@MAIN   BASR  12,0
         USING *,12
         LG    1,P4HIGH
         LA    0,1
         ST    0,0(1)
         LA    0,64
         ST    0,4(1)
         SR    2,2
         SVC   200
         LTR   15,15
         BNZ   P4BAD
         LG    1,P4HIGH
         L     0,0(1)
         C     0,P4VERSION
         BNE   P4BAD
         L     0,4(1)
         C     0,P4LENGTH
         BNE   P4BAD
         SR    15,15
         SVC   3
P4BAD    LA    15,39
         SVC   3
         DS    0D
P4HIGH   DC    X'0000000110001A00'
P4VERSION DC   F'1'
P4LENGTH DC    F'64'
         END   @@MAIN
