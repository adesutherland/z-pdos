* SPDX-License-Identifier: MIT
* Native PARAM child consumes a fullword argument and terminates its LINK RB.
         CSECT
         ENTRY @@MAIN
@@MAIN   BASR  12,0
         USING *,12
         L     1,0(,1)
         N     1,D0MASK
         L     15,0(,1)
         SVC   3
         DS    0F
D0MASK   DC    X'7FFFFFFF'
         END   @@MAIN
