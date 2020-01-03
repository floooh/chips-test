;**************************************************************************
;*
;* FILE  via_wrap.asm 
;* Copyright (c) 2009 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;*
;* DESCRIPTION
;*   Test case for via timer wrap around.
;*   The stabilize routine is not correct for NTSC, needs to be corrected.
;*
;******
	processor 6502
	include	"macros.i"

SCR_BASE	equ	$1e00
COL_BASE	equ	$9600
	
	if	IS_PAL
;**************************************************************************
;*
;* Constants PAL
;*
;******
;the length of a row in cycles
LINETIME	equ	71
;the number of raster lines
NUMLINES	equ	312
;the length of a screen in cycles
SCREENTIME	equ	LINETIME*NUMLINES-2

FIRST_LINE	equ	$26-4

	else
;**************************************************************************
;*
;* Constants NTSC
;*
;******
;the length of a row in cycles
LINETIME	equ	65
;the number of raster lines
NUMLINES	equ	261
;the length of a screen in cycles
SCREENTIME	equ	LINETIME*NUMLINES-2

FIRST_LINE	equ	$19-4
	endif

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
	sei

	ldx	#0
	lda	#$20
soc_lp1:
	sta	SCR_BASE+22*8,x
	inx
	bne	soc_lp1
	
	lda	#$7f
	sta	$912e
	sta	$912d
	lda	#%11000000
	sta	$912e
	lda	#%01000000
	sta	$912b

	jsr	stab
	lda	#<SCREENTIME
	sta	$9124
	lda	#>SCREENTIME
	sta	$9125	;load T1

soc_lp2:
	jsr	stab
	ldx	$9124
	lda	$900f
	tay
	eor	#$f7
	sta	$900f
	inc	SCR_BASE+22*8,x
	lda	646
	sta	COL_BASE+22*8,x
	jsr	st_twelve
	jsr	st_twelve
	sty	$900f
	jmp	soc_lp2

;**************************************************************************
;*
;* Stabilize routine
;*
;******
stab:

st_lp4
	lda	$9004
	bne	st_lp4
st_lp5:
	lda	$9004
	beq	st_lp5

	START_SAMEPAGE	"Stab"
	ldy	#64
st_lp1:
	lda	$9003
	bmi	st_skp1
st_skp1:
	bit	$ea
	ldx	#12
st_lp2:
	dex
	bne	st_lp2

	lda	$9003
	bpl	st_skp2
st_skp2:
	bit	$ea
	ldx	#11
st_lp3:
	dex
	bne	st_lp3

	dey
	bne	st_lp1

	nop
st_lp6:
	lda	$9004
	beq	st_ex1
	bit	$ea
	ldx	#11
st_lp7:
	dex
	bne	st_lp7
	bit	$ea
	jmp	st_lp6

st_ex1:
	END_SAMEPAGE

	START_SAMEPAGE "stab2"
	lda	#FIRST_LINE
st_lp8:
	jsr	st_twelve
	jsr	st_twelve
	jsr	st_twelve
	jsr	st_twelve
	jsr	st_twelve
	nop
	nop
	cmp	$9004		; 4
	bne	st_lp8		; 3
	END_SAMEPAGE
st_twelve:	
	rts	

; eof

	
