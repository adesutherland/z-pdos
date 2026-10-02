*        Use the supported SWA lookup for z/OS 1.5 JFCB tokens.
*        UNAUTH=YES requires AM31; the C caller remains AM24.
         SAM31
         XC    OPWSWA(OPWSWAL),OPWSWA
         LA    R1,OPWSVA
         ST    R1,OPWEPA
         MVC   OPWSVA+4(3),TIOEJFCB
         SWAREQ FCODE=RL,EPA=OPWEPA,MF=(E,OPWSWA),UNAUTH=YES
         LTR   R15,R15
         BNZ   DDCERR31
         ICM   R6,15,OPWSVA
         BZ    DDCERR31
         MVC   MYJFCB(JFCBLGTH),0(R6)
         SAM24
         B     DDCJOIN
DDCERR31 SAM24
         B     OPSERR
DDCJOIN  OI    DDWFLAG2,CWFDD     DD FOUND
