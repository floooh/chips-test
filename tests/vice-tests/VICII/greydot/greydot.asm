;**************************************************************************
;*
;* FILE  greydot.asm
;* Copyright (c) 2010 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;* $Id: intro.asm,v 1.101 2009-06-02 20:49:58 tlr Exp $
;*
;* DESCRIPTION
;*
;******
	processor 6502


LINE		equ	52+8*7
	
	seg.u	zp
;**************************************************************************
;*
;* SECTION  zero page
;*
;******
	org	$02
line_zp:
	ds.b	1
color_zp:
	ds.b	1

	
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
	jsr	init_screen
	sei
	lda	#$35
	sta	$01

	ldx	#6
sa_lp1:
	lda	vectors-1,x
	sta	$fffa-1,x
	dex
	bne	sa_lp1

	jsr	wait_vb

	lda	#$7f
	sta	$dc0d
	lda	$dc0d
	lda	#$1b
	sta	$d011
	lda	#LINE
	sta	$d012
	jsr	reset_raster
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

	jsr	render_dots

	dec framecount
	bne skp
	lda #0
	sta $d7ff
skp

accstore	equ	.+1
	lda	#0
xstore	equ	.+1
	ldx	#0
ystore	equ	.+1
	ldy	#0
	rti


framecount: dc.b 10

render_dots:
rd_sm1:
	lda	#0
	ldx	#5
rd_lp1:
	sta	$d021		;4
	sta	$d021		;4
	sta	$d021		;4
	sta	$d021		;4
	sta	$d021		;4
	sta	$d021		;4
	sta	$d021		;4
	sta	$d021		;4
	sta	$d021		;4
	sta	$d021		;4
	ds.b	9,$ea		;18
	dex
	bne	rd_lp1


	ldx	#[4*63]/5
rd_lp2:
	dex
	bne	rd_lp2

	lda	#6
	sta	$d021
	
	lda	line_zp
	clc
	adc	#16
	sta	line_zp
	inc	color_zp
	lda	color_zp
	cmp	#8
	bne	rd_ex1
	jsr	reset_raster
rd_ex1:
	lda	line_zp
	sta	$d012

	ldx	#0
	lda	#$7f
	sta	$dc00
	lda	$dc01
	and	#%00010000
	bne	rd_skp1
	ldx	#8
rd_skp1:
	txa
	ora	color_zp
	sta	rd_sm1+1
	rts

reset_raster:
	lda	#LINE
	sta	line_zp
	lda	#0
	sta	color_zp
	rts

init_screen:
	lda	#<greet_msg
	ldy	#>greet_msg
	jsr	$ab1e

	sei
	lda	#6
	sta	$d020

	ldx	#0
	lda	#15
is_lp1:
	sta	$d800,x
	sta	$d900,x
	sta	$da00,x
	sta	$db00,x
	inx
	bne	is_lp1

	ldx	#80
is_lp2:
	lda	line-1,x
	sta	$0400+40*7-1,x
	sta	$0400+40*9-1,x
	sta	$0400+40*11-1,x
	sta	$0400+40*13-1,x
	sta	$0400+40*15-1,x
	sta	$0400+40*17-1,x
	sta	$0400+40*19-1,x
	sta	$0400+40*21-1,x
	dex
	bne	is_lp2

	lda	#$33
	sta	$01
;	ldx	#0
is_lp3:
	lda	$d000,x
	sta	$3800,x
	lda	$d100,x
	sta	$3900,x
	inx
	bne	is_lp3

	lda	#$35
	sta	$01

	ldx	#CHARS_LEN
is_lp4:
	lda	chars-1,x
	sta	$3c00-1,x
	dex
	bne	is_lp4

	lda	#$1e
	sta	$d018

	rts

greet_msg:
	dc.b	147
	dc.b	"GREYDOT / TLR",13,13
	dc.b	"TOP OF DOTTED LINES SHOULD BE GREY DOTS",13,13
	dc.b	"SHOWING COLORS 0-7",13
	dc.b	"HOLD SPACE TO SHOW COLORS 8-F",13
	dc.b	0

chars:
	dc.b	%00000000
	dc.b	%00000000
	dc.b	%00000000
	dc.b	%10000000
	dc.b	%10000000
	dc.b	%10000000
	dc.b	%10000000
	dc.b	%10000000

	dc.b	%10000000
	dc.b	%10000000
	dc.b	%10000000
	dc.b	%10000000
	dc.b	%10000000
	dc.b	%00000000
	dc.b	%10000000
	dc.b	%00000000

	dc.b	%10000000
	dc.b	%10000000
	dc.b	%10000000
	dc.b	%10000000
	dc.b	%10000000
	dc.b	%00000000
	dc.b	%00000000
	dc.b	%00000000

CHARS_LEN	equ	.-chars
	
line:
	dc.b	"  ",$80," "
	dc.b	"  ",$80," "
	dc.b	"  ",$80," "
	dc.b	"  ",$80," "
	dc.b	"  ",$80," "
	dc.b	"  ",$80," "
	dc.b	"  ",$80," "
	dc.b	"  ",$80," "
	dc.b	"  ",$80," "
	dc.b	"  ",$80," "

	dc.b	$81," ",$82," "
	dc.b	$81," ",$82," "
	dc.b	$81," ",$82," "
	dc.b	$81," ",$82," "
	dc.b	$81," ",$82," "
	dc.b	$81," ",$82," "
	dc.b	$81," ",$82," "
	dc.b	$81," ",$82," "
	dc.b	$81," ",$82," "
	dc.b	$81," ",$82," "
; eof
