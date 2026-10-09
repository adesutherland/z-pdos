* SPDX-License-Identifier: MIT; selected native LINK and TPUT/TGET forms.
         CSECT
         ENTRY P4LINK,P4WIDE,P4LOW,P4PUT,P4GET,P4READ
P4LINK   STM   14,12,12(13)
         BASR  12,0
         USING *,12
         LA    1,P4PARMS
         LA    15,P4LINKPL
         SVC   6
         ST    15,16(13)
         LM    14,12,12(13)
         BR    14
         DS    0F
P4LINKPL DC    A(P4NAME),X'80000000',F'0'
P4NAME   DC    CL8'LAVM65O'
P4PARMS  DC    A(P4ARG+2147483648)
P4ARG    DC    H'2',CL2'-v'
         DROP  12
P4WIDE   STM   14,12,12(13)
         BASR  12,0
         USING *,12
         SR    1,1
         LA    15,P4WIDELINK
         SVC   6
         ST    15,16(13)
         LM    14,12,12(13)
         BR    14
         DS    0F
P4WIDELINK DC A(P4WIDENAME),X'80000000',F'0'
P4WIDENAME DC CL8'P4CAP64'
         DROP  12
P4LOW    STM   14,12,12(13)
         BASR  12,0
         USING *,12
         L     0,P4LOWSIZE
         SR    1,1
         LA    15,16
         SVC   120
         LTR   15,15
         BNZ   P4LOWBAD
         ST    1,16(13)
         LM    14,12,12(13)
         BR    14
P4LOWBAD SR    15,15
         ST    15,16(13)
         LM    14,12,12(13)
         BR    14
         DS    0F
P4LOWSIZE DC   F'16384'
         DROP  12
P4PUT    STM   14,12,12(13)
         BASR  12,0
         USING *,12
         L     0,0(1)
         L     1,4(1)
         O     1,P4OUTFLAG
         SVC   93
         ST    15,16(13)
         LM    14,12,12(13)
         BR    14
P4OUTFLAG DC   X'03000000'
         DROP  12
P4GET    STM   14,12,12(13)
         BASR  12,0
         USING *,12
         L     0,0(1)
         L     1,4(1)
         O     1,P4INFLAG
         SVC   93
         ST    15,16(13)
         LM    14,12,12(13)
         BR    14
P4INFLAG DC   X'83000000'
         DROP  12
* Raw TGET returns the actual byte count as well as its status.
P4READ   STM   14,12,12(13)
         BASR  12,0
         USING *,12
         L     8,8(1)
         L     0,0(1)
         L     1,4(1)
         O     1,P4READFL
         SVC   93
         ST    1,0(8)
         ST    15,16(13)
         LM    14,12,12(13)
         BR    14
P4READFL DC   X'83000000'
         END   P4LINK
