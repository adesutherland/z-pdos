* SPDX-License-Identifier: MIT
* U31 startup and console linkage for the diagnostic PCOMM profile.
* K supplies the above-line C stack; PDPCLIB START owns C initialisation.
         CSECT
         ENTRY @@CRT0,@@EXITA,TSUTPUT,TSUTGET
         EXTRN @@START
@@CRT0   DS    0H
         BASR  12,0
         USING @@CRT0+2,12
         L     2,0(,1)
         LA    3,TSUNAME
         LA    4,TSUBATCH
         STM   2,4,TSUARGS
         LA    1,TSUARGS
         L     15,TSUSTART
         BALR  14,15
         SVC   3
@@EXITA  L     15,0(,1)
         SVC   3
TSUNAME  DC    CL9'COMMAND '
         DS    0F
TSUBATCH DC    F'0'
TSUARGS  DS    3A
TSUSTART DC    V(@@START)
         DROP  12
TSUTPUT  DS    0H
         STM   14,12,12(13)
         L     0,0(,1)
         L     1,4(,1)
         SVC   93
         ST    15,16(,13)
         LM    14,12,12(13)
         BR    14
TSUTGET  DS    0H
         STM   14,12,12(13)
         BASR  12,0
         USING *,12
         L     0,0(,1)
         L     1,4(,1)
         O     1,TSUHIGH
         SVC   93
         LTR   15,15
         BNZ   TSUFAIL
         ST    1,16(,13)
         B     TSUDONE
TSUFAIL  LA    15,4095
         ST    15,16(,13)
TSUDONE  LM    14,12,12(13)
         BR    14
         DS    0F
TSUHIGH  DC    X'80000000'
         END   @@CRT0
