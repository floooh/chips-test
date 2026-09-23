;**************************************************************************
;*
;* FILE  colorsplit.asm
;* Copyright (c) 2010 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;*
;* DESCRIPTION
;*
;******
	processor 6502

	
	seg.u	zp
;**************************************************************************
;*
;* SECTION  zero page
;*
;******
	org	$02

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
	lda	#$35
	sta	$01

	ldx	#6
sa_lp1:
	lda	vectors-1,x
	sta	$fffa-1,x
	dex
	bne	sa_lp1

	jsr	prepare_test
	jsr	wait_vb
	jsr	init_video

	lda	#$7f
	sta	$dc0d
	lda	$dc0d
	lda	#$1b
	sta	$d011
	lda	#0
	sta	rcnt
	jsr	set_raster
	lda	#1
	sta	$d019
	sta	$d01a

	cli
	
sa_lp2:
	if	0
	inx
	bpl	sa_lp2
	inc	$4080,x
	dec	$4080,x
	endif
	jmp	sa_lp2

vectors:
	dc.w	nmi_entry,0,irq_stable

nmi_entry:
	sei
	lda	#$37
	sta	$01
	jmp	$fe66
	
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
;* NAME  irq_stable, irq_stable2
;*   
;******
	align	256

irq_stable:
	sta	accstore	; 4
	sty	ystore		; 4
	stx	xstore		; 4
	inc	$d019		; 6
	inc	$d012		; 6
is_sm1:
	lda	#<irq_stable2	; 2
	sta	$fffe		; 4
	tsx			; 2
	cli			; 2
	ds.b	13,$ea		; 24
	if	0
is_lp1:
	sei
	inc	$d020
	jmp	is_lp1

irq_stable2:
	ds.b	13,$ea		; 26
	else
irq_stable2	equ	.-13
	endif
	txs			; 2
	dec	$d019		; 6
	dec	$d012		; 6
	lda	#<irq_stable	; 2
	sta	$fffe		; 4	=46
	lda	$d012		; 4
	eor	$d012		; 4
	beq	is2_skp1	; 2 (3)
is2_skp1:
is2_sm1:
	jsr	rast1		; 6
	jsr	set_raster

	cpx #0
	bne notend
	
  	dec framecount
  	bne skp
  	lda #0
  	sta $d7ff
skp

notend:
	
accstore	equ	.+1
	lda	#0
xstore	equ	.+1
	ldx	#0
ystore	equ	.+1
	ldy	#0
	rti

framecount: ds.b 5

	align	256

set_raster:
	ldx	rcnt
	lda	rtab,x
	sta	$d012
	lda	rtab+1,x
	lsr
	php
	lda	$d011
	asl
	plp
	ror	
	sta	$d011
	lda	rtab+2,x
	sta	is2_sm1+1
	lda	rtab+3,x
	sta	is2_sm1+2
	inx
	inx
	inx
	inx
	lda	rtab+3,x
	bne	sr_ex1
	tax
sr_ex1:	
	stx	rcnt
	rts

rcnt:
	dc.b	0
	
;**************************************************************************
;*
;* SECTION  payload
;*   
;******

prepare_test:
	lda	#$f0
	sta	$3fff
	sta	$39ff
	rts

init_video:
	rts

START	equ	60
DIST	equ	8
LEN	equ	4

COL_L	equ	0
COL_R	equ	1
		
	
rtab:
	dc.w	30,rast0
	dc.w	47,rast1
	dc.w	START-2+DIST*0,rast2
	dc.w	START-2+DIST*1,rast2
	dc.w	START-2+DIST*2,rast2
	dc.w	START-2+DIST*3,rast2
	dc.w	START-2+DIST*4,rast2
	dc.w	START-2+DIST*5,rast2
	dc.w	START-2+DIST*6,rast2
	dc.w	START-2+DIST*7,rast2
	dc.w	START-2+DIST*8,rast2
	dc.w	START-2+DIST*9,rast2
	dc.w	START-2+DIST*10,rast2
	dc.w	START-2+DIST*11,rast2
	dc.w	START-2+DIST*12,rast2
	dc.w	START-2+DIST*13,rast2
	dc.w	START-2+DIST*14,rast2
	dc.w	START-2+DIST*15,rast2
	dc.w	START-2+DIST*16,rast2
	dc.w	START-2+DIST*17,rast2
	dc.w	START-2+DIST*18,rast2
	dc.w	START-2+DIST*19,rast2
	dc.w	248,rast3
	dc.w	260,rast0
	dc.w	0,0

	align	256
rast0:
	ds.b	10,$ea
	ldx	#LEN
rs0_lp1:	
	lda	#COL_R		; 2
	sta	$d021		; 4
	ds.b	7,$ea		; 14
	lda	#COL_L		; 2
	sta	$d021		; 4
	ds.b	16,$ea		; 32
	dex			; 2
	bne	rs0_lp1		; 3
	rts

rast1:
	lda	#$1b
	sta	$d011
	lda	#0
	sta	test_index
	rts

	align	256
rast2:
	ldx	test_index
	lda	d016_tab,x
	sta	$d016
	lda	d011_tab,x
	sta	$d011

	ldx	#LEN
rs2_lp1:	
	lda	#COL_R		; 2
	sta	$d021		; 4
	ds.b	7,$ea		; 14
	lda	#COL_L		; 2
	sta	$d021		; 4
	ds.b	16,$ea		; 32
	dex			; 2
	bne	rs2_lp1		; 3

	inc	test_index
	rts

rast3:
	lda	#$03
	sta	$d011
	rts


test_index:
	dc.b	0
	
d016_tab:
	dc.b	$08,$09,$0a,$0b
	dc.b	$18,$19,$1a,$1b
	dc.b	$08,$09,$0a,$0b
	dc.b	$18,$19,$1a,$1b
	dc.b	$08,$09,$0a,$0b
d011_tab:
	dc.b	$1b,$1b,$1b,$1b
	dc.b	$1b,$1b,$1b,$1b
	dc.b	$3b,$3b,$3b,$3b
	dc.b	$3b,$3b,$3b,$3b
	dc.b	$5b,$5b,$5b,$5b
; eof
