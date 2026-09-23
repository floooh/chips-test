;**************************************************************************
;*
;* FILE  rasterirq_hold.asm
;* Copyright (c) 2010 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;* $Id: intro.asm,v 1.101 2009-06-02 20:49:58 tlr Exp $
;*
;* DESCRIPTION
;*
;******
	processor 6502


LINE		equ	$10	
END_LINE	equ	$30	
	
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

	jsr     makescreen

	lda	#$7f
	sta	$dc0d
	lda	$dc0d
	lda	#$1b
	sta	$d011
	lda	#LINE
	sta	$d012
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

	ldx	#LINE+2
	stx	$d012
	lda	#1
	sta	$d019

	jsr	hold_off
	
	dec framecount
	bne skp
	lda #0
	sta $d7ff
skp:

	lda	#LINE
	sta	$d012
accstore	equ	.+1
	lda	#0
xstore	equ	.+1
	ldx	#0
ystore	equ	.+1
	ldy	#0
	rti

framecount: dc.b 5

hold_off:
ho_lp1:
	lda	$d019		; 4
	sta	$d020		; 4
	if	0
	lda	#1		; 2
	sta	$d019		; 4
	else
	ds.b	3,$ea		; 6
	endif
	ds.b	11,$ea		; 22
	inx			; 2
 	stx	$d012		; 4  =42
	ds.b	8,$ea		; 16
	cpx	#END_LINE	; 2
	bne	ho_lp1		; 3  =63
	
	lda	$d019
	sta	$d020
	lda	#1
	sta	$d019

	ldx	#14
ho_lp2:
	dex
	bne	ho_lp2

	lda	#14
	sta	$d020
	rts
	
makescreen:
        lda #$20
        ldx #0
sclp1:
        sta $0400,x
        sta $0500,x
        sta $0600,x
        sta $0700,x
        inx
        bne sclp1
        ldx #39
sclp2:
        lda #"#"
        sta $0400,x
        lda #1
        sta $d800,x
        dex
        bpl sclp2
        jmp     wait_vb
