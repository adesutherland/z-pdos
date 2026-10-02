* SPDX-License-Identifier: MIT
S        CSECT
         ENTRY E
         DC X'0011223344556677'
         ORG S+2
         DC X'ABCD'
         ORG ,
E        LR 1,2
         END E
