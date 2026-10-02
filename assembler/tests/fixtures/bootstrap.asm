CODE     CSECT
CODE     AMODE 31
CODE     RMODE ANY
         ENTRY START
         EXTRN EXT
         LR    3,4
START    LR    1,2
         DC    A(START)
         DC    V(EXT)
         DC    A(EXT)
         DS    2F
SECOND   CSECT
SECOND   AMODE 24
SECOND   RMODE 24
         DC    A(START)
         END   START
