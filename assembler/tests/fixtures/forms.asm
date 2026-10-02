* Independently expected historical format fields, no macros or literals.
FORMS    CSECT
         LR    1,2
         AR    3,4
         SR    5,6
         BCR   15,14
         L     1,4095(2,3)
         ST    4,0(,5)
         SLL   6,31
         CLI   0(8),X'FF'
         TM    4095(9),X'80'
         MVC   0(1,10),0(11)
         CLC   4095(256,12),4095(13)
         END   FORMS
