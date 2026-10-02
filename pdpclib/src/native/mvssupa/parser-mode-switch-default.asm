*
*
         AIF   ('&ZSYS' EQ 'S370').NODSNS3  Only S/380+90 needs a stub
         AIF   ('&OS' EQ 'PDOS').NODSNS3

         L     R3,=A(DSNCBOA) the DSN check stub needs this too
         ST    R2,0(,R3)
.NODSNS3 ANOP  ,                  Only S/380 etc needs a stub
*
*
