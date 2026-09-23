;**************************************************************************
;*
;* FILE  modesplit.asm
;* Copyright (c) 2010 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;*
;* DESCRIPTION
;*
;******

	processor 6502

; videobank to use. original program (R03) uses bank 0 ($0000-$3fff), which has
; some drawbacks (zeropage, stack, program etc in bitmap 0)
;
; we use bank 2 ($8000-$bfff) instead, since we can produce the same setup (with
; the chargen getting "In the way"), but without the annyoing drawbacks.
USEBANK = 2

; if (USEBANK == 0)
; ; bank 0 ($0000-$3fff)
; vbank equ $0000
; bitmap equ $0000
; vram equ $0400
; charset equ $1800
; ;bitmap equ $2000
; endif
;
; if USEBANK = 1
; ; bank 1 ($4000-$7fff)
; vbank equ $4000
; bitmap equ $4000
; vram equ $4400
; charset equ $5800
; endif

;if USEBANK = 2
; bank 2 ($8000-$bfff)
vbank equ $8000
bitmap equ $8000
vram equ $8400
charset equ $9800
;endif

; ;if USEBANK = 3
; ; bank 2 ($c000-$ffff)
; vbank equ $c000
; bitmap equ $c000
; vram equ $c400
; charset equ $d800
; ;endif


SCREENSHOTEXIT  equ 1
	
LINE		equ	56
; TEST_NAME	eqm	"MODESPLIT"
; TEST_REVISION	eqm	"R04"
; LABEL_LOWERCASE	equ	1
TEST_NAME	eqm	"modesplit"
TEST_REVISION	eqm	"r04"
LABEL_LOWERCASE	equ	1
LABEL_SCRADDR	equ	vram

	seg.u	zp
;**************************************************************************
;*
;* SECTION  zero page
;*
;******
	org	$02
ptr_zp:
	ds.w	1
tm1_zp:
	ds.b	1
tm2_zp:
	ds.b	1
guard_zp:
	ds.b	2

;**************************************************************************
;*
;* common startup and raster code
;*
;******
HAVE_STABILITY_GUARD	equ	1
HAVE_ADJUST		equ	1
	include	"../common/startup.asm"

	include	"../common/onescreen.asm"

;**************************************************************************
;*
;* NAME  test_present
;*
;******
test_present:
	jsr	show_label_bar
	rts


;**************************************************************************
;*
;* NAME  test_prepare
;*
;******
test_prepare:

    php
	sei
	lda $01
	pha

; clear entire video bank
    lda #%00011011
	ldy #0
bank_lp2:
	ldx #0
bank_lp1:
	sta vbank,x
	inx
	bne	bank_lp1
	inc bank_lp1+2 ; inc hi
	iny
	cpy #$40
	bne	bank_lp2

; clear bitmap area with a different pattern
	lda #%00111100
	ldy #0
bm_lp2:
	ldx #0
bm_lp1:
	sta bitmap,x
	inx
	bne	bm_lp1
	inc bm_lp1+2 ; inc hi
	iny
	cpy #$20
	bne	bm_lp2

; set up screen
;     lda #>vram
;     sta $0288
; 	jsr	show_label_bar

	ldx	#0
vrm_lp1:
	lda	$0400,x
	sta	vram,x
	inx
	cpx #40
	bne vrm_lp1


	ldx	#0
prt_lp1:
	lda	#$5f       ; original program clears vram with $5f
;	lda	#$01
	sta	vram+$0028,x
	sta	vram+$0100,x
	sta	vram+$0200,x
	sta	vram+$02e8,x
	lda	#$0e
	sta	$d828,x
	sta	$d900,x
	sta	$da00,x
	sta	$dae8,x
	inx
	bne	prt_lp1

; copy data from char rom (only for banks that dont have one)
	if 0

	lda #$33
	sta $01
	ldy #0
bm_lp4:
	ldx #0
bm_lp3:
	lda $d000,x
	sta charset,x
	inx
	bne	bm_lp3
	inc bm_lp3+2 ; inc hi
	inc bm_lp3+3+2 ; inc hi
	iny
	cpy #$02
	bne	bm_lp4

	ldx #0
