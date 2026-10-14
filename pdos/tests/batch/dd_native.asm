* SPDX-License-Identifier: MIT; selected native services, standard C linkage.
         CSECT
         ENTRY D4LINK,D4UNDD,D4BAD,D4TPUT
D4LINK   STM   14,12,12(13)
         BASR  12,0
         USING *,12
         LA    1,PARMS
         LA    15,LINKLIST
         SVC   6
         ST    15,16(13)
         LM    14,12,12(13)
         BR    14
         DROP  12
D4UNDD   STM   14,12,12(13)
         BASR  12,0
         USING *,12
         LA    1,RBPTR
         SVC   99
         ST    15,16(13)
         LM    14,12,12(13)
         BR    14
         DROP  12
D4BAD    STM   14,12,12(13)
         SR    1,1
         SVC   251
         ST    15,16(13)
         LM    14,12,12(13)
         BR    14
D4TPUT   STM   14,12,12(13)
         BASR  12,0
         USING *,12
         LA    0,3
         LA    1,TERMMSG
         SVC   93
         ST    15,16(13)
         LM    14,12,12(13)
         BR    14
         DS    0F
LINKLIST DC    A(CHILD),X'80000000',F'0'
CHILD    DC    CL8'D4DDCH'
PARMS    DC    A(ARG+2147483648)
ARG      DC    F'0'
RBPTR    DC    A(RB+2147483648)
RB       DC    X'1402000000000000',A(TUS),F'0',F'0'
TUS      DC    A(TU+2147483648)
TU       DC    H'1',H'1',H'8',CL8'SYSIN'
TERMMSG  DC    C'BAD'
         END   D4LINK
