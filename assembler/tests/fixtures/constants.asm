* Original bootstrap width, sign, target characters and alignment fixture.
CONSTS   CSECT
CONSTS   AMODE 24
CONSTS   RMODE 24
         DC    X'7F'
         DS    0H
HMIN     DC    H'-32768'
HMAX     DC    H'32767'
         DS    0F
FMIN     DC    F'-2147483648'
FMAX     DC    F'2147483647'
         DS    0D
ADPOS    DC    AD(1048575)
ADNEG    DC    AD(-1)
AFOUR    DC    A(1048575)
CHARS    DC    CL4'ABCD'
         END   CONSTS
