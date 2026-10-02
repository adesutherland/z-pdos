* SPDX-License-Identifier: MIT
WTOTEST  CSECT
         USING WTOTEST,12
MSG      WTO   'AB',ROUTCDE=11
TAIL     EQU   *-8-2,2,C'C'
         WTO   'C',ROUTCDE=1,DESC=12
LIST     WTO   'D',ROUTCDE=16,DESC=1,MF=L
         DC    AL2(TAIL-MSG),AL2(L'TAIL)
         END   WTOTEST
