* SPDX-License-Identifier: MIT
* Native CMS31 PROGRAM call; see TWO-SPACE-ABI.md and IBM CMSCALL.
         CSECT
         ENTRY @@MAIN
@@MAIN   BASR  12,0
         USING *,12
         LR    10,14
         LA    1,D0LIST
         SR    0,0
         SR    15,15
         SVC   204
         BR    10
         DS    0D
D0LIST   DC    CL8'D0CMCH'
         DC    CL8'D003ARG'
         DC    8X'FF'
         END   @@MAIN
