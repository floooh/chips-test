;**************************************************************************
;*
;* FILE  lplatency.asm
;* Copyright (c) 2010 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;*
;* DESCRIPTION
;*   Verify latency of CIA triggered light pen IRQ.
;*
;******
	processor 6502


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
	lda	#<greet_msg
	ldy	#>greet_msg
	jsr	$ab1e

	sei

	lda	#%11111111
	sta	$dc00
	lda	#%00000000
	sta	$dc02
	lda	#%11111111
	sta	$dc01
	sta	$dc03
	lda	#$0f
	sta	$d019		; clear interrupts

	ldx	#$15
	stx	$d018
	lda	#$01
	sta	$00,x
	lda	#$dc
	sta	$01,x

	lda	#$f2
	sta	$f0
	lda	#$00
	sta	$f1
	lda	#"N" & $3f
	sta	$f2
	lda	#"Y" & $3f
	sta	$60f0

	lda	#$91		; STA (<zp>),Y
	sta	$d017
	lda	#$00
	sta	$d01a
	lda	#$60
	sta	$d01b
	sta	$d01c
	
sa_lp1:
	lda	#$0f
	sta	$d019
	jsr	wait_vb
	jsr	wait_vb
;make sure raster compare has triggered once	
sa_lp2:
	bit	$d011
	bpl	sa_lp2
; in bottom border
	inc	$0420
	ldy	#%00000000
; get $d019 without trigger
	tya
	clc
	jsr	$d019
	tax
; trigger and get $d019
	tya
	clc
	jsr	$d017
; d017  91 15     sta  ($15),y	; -> $dc01
; #1:
; d019  71 f0     adc  ($f0),y
; d01b  60        rts
; d01c  60        rts
; #2:
; d019  79 f0 60  adc  $60f0,y
; d01c  60        rts

	ldy	#%11111111
	sty	$dc01
	dec	$0420
	
	ldy	$d3
	dey
	sta	($d1),y
	txa
	dey
	dey
	sta	($d1),y
	
	ldx     #10
    lda     $0400+40*7+8
    cmp     $0400+40*8+8
    bne     err
    lda     $0400+40*7+10
    cmp     $0400+40*8+10
    bne     err
	ldx     #5
err:
    stx     $d020
    
    lda #0
    cpx #5
    beq passed
    lda #$ff
passed:
    sta $d7ff

	jmp	sa_lp1


greet_msg:
	dc.b	147,"LP LATENCY R01 / TLR",13,13
	dc.b	"DETERMINE LATENCY OF $D019 LP IRQ",13,13
	dc.b	"1ST: IRQ BEFORE TRIGGER (Y/N)",13
	dc.b	"2ND: IRQ CYCLE AFTER TRIGGER (Y/N)",13,13
	dc.b	"EXPECT: N Y",13
	dc.b	"RESULT: . .",0

	
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

; eof
