* SPDX-License-Identifier: MIT; ordinary native termination, bypassing stdio.
         CSECT
         ENTRY P3EXIT
P3EXIT   LA    15,37
         SVC   3
         END   P3EXIT
