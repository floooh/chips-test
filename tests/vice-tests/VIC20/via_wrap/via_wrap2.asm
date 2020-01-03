;**************************************************************************
;*
;* FILE  via_wrap2.asm 
;* Copyright (c) 2009 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;*
;* DESCRIPTION
;*   Test case for via timer wrap around.
;*
;******
	processor 6502
	include	"macros.i"

SCR_BASE	equ	$1e00
COL_BASE	equ	$9600

tmp_zp	equ	$fb
ptr_zp	equ	$fc

	seg	code
	org	$1001
;**************************************************************************
;*
;* Basic line!
;*
;******
start_of_line:
	dc.w	end_line
	dc.w	0
	dc.b	$9e,"4117 /T.L.R/",0
;	        0 SYS4117 /T.L.R/
end_line:
	dc.w	0

;**************************************************************************
;*
;* NAME  startofcode
;*
;******
startofcode:
	lda	#<greet_msg
	ldy	#>greet_msg
	jsr	$cb1e

	ldx	#0
	lda	646
soc_lp1:
	sta	COL_BASE,x
	sta	COL_BASE+$0100,x
	inx
	bne	soc_lp1

	ldx	#22*2
soc_lp2:
	lda	reference-1,x
	sta	SCR_BASE+22*3-1,x
	dex
	bne	soc_lp2
	
	lda	#$24
	ldx	#%00000000	; single
	jsr	perform_test
	lda	#$25
	ldx	#%00000000	; single
	jsr	perform_test
	
	lda	#<continuous_msg
	ldy	#>continuous_msg
	jsr	$cb1e

	lda	#$24
	ldx	#%01000000	; continuous
	jsr	perform_test
	lda	#$25
	ldx	#%01000000	; continuous
	jsr	perform_test

soc_lp4:
	jmp	soc_lp4


perform_test:
	sei
	sta	pt_sm2+1
	ldy	#0
pt_lp1:
	lda	#$7f
	sta	$912e
	sta	$912d
	stx	$912b
	lda	#18
	sta	$9124
	sty	tmp_zp
	lda	#BR_LEN-1
	sec
	sbc	tmp_zp	
	sta	pt_sm1+1
	clc
	lda	#0
	sta	$9125	;load T1
pt_sm1:
	bcc	pt_sm1
pt_branch:
	ds.b	20,$a9
	dc.b	$24,$ea
BR_LEN	equ	. - pt_branch

pt_sm2:
	lda	$9124
	sta	($d1),y
	iny
	cpy	#BR_LEN
	bne	pt_lp1

	cli
	lda	#13
	jmp	$ffd2
;	rts

greet_msg:
	dc.b	147,"VIA WRAP2 / TLR",13,13
	dc.b	"REFERENCE:",13,13,13,13
	dc.b	"MEASURED (SINGLE):",13
	dc.b	0
continuous_msg:
	dc.b	13
	dc.b	"MEASURED (CONT):",13
	dc.b	0

reference:
	dc.b	10,9,8,7,6,5,4,3,2,1,0,255,18,17,16,15,14,13,12,11,10,9		
	dc.b	0,0,0,0,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,0,0,0
; eof

	
