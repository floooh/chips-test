FAC			equ		$61
FACEXP		equ		$61
FACHO		equ		$62
FACSGN		equ		$66
FAC2		equ		$69
	
GIVAYF		equ		$B391	;Convert 16-Bit Signed Integer to Floating Point
MOV2F		equ		$BBC7	;Move a Floating Point Number from FAC1 to temp1
MOV2F2		equ		$BBCA	;Move a Floating Point Number from FAC1 to temp2
MOV2M		equ		$BBD4	;Move a Floating Point Number from FAC1 to memory .X Low Byte .Y High Byte
MOVFM		equ		$BBA2	;Move a Floating Point Number from Memory to FAC1 .A Low Byte .Y High Byte
MOVFA		equ		$BBFC	;Move a Floating Point Number from FAC2 to FAC1
MOVAF		equ		$BC0C	;Round and Move a Floating Point Number from FAC1 to FAC2
MOVEF		equ		$BC0F	;Copy FAC1 to FAC2 Without Rounding
FDIV		equ		$BB0F	;Divide a Number in Memory by FAC1
FDIVT		equ		$BB12	;Divide FAC2 by FAC1
MULT		equ		$BA28	;Multiply a Number in Memory by FAC1
FMULT		equ		$BA2B	;Multiply FAC1 with FAC2
FADD		equ		$B867	;Add a Number in Memory to FAC1
FADDT		equ		$B86A	;Add FAC1 to FAC2
FADDH		equ		$B849	;Add .5 to Contents of Floating Point Accumulator #1
QINT		equ		$BC9B	;Convert FAC1 into Signed Integer Within FAC1
QUINT		equ		$B7F7	;Convert FAC1 into Unsigned Integer at $0014-$0015
NEGOP		equ		$BFB4	;Negate
