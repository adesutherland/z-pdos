* SPDX-License-Identifier: MIT
* Native token-list child: exact D003ARG token gives application RC37.
         CSECT
         ENTRY @@MAIN
@@MAIN   BASR  12,0
         USING *,12
         LA    15,38
         CLC   8(8,1),D0ARG
         BNE   D0DONE
         LA    15,37
D0DONE   BR    14
D0ARG    DC    CL8'D003ARG'
         END   @@MAIN
