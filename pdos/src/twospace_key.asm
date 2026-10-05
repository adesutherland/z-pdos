* SPDX-License-Identifier: MIT
* Privileged C31 storage-key setter for one real 4 KiB frame. The caller
* validates the real frame and supplies an SSKE key byte (X'80' or X'00').
         CSECT
         ENTRY TSKEYSET
TSKEYSET DS    0H
         L     2,0(,1)
         L     1,4(,1)
* GNU z900 assembler encodes SSKE R1,R2 as B2 2B 00 12.
         DC    X'B22B0012'
         BR    14
         END   TSKEYSET
