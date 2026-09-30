CODE     CSECT
         AMODE 31
         RMODE ANY
         ENTRY START
         EXTRN EXT
         LR    3,4
START    LR    1,2
         DC    A(START)
         DC    V(EXT)
         DC    A(EXT)
         DS    2F
SECOND   CSECT
         AMODE 24
         RMODE 24
         DC    A(START)
         END   START
