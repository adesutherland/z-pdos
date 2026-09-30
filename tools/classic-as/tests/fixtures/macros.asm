* Original project fixture; MIT. No external service definitions.
         MACRO
&LABEL   LOADVALUE &REG,&SOURCE
&LABEL   L     &REG,&SOURCE
         L     &REG,0(&REG)
         MEND
         MACRO
&LABEL   LOADADDR &REG,&SOURCE
&LABEL   L     &REG,&SOURCE
         MEND
         MACRO
&LABEL   STOREVALUE &REG,&SOURCE,&WORK=14
&LABEL   L     &WORK,&SOURCE
         ST    &REG,0(&WORK)
         MEND
MACREF   CSECT
FIRST    LOADVALUE 4,16(1)
         LOADADDR 5,20(1)
         STOREVALUE 6,24(1)
         STOREVALUE 6,28(1),WORK=7
         END FIRST
