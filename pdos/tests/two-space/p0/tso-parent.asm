* SPDX-License-Identifier: MIT
* Native LINK/SVC6: 12-byte control list, fenced one-address PARAM list.
* IBM MVS Diagnosis SVC6 and LINK/LINKX standard/execute forms.
         CSECT
         ENTRY @@MAIN
@@MAIN   BASR  12,0
         USING *,12
         LR    10,14
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
