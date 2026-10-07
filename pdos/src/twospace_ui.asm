* SPDX-License-Identifier: MIT; native Classic C31 neutral I/O boundary.
         CSECT
         ENTRY TUIIO
TUIIO    STM   14,12,12(13)
         L     0,0(1)
         L     2,8(1)
         L     1,4(1)
         SVC   200
         ST    15,16(13)
         LM    14,12,12(13)
         BR    14
         END   TUIIO
