* SPDX-License-Identifier: MIT; original save-area consumer.
LINKTST  CSECT
ENTER    SAVE  (0,11),,ID
         RETURN (0,11),RC=(15)
ZERO     SAVE  (14,12)
         RETURN (14,12),RC=0
         END   ENTER
