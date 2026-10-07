* SPDX-License-Identifier: MIT
* Open a child-owned native input, then fault with a defined illegal opcode.
         CSECT
         ENTRY @@MAIN
@@MAIN   BASR  12,0
         USING *,12
         LA    1,D0READ
         SR    0,0
         SR    15,15
         SVC   204
         LTR   15,15
         BNZ   D0FAIL
         DC    X'0000'
D0FAIL   LA    15,39
         BR    14
         DS    0D
D0READ   DC    CL8'RDBUF',CL8'D0CUR',CL8'DATA',CL2'A1'
         DC    H'1',A(D0BUF),F'8',F'0',F'0'
D0BUF    DS    CL8
         END   @@MAIN
