* SPDX-License-Identifier: MIT
* Repeated U64 capability requests above 4 GiB. Clock is measurement only;
* every return is checked and the fixed completed-call count is published.
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
         L     8,P4CALLS
         STCK  64(1)
P4LOOP   DS    0H
         SVC   200
         LTR   15,15
         BNZ   P4BAD
         LG    1,P4HIGH
         BCT   8,P4LOOP
         STCK  72(1)
         L     0,P4CALLS
         ST    0,80(1)
* Legacy asynchronous diagnostic controls must not take or clear the v1
* console's pending input. These are not native TGET or v1 operations.
         SR    0,0
         SR    1,1
         SR    2,2
         SVC   209
         C     15,P4UNSUP
         BNE   P4BAD
         SVC   239
         C     15,P4UNSUP
         BNE   P4BAD
         LG    1,P4HIGH
         LA    0,2
         ST    0,84(1)
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
P4CALLS  DC    F'1000'
P4UNSUP  DC    F'20'
         END   @@MAIN
