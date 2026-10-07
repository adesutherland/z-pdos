* SPDX-License-Identifier: MIT
* Native CMS31 parent retains an open cursor across a same-personality call.
         CSECT
         ENTRY @@MAIN
@@MAIN   BASR  12,0
         USING *,12
         LR    10,14
         LA    1,D0READ
         SR    0,0
         SR    15,15
         SVC   204
         LTR   15,15
         BNZ   D0FAIL
         CLC   D0BUF(3),D0ONE
         BNE   D0FAIL
         LA    1,D0CALL
         SR    0,0
         SR    15,15
         SVC   204
         C     15,D0RC
         BNE   D0FAIL
         MVI   D0READ+27,0
         LA    1,D0READ
         SR    0,0
         SR    15,15
         SVC   204
         LTR   15,15
         BNZ   D0FAIL
         CLC   D0BUF(3),D0TWO
         BNE   D0FAIL
         LA    15,37
         BR    10
D0FAIL   LA    15,39
         BR    10
         DS    0D
D0CALL   DC    CL8'D0FILECH',CL8'D003ARG',8X'FF'
D0READ   DC    CL8'RDBUF',CL8'D0CUR',CL8'DATA',CL2'A1'
         DC    H'1',A(D0BUF),F'8',F'0',F'0'
D0BUF    DS    CL8
D0ONE    DC    CL3'ONE'
D0TWO    DC    CL3'TWO'
         DS    0F
D0RC     DC    F'37'
         END   @@MAIN
