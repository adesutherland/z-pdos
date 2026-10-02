TRKTEST  CSECT
         USING TRKTEST,12
LIST     TRKCALC MF=L
         TRKCALC FUNCTN=TRKCAP,UCB=(3),BALANCE=(5),RKDD=(4),           X
               REGSAVE=YES,MF=(E,LIST)
         BR    14
         END   TRKTEST
