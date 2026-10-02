* SPDX-License-Identifier: MIT
BSAMTEST CSECT
         USING BSAMTEST,12
LIST     READ  REQUEST,SF,,,,MF=L
         READ  REQUEST,SF,(10),(8),(9),MF=E
         WRITE REQUEST,SF,MF=E
         BR    14
         LTORG
         END   BSAMTEST
