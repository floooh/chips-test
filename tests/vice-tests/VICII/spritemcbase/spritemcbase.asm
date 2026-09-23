;**************************************************************************
;*
;* FILE  spritemcbase.asm
;* Copyright (c) 2010 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;*
;* DESCRIPTION
;*
;******
	processor 6502


LINE		equ	56

	seg.u	zp
;**************************************************************************
;*
;* SECTION  zero page
;*
;******
	org	$02
ptr_zp:
	ds.w	1

	seg	code
	org	$0801
;**************************************************************************
;*
;* Basic line!
;*
;******
StartOfFile:
	dc.w	EndLine
	dc.w	0
	dc.b	$9e,"2069 /T.L.R/",0
;	        0 SYS2069 /T.L.R/
EndLine:
	dc.w	0

;**************************************************************************
;*
;* SysAddress... When run we will enter here!
;*
;******
SysAddress:
	sei
	ldx	#0
	ldy	#>[PAYLOAD_LEN+255]
sa_lp1:
sa_sm1:
	lda	payload_st,x
sa_sm2:
	sta	payload,x
	inx
	bne	sa_lp1
	inc	sa_sm1+2
	inc	sa_sm2+2
	dey
	bne	sa_lp1
	jmp	($8000)

	
	
	
	
payload_st:
	rorg	$8000
payload:
;**************************************************************************
;*
;* NAME  payload
;*
;* DESCRIPTION
;*   Auto starting test program
;*   
;******
	dc.w	reset
	dc.w	$fcf2
	dc.b	$c3,$c2,$cd,$38,$30

start_msg:
	dc.b	"SPRITE MCBASE TEST / TLR",13,13,0

menu_msg:
	dc.b	13,13,"1-8 TOGGLE ENABLES.",0

reset:
	sei
	cld
	ldx	#$ff
	txs

	ldx	#$2f
rs_lp0:
	lda	$d000-1,x
	sta	vtab-1,x
	dex
	bne	rs_lp0

	jsr	$fda3
	jsr	clone_fd50
	jsr	clone_fd15
	jsr	clone_ff5b

	lda	#<start_msg
	ldy	#>start_msg
	jsr	print

	jsr	dump_vram

	lda	#<menu_msg
	ldy	#>menu_msg
	jsr	print

main_loop:
	cli
rs_lp00:
	jsr	$ffe4
	beq	rs_lp00
	cmp	#"1"
	beq	trigger
	cmp	#"2"
	beq	trigger
	cmp	#"3"
	beq	trigger
	cmp	#"4"
	beq	trigger
	cmp	#"5"
	beq	trigger
	cmp	#"6"
	beq	trigger
	cmp	#"7"
	beq	trigger
	cmp	#"8"
	beq	trigger
	jmp	rs_lp00

trigger:
	sec
	sbc	#"1"
	pha

	sei
	ldx	#63
rs_lp1:
	lda	sprite-1,x
	sta	$3f00-1,x
	dex
	bne	rs_lp1
	
	ldx	#8
tr_lp2:
	lda	#$fc
	sta	$07f8-1,x
	lda	#1
	sta	$d027-1,x
	dex
	bne	tr_lp2

	ldx	#0
	ldy	#24
tr_lp3:
	tya
	sta	$d000,x
	clc
	adc	#24
	tay
	lda	#160
	sta	$d001,x
	inx
	inx
	cpx	#$10
	bne	tr_lp3
	lda	#0
	sta	$d010
	
	jsr	wait_vb
	ldy	#100
rs_lp2:
	cpy	$d012
	bne	rs_lp2

	pla
	tax
	lda	tab,x	
	eor	$d015
	sta	$d015

	jmp	main_loop

tab:
	dc.b	%00000001
	dc.b	%00000010
	dc.b	%00000100
	dc.b	%00001000
	dc.b	%00010000
	dc.b	%00100000
	dc.b	%01000000
	dc.b	%10000000

sprite:
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000
	dc.b	%11111111,%00000000,%00000000


;**************************************************************************
;*
;* NAME  dump_vram
;*
;* DESCRIPTION
;*   Dump the preserved vram contents.
;*
;******
dump_vram:
	ldx	#0
	ldy	#0
dv_lp1:
	jsr	print_space
	lda	vtab,x
	jsr	print_hex
	iny
	tya
	and	#$07
	bne	dv_skp1
	jsr	print_cr
dv_skp1:
	inx
	cpx	#$2f
	bne	dv_lp1
	rts

	
;**************************************************************************
;*
;* NAME  clone_fd50
;*
;* DESCRIPTION
;*   Clone the memory init at $fd50 without memory test
;*
;******
clone_fd50:
	lda	#0
	tay
cfd50_lp1:
	sta	$0002,y
	sta	$0200,y
	sta	$0300,y
	iny
	bne	cfd50_lp1

	ldx	#$03
	lda	#$3c
	sta	$b2
	stx	$b3

	ldx	#$00
	ldy	#$a0
	jmp	$fd8c	;Set MemBounds

;**************************************************************************
;*
;* NAME  clone_fd15
;*
;* DESCRIPTION
;*   Clone the vector setup at $fd15 without trashing ram under kernal.
;*
;******
clone_fd15:
	ldy	#$1f
cfd15_lp1:
	lda	$fd30,y
	sta	$0314,y
	dey
	bpl	cfd15_lp1
	rts

;**************************************************************************
;*
;* NAME  clone_ff5b
;*
;* DESCRIPTION
;*   Clone the video init at $ff5b with minimal change of vic registers.
;*
;******
clone_ff5b:
	lda	#3
	sta	$9a
	lda	#0
	sta	$99
	lda	#$1b
	sta	$d011
	lda	#$08
	sta	$d016
	lda	#14
	sta	$d020
	sta	646
	lda	#6
	sta	$d021
	lda	#$15
	sta	$d018
	jsr	$e51b
	rts

;**************************************************************************
;*
;* NAME  wait_vb
;*   
;******
wait_vb:
wv_lp1:
	bit	$d011
	bpl	wv_lp1
wv_lp2:
	bit	$d011
	bmi	wv_lp2
	rts

;**************************************************************************
;*
;* NAME  print
;*
;* DESCRIPTION
;*   output string
;*   
;******
print:
	sta	ptr_zp
	sty	ptr_zp+1
	ldy	#0
prt_lp1:
	lda	(ptr_zp),y
	beq	prt_ex1
	jsr	$ffd2
	iny
	bne	prt_lp1
	inc	ptr_zp+1
	jmp	prt_lp1
prt_ex1:
	rts


;**************************************************************************
;*
;* NAME  print_space, print_cr
;*
;* DESCRIPTION
;*   print routines for common chars.
;*   
;******
print_space:
	lda	#" "
	dc.b	$2c
print_cr:
	lda	#13
	jmp	$ffd2

;**************************************************************************
;*
;* NAME  print_hex
;*
;* DESCRIPTION
;*   output hex byte.
;*   
;******
print_hex:
	pha
	lsr
	lsr
	lsr
	lsr
	jsr	ph_skp1
	pla
	and	#$0f
ph_skp1:
	clc
	adc	#"0"
	cmp	#"9"+1
	bcc	ph_skp2
	adc	#"A"-("9"+1)-1
ph_skp2:
	jmp	$ffd2



;**************************************************************************
;*
;* NAME  tables
;*   
;******
vtab:
	ds.b	$2f


	rend
PAYLOAD_LEN	equ	.-payload_st	
; eof
