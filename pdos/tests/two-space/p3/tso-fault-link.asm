* SPDX-License-Identifier: MIT
* Return K's separate R0 failure status to the C parent after its child faults.
         CSECT
         ENTRY P3LINK
P3LINK   STM   14,12,12(13)
         BASR  12,0
         USING *,12
         LA    1,D0PARMS
         LA    15,D0LINK
         SVC   6
         ST    0,16(13)
         LM    14,12,12(13)
         BR    14
         DS    0F
D0LINK   DC    A(D0NAME),X'80000000',F'0'
D0NAME   DC    CL8'D0TFLCH'
D0PARMS  DC    A(D0ARG+2147483648)
D0ARG    DC    F'0'
         END   P3LINK
