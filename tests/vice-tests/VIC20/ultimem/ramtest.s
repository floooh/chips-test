;;; @file ramtest.s
;;; RAM test for the UltiMem
;;; @author Marko Mäkelä (marko.makela@iki.fi)

;;; This file can be compiled with xa
;;; (Cross-Assembler 65xx V2.1.4h 12dec1998 (c) 1989-98 by A.Fachat)
;;; or xa (xa65) v2.3.5
;;; Written by Andre Fachat, Jolse Maginnis, David Weinehall and Cameron Kaiser

;;; Copyright © 2010,2016 Marko Mäkelä (marko.makela@iki.fi)
;;;
;;;	This program is free software; you can redistribute it and/or modify
;;;	it under the terms of the GNU General Public License as published by
;;;	the Free Software Foundation; either version 2 of the License, or
;;;	(at your option) any later version.
;;;
;;;	This program is distributed in the hope that it will be useful,
;;;	but WITHOUT ANY WARRANTY; without even the implied warranty of
;;;	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;;	GNU General Public License for more details.
;;;
;;;	You should have received a copy of the GNU General Public License along
;;;	with this program; if not, write to the Free Software Foundation, Inc.,
;;;	51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA.

#include "ultimem_regs.s"

	.word $1001
	*=$1001
prg	.word nextln
	.word 2016
	.byte $9e		; SYS
	.byte $30 + (start / 1000)
	.byte $30 + ((start - (start / 1000 * 1000)) / 100)
	.byte $30 + ((start - (start / 100 * 100)) / 10)
	.byte $30 + (start - (start / 10 * 10))
	.byte 0
nextln	.word 0

strout	= $cb1e			; output a NUL-terminated string
chrout	= $ffd2			; output a character
#define printmsg(x) lda#<x:ldy#>x:jsr strout
#define printmsg_rts(x) lda#<x:ldy#>x:jmp strout

crsrchr	= $d1			; cursor position in screen memory
crsr_x	= $d3			; current cursor column
crsrcol	= $f3			; cursor position in color memory
txtcolor= 646			; current text color

	;; 16-bit division
	;; .A = NUM % DIV
	;; .Y = 0
	;; NUM /= DIV
#define div16(NUM,DIV) ldy #17	; n_bits+1:\
	lda #0			; remainder:.(:\
	beq entry		; entry point (branch always):\
loop	rol:\
	cmp #127:\
	bcc entry		; remainder > divisor?:\
	sbc #127		; yes, subtract the divisor:\
entry				; shift the quotient:\
	rol NUM:\
	rol NUM+1:\
	dey:\
	bne loop:.)

	;; Generate a 16-bit random number using a binary Galois LFSR
	;; with the primitive polynomial
	;; x^16 + x^14 + x^13 + x^11 + 1
	;; (taps corresponding to $b400)
	;; from en.wikipedia.org/wiki/Linear_feedback_shift_register
	;; RNG = new random value (never 0)
	;; .A = trashed

#define rng16(RNG) .(:\
	lsr RNG+1:\
	ror RNG:\
	bcc done:\
	lda RNG+1:\
	eor #$b4:\
	sta RNG+1:\
done	.)

ram123	= $0400			; 3k RAM
blk1	= $2000			; 8k+8k+8k RAM
blk5	= $a000			; 8k RAM

cnt	= $ff			; loop counter
rng	= $fb			; random number generator
dst	= $fd