bm_lp5:
;	lda $d800+(8*$5f),x
	lda $daf8,x
	sta charset+$2f8,x
	inx
	cpx #8
	bne	bm_lp5

	endif

	pla
	sta $01

	;cli
	plp


	jsr	adjust_timing

; 	lda	#$18   ; vram at $0400|charset at $2000/bitmap at $2000
	lda	#$17   ; vram at $0400|charset at $1800/bitmap at $0000 (original value)
; 	lda	#$16   ; vram at $0400|charset at $1800/bitmap at $0000
; 	lda	#$15   ; vram at $0400|charset at $1000/bitmap at $0000
; 	lda	#$14   ; vram at $0400|charset at $1000/bitmap at $0000
; 	lda	#$13   ; vram at $0400|charset at $0800/bitmap at $0000
; 	lda	#$12   ; vram at $0400|charset at $0800/bitmap at $0000
; 	lda	#$11   ; vram at $0400|charset at $0000/bitmap at $0000
; 	lda	#$10   ; vram at $0400|charset at $0000/bitmap at $0000
	sta	$d018
	lda	#$03 - USEBANK
	sta	$dd00

	lda	#$1b | (>LINE << 7)
	sta	$d011
	lda	#<LINE
	sta	$d012

	rts

	
;******
; One 8 char high chunk
	mac	CHUNK
	ldy	#$08
	bne	.+4
.lp1:
	ds.b	9,$ea
	sty	$d016
	lda	#7
	sta	$d021
	lda	#6
	sta	$d021
	ds.b	7,$ea
	EOL

	jsr	{1}

	ldx	#$1b
	stx	$d011
	sty	$d016
	ds.b	1,$ea
	EOL

	iny
	cpy	#$10
	bne	.lp1
	endm
	
	align	256
test_start:
;**************************************************************************
;*
;* NAME  test_perform
;*
;******
test_perform:
	lda	$dc06
	sta	guard_zp+0
	bit	$ea
	ds.b	1,$ea
; start 1
	CHUNK 	section1
; start 2
	CHUNK 	section2
; start 3
	CHUNK 	section3
; end
	bit	$ea
	ds.b	4,$ea	
	lda	#$1b
	sta	$d011
	lda	#$08
	sta	$d016
	lda	#7
	sta	$d021
	lda	#6
	sta	$d021
	ds.b	7,$ea
	EOL
	lda	$dc06
	sta	guard_zp+1


	ldx	#1
tp_lp1:
	ldy	guard_zp,x
	jsr	update_guard
	dex
	bpl	tp_lp1

	jsr	show_guards
	rts

	align	256


;**************************************************************************
;*
;* NAME  section1
;*
;******
section1:
	repeat	6
	EOL
	ds.b	2,$ea
	ldx	#$1b
	stx	$d011       ; text
	sty	$d016
	tya
	ora	#%00010000
	sta	$d016		; mc text
	ldx	#$5b		; illegal text
	stx	$d011
	ldx	#$3b		; mc bitmap
	stx	$d011
	and	#%11101111
	sta	$d016		; hires bitmap
	ldx	#$7b
	stx	$d011		; illegal bitmap1
	ldx	#$5b
	stx	$d011		; ECM
	ds.b	3,$ea
	bit	$ea
	repend
	rts

;**************************************************************************
;*
;* NAME  section2
;*
;******
section2:
	repeat	6
	EOL
	ds.b	2,$ea
	ldx	#$1b
	stx	$d011
	sty	$d016

	ldx	#$5b
	stx	$d011
	ldx	#$1b
	stx	$d011
	ds.b	16,$ea
	bit	$ea
	repend
	rts

;**************************************************************************
;*
;* NAME  section3
;*
;******
section3:
	repeat	6
	EOL
	ds.b	2,$ea
	ldx	#$1b
	stx	$d011
	sty	$d016
	tya
	ora	#%00010000
	sta	$d016
	and	#%11101111
	sta	$d016
	ds.b	15,$ea
	bit	$ea
	repend
	rts

test_end:

; eof
