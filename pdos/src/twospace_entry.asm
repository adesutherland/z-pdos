* SPDX-License-Identifier: MIT
* Fixed C31 trampoline target. The C compiler may place helper routines
* ahead of pdosTwoSpaceService; the K64 gate enters this module at its base.
         CSECT
         ENTRY TSMENTRY
         EXTRN P0D3PCAA
TSMENTRY DS    0H
         BASR  12,0
         USING TSMENTRY+2,12
         L     15,TSMTARGET
         BR    15
TSMTARGET DC   A(P0D3PCAA)
         END   TSMENTRY