start	sei
	lda $9f55		; Re-enable the UltiMem registers
	lda $9faa		; if they were disabled previously.
	lda $9f01
	.(
	ldx #unit_id_end-unit_id-1
	lda ultimem_id
id	cmp unit_id,x
	beq id_ok
	dex
	bpl id
	cli
	printmsg_rts(ultimem_not_detected)
id_ok	lda #ultimem_cfg_led
	sta ultimem_cfg
	lda #0
	ldx #12
banks	sta ultimem_ram-1,x	; map address 0 to every address window
	dex
	bpl banks
	stx ultimem_ioram	; RAM at I/O3, I/O2, RAM123
	stx ultimem_blk		; RAM at BLK5, BLK3, BLK2, BLK1
	.)
	printmsg(initializing)
	.(
	;; detect the RAM size
	ldy $4000
	lda #1			; detected RAM size in 8KiB blocks
ramsz	sta ultimem_blk2
	cpy $2000
	bne moreram
	inc $4000
	cpy $2000
	bne ramtop
	sty $4000
moreram	asl
	bne ramsz
ramtop	sty $4000
	lsr
	lsr
	sbc #0			; RAM size in 32KiB blocks, minus 1
	sta ramsize
	sta cnt
	;; initialize 32k RAM banks
init32k	lda cnt
	sta eor1
	sta eor2
	asl
	asl
	tax
	stx ultimem_blk1
	inx
	stx ultimem_blk2
	inx
	stx ultimem_blk3
	inx
	stx ultimem_blk5
	;; copy a 127-byte array to the memory (32768 = 258 * 127 + 2)
	lda #>blk1
	ldy #0
	sty dst
	sta dst+1
	sta rng			; nonzero initial value for the pseudo rng
	sta rng+1
copy1	ldx #1
copy	lda pattern-1,x
eor1 = *+1
	eor #0
	sta (dst),y
	iny
	bne dstok
	inc dst+1
	bpl dstok
	bmi last8k
dstok	inx
	bpl copy
	bmi copy1
last8k	lda #>blk5
	sta dst+1
dstok2	inx
	bpl copy2
	ldx #1
copy2	lda pattern-1,x
eor2 = *+1
	eor #0
	sta (dst),y
	iny
	bne dstok2
	inc dst+1
	bit dst+1
	bvc dstok2		; until $c000 reached
	lda cnt
	jsr hex2
	lda #$9d
	jsr chrout
	jsr chrout
	dec cnt
	bpl init32k
	.)
	printmsg(testing)
	ldy #0
	.(
	sty ultimem_blk1	; BLK1 at 0
	sty ultimem_blk2	; BLK2 at 0
	sty ultimem_blk3	; BLK3 at 0
	sty ultimem_blk5	; BLK5 at 0

	lda #4			; compare RAM123 to BLK1235
	sta cmphi1
	lda #$24
	sta cmphi2
cmploop
cmphi1	= *+2
	lda $400,y
cmphi2	= *+2
	cmp $2400,y
	bne cmpfail
	iny
	bne cmploop
	inc cmphi1
	lda cmphi2
	sec
	adc #$20
	bpl hiok		; below $8000
	cmp #$a0
	eor #$20
	bcc hiok		; $a000..$bfff = BLK5
	eor #>(blk1^blk5)	; wrap back to BLK1
hiok	sta cmphi2
	lda cmphi1
	and #$f
	bne cmploop
	iny
	sty ultimem_blk2	; BLK2 at $2000
	iny
	sty ultimem_blk3	; BLK3 at $4000
	iny
	sty ultimem_blk5	; BLK5 at $6000
	bne ram123_ok
cmpfail	tya
	pha
	printmsg(ram123_fail)
	lda cmphi2
	jsr hex2
	pla
	.)
hex2_exit
	jsr hex2
	lda #$0d
	jsr chrout
	jmp exit

ram123_ok
	.(
	lda #$9f		; compare IO2 and IO3 to BLK1
	sta cmphi1
	lda #$3f
	sta cmphi2
	ldy #$ef
	ldx #8
cmploop
cmphi1	= *+2
	lda $9f00,y
cmphi2	= *+2
	cmp $3f00,y
	bne cmpfail
	dey
	bne cmploop
	dec cmphi1
	dec cmphi2
	dex
	bne cmploop
	beq blkcmp
cmpfail	tya
	pha
	printmsg(io_fail)
	lda cmphi2
	jsr hex2
	pla
	.)
	jmp hex2_exit
