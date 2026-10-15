* SPDX-License-Identifier: MIT; checked native LINK, no package replacement.
         CSECT
         ENTRY FONEST,FOADDR
FONEST   STM   14,12,12(13)
         BASR  12,0
         USING *,12
         L     2,0(1)
         LTR   2,2
         BZ    FONFORE
         LA    1,FONPARMS
         B     FONCALL
FONFORE  SR    1,1
FONCALL  DS    0H
         LA    15,FONLINK
         SVC   6
         ST    15,16(13)
         LM    14,12,12(13)
         BR    14
         DS    0F
FONLINK  DC    A(FONNAME),X'80000000',F'0'
FONNAME  DC    CL8'FOWNCH'
* Selected native PARAM copies fullword values. Both words back the tail.
FONPARMS DC    A(FONARG),A(FONARG+4+2147483648)
FONARG   DC    H'5',CL5'BATCH',X'00'
         DROP  12
FOADDR   STM   14,12,12(13)
         BASR  12,0
         USING *,12
         LA    1,FOAPARMS
         LA    15,FOALINK
         SVC   6
         ST    15,16(13)
         LM    14,12,12(13)
         BR    14
         DS    0F
FOALINK  DC    A(FOANAME),X'80000000',F'0'
FOANAME  DC    CL8'BADDR31'
FOAPARMS DC    A(FOAARG),A(FOAARG+4+2147483648)
FOAARG   DC    H'3',CL3'TSO',X'000000'
         END   FONEST
