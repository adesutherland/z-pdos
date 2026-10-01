* SPDX-License-Identifier: MIT
SYNCTEST CSECT
         USING SYNCTEST,12
         EXCP  (5)
         WAIT  ECB=(2)
         EOV   (10)
         CHECK (8)
         BR    14
         LTORG
         END   SYNCTEST
