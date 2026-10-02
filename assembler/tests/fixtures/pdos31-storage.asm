* SPDX-License-Identifier: MIT
STORAGE  CSECT
         USING STORAGE,12
START    GETMAIN RU,LV=128,SP=5,LOC=BELOW
         GETMAIN RC,LV=(6),SP=0,LOC=ANY
         FREEMAIN RU,LV=(0),A=(1),SP=5
         FREEMAIN R,LV=64,A=(R10),SP=3
         BR    14
         LTORG
         END   START