;;; ...
	;; verify BLK1, BLK2, BLK3, BLK5 at random addresses
blkcmp	ldx #0
	stx cnt			; 256*256 rounds
	txa
	and ramsize
	sta eor3
	asl
	asl
	tax
	stx ultimem_blk1
	inx
	stx ultimem_blk2
	inx
	stx ultimem_blk3
	inx
	stx ultimem_blk5
	ldx #0

blk_l	rng16(rng)
	lda rng+1		; load 15 bits to the address
	and #$7f
	sta dst+1
	pha
	lda rng
	sta dst
	pha
	div16(dst,127)
	tay			; address % 127
	pla
	sta dst
	pla
	clc
	adc #>blk1
	bpl addr_ok
	eor #>(blk5-$8000)
addr_ok	sta dst+1
	lda pattern,y
	ldy #0
eor3 = *+1
	eor #0
	eor (dst),y
	bne ram_fail
	dex
	bne blk_l
	lda cnt			; update the on-screen progress display
	and #15
	tay
	lda bar,y
	ldy crsr_x
	sta (crsrchr),y
	dec cnt
	bne blk_l
	printmsg(ram_ok)
exit	lda #0
	sta ultimem_cfg
	sta ultimem_ioram
	sta ultimem_blk
	cli
	rts
ram_fail
	printmsg(blk_fail)
	lda eor3
	jsr hex2
	lda #$2c
	jsr chrout
	lda dst+1
	jsr hex2
	lda dst
	jmp hex2_exit

	;; print two hexadecimal digits in .A
hex2
	tax
	lsr
	lsr
	lsr
	lsr
	jsr hex1
	txa
	;; print one hexadecimal digit in .A
hex1
	and #$f
	ora #$30
	cmp #$3a
	bcc doprint
	adc #$41-$3a-1
doprint
	jmp chrout

;; 127 bytes of test data
pattern	.byte $01,$02,$03,$04,$05,$06,$07,$08,$09,$0a,$0b,$0c,$0d,$0e,$0f,$10
	.byte $aa,$55,$cc,$33,$11,$22,$44,$88,$33,$66,$cc,$99,$00,$ff,$c3,$3c
	.byte $03,$1c,$2a,$35,$40,$5f,$61,$7e,$82,$9d,$a3,$bc,$c4,$db,$e5,$fa
	.byte $10,$20,$30,$40,$50,$60,$70,$80,$90,$a0,$b0,$c0,$d0,$e0,$f0,$01
	.byte $30,$c1,$a2,$53,$04,$f5,$16,$e7,$28,$d9,$3a,$cb,$4c,$bd,$5e,$af
	.byte $c0,$ff,$ee,$de,$ad,$be,$ef,$c0,$de,$b1,$de,$37,$73,$f0,$0f,$09
	.byte $b1,$40,$23,$a2,$85,$74,$97,$66,$a9,$38,$db,$4c,$cd,$35,$df,$2e
	.byte $ef,$df,$cf,$bf,$af,$9f,$8f,$7f,$6f,$5f,$4f,$3f,$2f,$1f,$0f
ultimem_not_detected
	.byte "ULTIMEM NOT DETECTED", 0
initializing
	.byte "INITIALIZING RAM...", 0
testing	.byte "OK", $0d, "TESTING... ", $9d, 0
ram123_fail
	.byte $0d, "RAM<>BLK AT ", 0
io_fail
	.byte $0d, "IO<>BLK AT ", 0
blk_fail
	.byte $0d, "BLK MISMATCH: ", 0
ram_ok
	.byte $0d, "MEMORY OK", 0

	;; UltiMem unit type identifier
unit_id	.byte ultimem_id_512k, ultimem_id_8m
unit_id_end
;; progress bar
bar
	.byte $20,$65,$74,$75,$61,$f6,$ea,$e7,$a0,$e5,$f4,$f5,$76,$6a,$67,$20
ramsize
