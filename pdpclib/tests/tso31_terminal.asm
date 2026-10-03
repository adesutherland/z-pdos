* SPDX-License-Identifier: MIT
* Selected TSO/E terminal forms; no guest service is invoked by this fixture.
R0       EQU   0
R1       EQU   1
R2       EQU   2
R4       EQU   4
R6       EQU   6
R7       EQU   7
R15      EQU   15
TEST     CSECT
         USING TEST,12
         EXTRACT (R2),FIELDS=PSB
         GETLINE PARM=ZGETLINE,ECT=(R6),UPT=(R7),ECB=ZIOECB,           X
               MF=(E,ZIOPL)
         PUTLINE PARM=ZPUTLINE,ECT=(R6),UPT=(R7),ECB=ZIOECB,           X
               OUTPUT=((R4),DATA),TERMPUT=EDIT,MF=(E,ZIOPL)
ZPUTLINE PUTLINE MF=L
ZIOPL    DC    4F'0'
ZIOECB   DC    F'0'
ZGETLINE GETLINE MF=L
         END   TEST
