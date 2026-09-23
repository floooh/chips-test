;**************************************************************************
;*
;* FILE  spritey.asm
;* Copyright (c) 2010 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;* $Id: intro.asm,v 1.101 2009-06-02 20:49:58 tlr Exp $
;*
;* DESCRIPTION
;*   Check Y range on which sprite collisions can happen.
;*
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
tmp_zp:
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
	sei

	
	ldx	#0
sa_lp1:
	lda #$00
	sta	$3f00,x
	lda #$ff
	sta BUFFER,x
	sta BUFFER+$100,x
	inx
	bne	sa_lp1
	lda	#$80
	sta	$3f00

	lda	#$fc
	sta	$07f8
	sta	$07f9

	lda	#0
	sta	$d015
	sta	$d01c
	sta	$d01d
	sta	$d01b
	sta	$d010

	lda	#128
	sta	$d000
	sta	$d002


	
	lda	#<irq_server
	sta	$0314
	lda	#>irq_server
	sta	$0315

	lda	#$7f
	sta	$dc0d
	lda	$dc0d
	jsr	set_raster
	lda	#1
	sta	$d019
	sta	$d01a


	lda #0
	sta $0401
	sta $0400
	
	cli
sa_lp2:

    lda #2
chkwait:
    cmp $0400
    bne chkwait
    
    ldx #0
chklp:    
    lda BUFFER,x
    cmp reference,x
    bne failed
    lda BUFFER+$100,x
    cmp reference+$100,x
    bne failed
    inx
    bne chklp

    lda #5
    sta $d020
    sta bgcol+1
    lda #$00
    sta $d7ff
    
	jmp	sa_lp2

failed:
    lda #10
    sta $d020
    sta bgcol+1
    lda #$ff
    sta $d7ff
    jmp sa_lp2
	

irq_server:
	lda	#1
	sta	$d019
	lda	#3
	sta	$d015
	lda	$d01e
	sta	$d020
	lda	spos
	sta	$d001
	sta	$d003
	ldx	#20
is_lp1:
	dex
	bne	is_lp1
	lda	#15
	sta	$d020
	ldx	#100
is_lp2:
	dex
	bne	is_lp2
	dec	$d020

	lda	spos+1
	and	#1
	ora	#>BUFFER
	sta	is_sm1+2
	ldx	spos
	lda	$d01e
is_sm1:
	sta	BUFFER,x

	sta	$d020
	ldx	#50
is_lp3:
	dex
	bne	is_lp3
	
bgcol:
	lda	#14
	sta	$d020

	lda	#0
	sta	$d015

	ldx	#0
	jsr	inc_ypos
	ldx	#2
	jsr	inc_ypos

	jsr	set_raster
	
	inc $0401
	bne skip
	inc $0400
skip:
	jmp	$febc

set_raster:
	lda	ypos
	sta	$d012
	lda	ypos+1
	lsr
	ror
	and	#$80
	sta	tmp_zp
	lda	$d011
	and	#$7f
	ora	tmp_zp
	sta	$d011
	rts
	
ypos:
	dc.w	128
spos:
	dc.w	134

inc_ypos:
	inc	ypos,x
	bne	iy_skp1
	inc	ypos+1,x
iy_skp1:
	lda	ypos+1,x
	beq	iy_ex1
	lda	ypos,x
	cmp	#<312
	bne	iy_ex1
	lda	#0
	sta	ypos,x
	sta	ypos+1,x
iy_ex1:
	rts

reference:
    incbin "dumps/dump8565.bin"
	
BUFFER	equ	$4000
	
; eof
