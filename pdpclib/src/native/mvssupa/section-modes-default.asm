         CSECT ,
*
* Switch temporarily to this other CSECT, so that we can
* set AMODE/RMODE upfront
*
         AIF   ('&ZSYS' EQ 'S370').NOAM  AMODE on z/OS so no warning
@@PCLST  AMODE ANY
@@PCLST  RMODE ANY
.NOAM    ANOP  ,        S/370 doesn't have AMODE/RMODE
@@PCLST  CSECT ,
*
* Back to normal
