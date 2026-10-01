* SPDX-License-Identifier: MIT
DIRTEST  CSECT
         USING DIRTEST,12
         BLDL  (R10),AREA
         FIND  (R10),MEM,D
         BR    14
         DS    0F
AREA     DS    12C
MEM      DC    CL8'TEST'
         END   DIRTEST
