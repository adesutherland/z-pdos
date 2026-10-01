* SPDX-License-Identifier: MIT; original selected CALL consumer.
CALLER   CSECT
         USING CALLER,12
         ENTRY CALLEE
         CALL CALLEE
CALLEE   BR 14
         LTORG
         END CALLER
