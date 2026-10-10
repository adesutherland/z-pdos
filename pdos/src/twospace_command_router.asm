* SPDX-License-Identifier: MIT; U31 stateless router and native entry veneers.
         CSECT
         ENTRY TCMENTRY,TCMIO,TCMINIT,TCMSUB,TCMTSO,TCMPDOS
         EXTRN TCMROUTE
TCMENTRY BASR  12,0
         USING TCMENTRY+2,12
         L     15,TCMTARGET
         BALR  14,15
         LA    0,23
         SVC   197
         DC    X'0000'
TCMTARGET DC   V(TCMROUTE)
         DROP  12
TCMIO    STM   14,12,12(13)
         L     0,0(1)
         L     2,8(1)
         L     1,4(1)
         SVC   197
         ST    15,16(13)
         LM    14,12,12(13)
         BR    14
TCMINIT  LA    0,1
         SVC   197
         BR    14
TCMSUB   LA    0,2
         SVC   197
         BR    14
TCMTSO   LA    0,3
         SVC   197
         BR    14
TCMPDOS  LA    0,4
         SVC   197
         BR    14
         END   TCMENTRY
