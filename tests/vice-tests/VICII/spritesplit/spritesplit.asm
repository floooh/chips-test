;**************************************************************************
;*
;* FILE  spritesplit.asm
;* Copyright (c) 2010 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;* $Id: intro.asm,v 1.101 2009-06-02 20:49:58 tlr Exp $
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
skp:
notend:

accstore	equ	.+1
	lda	#0
xstore	equ	.+1
	ldx	#0
ystore	equ	.+1
	ldy	#0
	rti

framecount: dc.b 5

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
	ifconst	SPLIT_HIRES_TO_MC
IDLE_PATTERN	equ	$00
SPR_PATTERN	equ	SPRITE_PATTERN_VALUE
COLOR_SPR	equ	1
COLOR_MC1	equ	13
COLOR_MC2	equ	0
REGISTER_UT	equ	$d01c
FIRST_VALUE	equ	$00
LAST_VALUE	equ	$01
INITIAL_MC	equ	0
INITIAL_EXPX	equ	0
	endif

	ifconst	SPLIT_HIRES_TO_MC_EXPANDED
IDLE_PATTERN	equ	$00
SPR_PATTERN	equ	SPRITE_PATTERN_VALUE
COLOR_SPR	equ	1
COLOR_MC1	equ	13
COLOR_MC2	equ	0
REGISTER_UT	equ	$d01c
FIRST_VALUE	equ	$00
LAST_VALUE	equ	$01
INITIAL_MC	equ	0
INITIAL_EXPX	equ	1
	endif

	ifconst	SPLIT_MC_TO_HIRES
IDLE_PATTERN	equ	$00
SPR_PATTERN	equ	SPRITE_PATTERN_VALUE
COLOR_SPR	equ	1
COLOR_MC1	equ	13
COLOR_MC2	equ	0
REGISTER_UT	equ	$d01c
FIRST_VALUE	equ	$01
LAST_VALUE	equ	$00
INITIAL_MC	equ	0
INITIAL_EXPX	equ	0
	endif

	ifconst	SPLIT_MC_TO_HIRES_EXPANDED
IDLE_PATTERN	equ	$00
SPR_PATTERN	equ	SPRITE_PATTERN_VALUE
COLOR_SPR	equ	1
COLOR_MC1	equ	13
COLOR_MC2	equ	0
REGISTER_UT	equ	$d01c
FIRST_VALUE	equ	$01
LAST_VALUE	equ	$00
INITIAL_MC	equ	0
INITIAL_EXPX	equ	1
	endif

	ifconst	SPLIT_HIRES_COLOR
IDLE_PATTERN	equ	$00
SPR_PATTERN	equ	SPRITE_PATTERN_VALUE
COLOR_SPR	equ	1
COLOR_MC1	equ	13
COLOR_MC2	equ	0
REGISTER_UT	equ	$d027
FIRST_VALUE	equ	$00
LAST_VALUE	equ	$01
INITIAL_MC	equ	0
INITIAL_EXPX	equ	0
	endif

	ifconst	SPLIT_MC_COLOR0
IDLE_PATTERN	equ	$00
SPR_PATTERN	equ	SPRITE_PATTERN_VALUE
COLOR_SPR	equ	1
COLOR_MC1	equ	13
COLOR_MC2	equ	0
REGISTER_UT	equ	$d027
FIRST_VALUE	equ	$00
LAST_VALUE	equ	$01
INITIAL_MC	equ	1
INITIAL_EXPX	equ	0
	endif

	ifconst	SPLIT_MC_COLOR1
IDLE_PATTERN	equ	$00
SPR_PATTERN	equ	SPRITE_PATTERN_VALUE
COLOR_SPR	equ	1
COLOR_MC1	equ	13
COLOR_MC2	equ	0
REGISTER_UT	equ	$d025
FIRST_VALUE	equ	$00
LAST_VALUE	equ	$01
INITIAL_MC	equ	1
INITIAL_EXPX	equ	0
	endif
	
	ifconst	SPLIT_MC_COLOR2
IDLE_PATTERN	equ	$00
SPR_PATTERN	equ	SPRITE_PATTERN_VALUE
COLOR_SPR	equ	1
COLOR_MC1	equ	13
COLOR_MC2	equ	0
REGISTER_UT	equ	$d026
FIRST_VALUE	equ	$00
LAST_VALUE	equ	$01
INITIAL_MC	equ	1
INITIAL_EXPX	equ	0
	endif

	ifconst	SPLIT_XPOS
IDLE_PATTERN	equ	$00
SPR_PATTERN	equ	SPRITE_PATTERN_VALUE
COLOR_SPR	equ	1
COLOR_MC1	equ	13
COLOR_MC2	equ	0
REGISTER_UT	equ	$d000
FIRST_VALUE	equ	116
LAST_VALUE	equ	150
INITIAL_MC	equ	0
INITIAL_EXPX	equ	0
	endif

	ifconst	SPLIT_UNEXP_EXP_HIRES
IDLE_PATTERN	equ	$00
SPR_PATTERN	equ	SPRITE_PATTERN_VALUE
COLOR_SPR	equ	1
COLOR_MC1	equ	0
COLOR_MC2	equ	13
REGISTER_UT	equ	$d01d
FIRST_VALUE	equ	0
LAST_VALUE	equ	1
INITIAL_MC	equ	0
INITIAL_EXPX	equ	0
	endif

	ifconst	SPLIT_EXP_UNEXP_HIRES
