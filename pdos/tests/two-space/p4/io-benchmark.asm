* SPDX-License-Identifier: MIT
* Own a U64 span rather than relying on a diagnostic image's fixed mapping.
* All operation counts and native return codes are fixed; clocks measure cost.
         CSECT
         ENTRY @@MAIN
@@MAIN   BASR  12,0
         USING *,12
         L     0,P25SIZE
* LGHI R1/R2/R3,0: clear complete GPRs before full-width service/index use.
         DC    X'A7190000A7290000A7390000'
         SVC   223
         LTR   15,15
         BNZ   P25BAD
         LA    0,52
         ST    0,P25FAIL
         STG   1,P25BUF
         L     8,P25PAGES
P25FILL  MVI   0(1),X'5A'
         MVC   1(255,1),0(1)
* AGHI R1,256 (z/Architecture RI encoding).
         DC    X'A71B0100'
         BCT   8,P25FILL
         LG    1,P25BUF
         LA    0,1
         ST    0,0(1)
         LA    0,64
         ST    0,4(1)
         L     8,P25CALLS
         STCK  256(1)
P25CALL  SVC   200
         LTR   15,15
         BNZ   P25BAD
         LG    1,P25BUF
         BCT   8,P25CALL
         STCK  264(1)
         L     0,P25CALLS
         ST    0,272(1)
         LA    0,128
         ST    0,4(1)
         LA    0,53
         ST    0,P25FAIL
         LA    0,128
         SR    2,2
         SVC   198
         LTR   15,15
         BNZ   P25BAD
         LA    0,54
         ST    0,P25FAIL
         LG    1,P25BUF
         LA    6,P25LINE
         LA    7,P25HEX
         LA    9,3
         LA    0,135
         ST    0,P25REQ+12
P25PART  LA    8,64
         MVC   0(7,6),P25PREFIX
         C     9,P25THREE
         BNE   P25NOTA
         MVI   5(6),C'A'
         B     P25CONV
P25NOTA  C     9,P25TWO
         BNE   P25C
         MVI   5(6),C'B'
         B     P25CONV
P25C     MVI   5(6),C'C'
         LA    8,32
         LA    0,71
         ST    0,P25REQ+12
         LG    1,P25BUF
         DC    X'A71B0100'
P25CONV  LA    10,7(6)
P25BYTE  SR    2,2
         IC    2,0(1)
         LR    3,2
         SRL   3,4
         IC    4,0(3,7)
         STC   4,0(10)
         N     2,P25NIB
         IC    4,0(2,7)
         STC   4,1(10)
* AGHI R1,1.
         DC    X'A71B0001'
         LA    10,2(10)
         BCT   8,P25BYTE
         STG   1,P25NEXT
         LA    0,32
         LA    1,P25REQ
         LA    2,1
         SVC   200
         LTR   15,15
         BNZ   P25BAD
         LG    1,P25NEXT
         BCT   9,P25PART
         SR    15,15
         SVC   3
P25BAD   L     15,P25FAIL
         SVC   3
         DS    0D
P25BUF   DC    X'0000000000000000'
P25NEXT  DC    X'0000000000000000'
P25SIZE  DC    F'65536'
P25FAIL  DC    F'51'
P25PAGES DC    F'256'
P25CALLS DC    F'1000'
P25NIB   DC    X'0000000F'
P25THREE DC    F'3'
P25TWO   DC    F'2'
P25HEX   DC    CL16'0123456789ABCDEF'
P25PREFIX DC   CL7'PD25 A '
P25LINE  DC    CL135' '
P25REQ   DC    F'1',F'32',F'0',F'135',F'0',A(P25LINE),F'0',F'0'
         END   @@MAIN
