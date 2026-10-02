* SPDX-License-Identifier: MIT
DIAGTEST CSECT
         USING DIAGTEST,12
LIST     SNAP  PDATA=(PSW,REGS),LIST=AREA,STRHDR=HDR,MF=L
         SNAP  DCB=AREA,ID=(R7),MF=(E,(R9))
         ABEND 123
         ABEND 1,DUMP
         NOTE  (R10)
         POINT (R10),AREA
         BR    14
AREA     DS    2A
HDR      DC    A(0)
         LTORG
         END   DIAGTEST
