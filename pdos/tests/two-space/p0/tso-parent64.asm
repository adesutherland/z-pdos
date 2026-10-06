* SPDX-License-Identifier: MIT
* Native AMODE64 SVC6/LINKX caller. First subset uses low control pointers.
         CSECT
         ENTRY @@MAIN
@@MAIN   BASR  12,0
         USING *,12
* GNU z900 LGR R10,R14: preserve the complete return address.
         DC    X'B90400AE'
         LA    1,D0PARMS
         LA    15,D0LINK
         SVC   6
         BR    10
D0ERRET  LA    15,39
         BR    10
         DS    0F
D0LINK   DC    A(D0NAME),X'80000000',A(D0ERRET)
D0NAME   DC    CL8'D0TSCH'
D0PARMS  DC    A(D0ARG+2147483648)
D0ARG    DC    F'37'
         END   @@MAIN
