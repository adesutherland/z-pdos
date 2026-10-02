* SPDX-License-Identifier: MIT; unsupported parameter-list failure.
CALLER   CSECT
         USING CALLER,12
         CALL CALLEE,(VALUE)
VALUE    DS F
         END CALLER
