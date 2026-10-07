* SPDX-License-Identifier: MIT
* Native CMS24 SVC202 with its inline error continuation, CALLTYP PROGRAM.
         CSECT
         ENTRY @@MAIN
@@MAIN   BASR  12,0
         USING *,12
         LR    10,14
         LA    1,D0LIST
         SVC   202
         DC    AL4(D0ERROR)
         BR    10
D0ERROR  BR    10
         DS    0D
D0LIST   DC    CL8'D0CMCH'
         DC    CL8'D003ARG'
         DC    8X'FF'
         END   @@MAIN
