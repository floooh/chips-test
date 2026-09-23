	.code

	.word	basicLoader
basicLoader:
	; 2014 SYS(2080):PW.SOFT.
	.byte	$16, $08, $de, $07, $9e, $28, $32, $30
	.byte	$38, $30, $29, $3a, $50, $57, $2E, $53
	.byte	$4F, $46, $54, $2E, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00
