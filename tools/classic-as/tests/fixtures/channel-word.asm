* SPDX-License-Identifier: MIT
S        CSECT
         ENTRY CC
         DC X'AA'
CC       CCW1 7,DATA,X'40',6
         CCW1 X'1D',0,0,32767
DATA     DS 6C
         END CC
