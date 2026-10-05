* SPDX-License-Identifier: MIT
* Single-CPU privileged purge after a live DAT entry change. PTLB is
* z/Architecture opcode B20D. The production caller must serialize table
* mutation and must not enable additional CPUs without a wider invalidation.
         CSECT
         ENTRY TSFPURGE
TSFPURGE DS    0H
         DC    X'B20D0000'
         BR    14
         END   TSFPURGE