IDLE_PATTERN	equ	$00
SPR_PATTERN	equ	SPRITE_PATTERN_VALUE
COLOR_SPR	equ	1
COLOR_MC1	equ	0
COLOR_MC2	equ	13
REGISTER_UT	equ	$d01d
FIRST_VALUE	equ	1
LAST_VALUE	equ	0
INITIAL_MC	equ	0
INITIAL_EXPX	equ	0
	endif

	ifconst	SPLIT_UNEXP_EXP_MC
IDLE_PATTERN	equ	$00
SPR_PATTERN	equ	SPRITE_PATTERN_VALUE
COLOR_SPR	equ	1
COLOR_MC1	equ	0
COLOR_MC2	equ	13
REGISTER_UT	equ	$d01d
FIRST_VALUE	equ	0
LAST_VALUE	equ	1
INITIAL_MC	equ	1
INITIAL_EXPX	equ	0
	endif

	ifconst	SPLIT_EXP_UNEXP_MC
IDLE_PATTERN	equ	$00
SPR_PATTERN	equ	SPRITE_PATTERN_VALUE
COLOR_SPR	equ	1
COLOR_MC1	equ	0
COLOR_MC2	equ	13
REGISTER_UT	equ	$d01d
FIRST_VALUE	equ	1
LAST_VALUE	equ	0
INITIAL_MC	equ	1
INITIAL_EXPX	equ	0
	endif

	ifconst	SPLIT_PRI
IDLE_PATTERN	equ	$ff
SPR_PATTERN	equ	SPRITE_PATTERN_VALUE
COLOR_SPR	equ	1
COLOR_MC1	equ	10
COLOR_MC2	equ	0
REGISTER_UT	equ	$d01b
FIRST_VALUE	equ	0
LAST_VALUE	equ	1
INITIAL_MC	equ	0
INITIAL_EXPX	equ	0
	endif

	ifconst	SPLIT_PRI_EXP
IDLE_PATTERN	equ	$ff
SPR_PATTERN	equ	SPRITE_PATTERN_VALUE
COLOR_SPR	equ	1
COLOR_MC1	equ	10
COLOR_MC2	equ	0
REGISTER_UT	equ	$d01b
FIRST_VALUE	equ	0
LAST_VALUE	equ	1
INITIAL_MC	equ	0
INITIAL_EXPX	equ	1
	endif

	ifconst	SPLIT_PRI_MC
IDLE_PATTERN	equ	$ff
SPR_PATTERN	equ	SPRITE_PATTERN_VALUE
COLOR_SPR	equ	1
COLOR_MC1	equ	10
COLOR_MC2	equ	0
REGISTER_UT	equ	$d01b
FIRST_VALUE	equ	0
LAST_VALUE	equ	1
INITIAL_MC	equ	1
INITIAL_EXPX	equ	0
	endif

	ifconst	SPLIT_PRI_MC_EXP
IDLE_PATTERN	equ	$ff
SPR_PATTERN	equ	SPRITE_PATTERN_VALUE
COLOR_SPR	equ	1
COLOR_MC1	equ	10
COLOR_MC2	equ	0
REGISTER_UT	equ	$d01b
FIRST_VALUE	equ	0
LAST_VALUE	equ	1
INITIAL_MC	equ	1
INITIAL_EXPX	equ	1
	endif

prepare_test:
	lda	#IDLE_PATTERN
	sta	$3fff
	lda	#SPR_PATTERN
	ldx	#$3f-1
pt_lp1:
	sta	$3f00,x
	dex
	bpl	pt_lp1
	lda	#$fc
	sta	$07f8

	lda	#COLOR_SPR
	sta	$d027
	lda	#INITIAL_MC
	sta	$d01c
	lda	#INITIAL_EXPX
	sta	$d01d
	lda	#0
	sta	$d01b
	lda	#COLOR_MC1
	sta	$d025
	lda	#COLOR_MC2
	sta	$d026	
	rts

init_video:
	rts

old_value:
	dc.b	0
	
	
START	equ	54
STARTX	equ	8*12
DIST	equ	25

set_sprite:
	lda	#STARTX
	sta	$d000
	sta	$d010
	lda	#START
	sta	$d001
	lda	#1
	sta	$d015
	rts


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
	dc.w	248,rast3
	dc.w	260,rast0
	dc.w	0,0

	align	256
rast0:
	bit	$ea
	ds.b	4,$ea
	ldx	#11
rs0_lp1:	
	lda	#13		; 2
	sta	$d021		; 4
	ds.b	7,$ea		; 14
	lda	#5		; 2
	sta	$d021		; 4
	ds.b	16,$ea		; 32
	dex			; 2
	bne	rs0_lp1		; 3
	rts

rast1:
	lda	#$1b
	sta	$d011

	jsr	set_sprite

	lda	#FIRST_VALUE
	sta	REGISTER_UT
	
	rts

	align	256
rast2:
	nop
	nop
	bit	$ea

	ds.b	3,$ea
	lda	#13
	sta	$d021
	ds.b	7,$ea
	lda	#5
	sta	$d021

	lda	REGISTER_UT
	sta	old_value
	ds.b	11,$ea
	
	ldx	#11
rs_lp2:	
	lda	#LAST_VALUE
	sta	REGISTER_UT
	ds.b	6,$ea
	lda	old_value
	sta	REGISTER_UT
	ds.b	12,$ea
	bit	$ea
	dex
	bne	rs_lp2
			
	lda	$d001
	clc
	adc	#DIST
	sta	$d001
	inc	$d000
	rts

rast3:
	lda	#$03
	sta	$d011
	lda	#0
	sta	$d015
	rts
	
; eof
