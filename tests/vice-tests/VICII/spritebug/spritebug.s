
#define PAGE	.dsb	$ff - ((* - 1) & $ff), $00

BORDER		=	$d020
SPLITREG    =   $d01d
;SPLITREG    =   $d021

time		=	$02

		.word	basicstub
		*=$801
basicstub
		.(
		.word	end, 1
		.byt	$9e,"2061",0
end		.word	0
		.)
entry
		sei
		lda	#$35
		sta	1
		
		lda #6
		sta $d021
		lda #14
		sta $d020

		lda	#$7f
		sta	$dc0d
		sta	$dd0d

		.(
		ldx	#$3f
loop
		lda	sprdef,x
		sta	$f00,x
		dex
		bpl	loop
		.)

		.(
		ldx	#$00
loop
		lda #$20
		sta	$0400,x
		sta	$0500,x
		sta	$0600,x
		sta	$0700,x
		lda #$01
		sta	$d800,x
		sta	$d900,x
		sta	$da00,x
		sta	$db00,x
		inx
		bne	loop
		.)
		
		lda #$65
		sta $0400+(1*40)+15
		sta $0400+(2*40)+15
		sta $0400+(3*40)+15

		lda	#COUNTER
		sta	time

		lda	#$44
		sta	$d012

		lda	#$3c
		sta	$dd02

		lda	#$04
		sta	$d015
		lda	#$1b
		sta	$d011
		lda	#$08
		sta	$d016

		lda	#$44
		sta	$d005
		lda	#0
		sta	$d010

		lda	#$3c
		sta	$7fa

		ldx	$d012
		inx
resync
		cpx	$d012
		bne	*-3
		; at cycle 4 or later
		ldy	#0		; 4
		sty	$dc07		; 6
		lda	#8		; 10
		sta	$dc06		; 12
		iny			; 16
		sty	$d01a		; 18
		dey			; 22
		dey			; 24
		lda	#$11		; 26
		sta	$dc0f		; 28
		sty	$dc02		; 32
		cmp	(0,x)		; 36
		cmp	(0,x)		; 42
		cmp	(0,x)		; 48
		txa			; 54
		inx			; 56
		inx			; 58
		cmp	$d012		; 60	still on the same line?
		bne	resync

		lda	#0
		sta	$dd00

		lda	#<interrupt
		sta	$fffe
		lda	#>interrupt
		sta	$ffff
		lsr	$d019
		cli
main
		jmp	main

sprdef
		.byt	$55,$55,$55
		.byt	$55,$55,$55
		.byt	$aa,$aa,$aa
		.byt	$aa,$aa,$aa
		.byt	$ff,$ff,$ff
		.byt	$ff,$ff,$ff
		.byt	$00,$00,$00
		.byt	$00,$00,$00
		.dsb	sprdef+$40-*, 0

		PAGE
interrupt
		; nominal sync code with no sprites:
		sta	int_savea+1	; 10..16
		lda	$dc06		; in the range 1..7
		eor	#7
		sta	*+4
		bpl	*+2
		lda	#$a9
		lda	#$a9
		lda	$eaa5

		; at cycle 35

		dec	BORDER		; 35, $44

		stx	int_savex+1	; 41
		sty	int_savey+1	; 45

		.(
		ldy	#8		; 49
loop
		lda	#$00		; 51
		sta	$d017		; 53
		lda	#$00		; 57
		sta	$d01c		; 1
		lda	#$04		; 5
		sta	SPLITREG		; 7

		lda	#$00		; 11
		jsr	delay16		; 13
		sta	SPLITREG		; 29

		lda	0		; 33
		nop			; 36
		nop			; 38
		nop			; 40
		nop			; 42
		nop			; 44

		dey			; 46
		bne	loop		; 48
		.)

#if (COUNTER = 0)
		inc	time
#endif
		lda	time
		sta	$d004

		inc	BORDER

		lda	time
		pha
		and #$0f
		tax
		lda hextab,x
		sta $0401
		pla
		lsr
		lsr
		lsr
		lsr
		tax
		lda hextab,x
		sta $0400
		
		dec framecount
		bne skp
		lda #0
		sta $d7ff
skp:
		
int_savea	lda	#0
int_savex	ldx	#0
int_savey	ldy	#0
		lsr	$d019
		rti

delay16		nop
delay14		nop
delay12		rts

hextab: .byt "0123456789",1,2,3,4,5,6

framecount: .byt 5
