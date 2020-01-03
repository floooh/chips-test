;;; @file banktest.s
;;; Banking register test for the Vic UltiMem
;;; @author Marko Mäkelä (marko.makela@iki.fi)

;;; This file can be compiled with xa
;;; (Cross-Assembler 65xx V2.1.4h 12dec1998 (c) 1989-98 by A.Fachat)
;;; or xa (xa65) v2.3.5
;;; Written by Andre Fachat, Jolse Maginnis, David Weinehall and Cameron Kaiser

;;; Copyright © 2016 Marko Mäkelä (marko.makela@iki.fi)
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

ramsize	= $fb			; RAM size in 8KiB blocks
cnt	= $fc			; counter

blk1	= $3fff			; test address in BLK1
blk2	= $5fff			; test address in BLK2
blk3	= $7fff			; test address in BLK3
blk5	= $bfff			; test address in BLK5

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
id_ok	lda unit_ramsize,x
	sta ramsize
	lda #ultimem_cfg_led
	sta ultimem_cfg
	lda #0
	ldx #12
banks	sta ultimem_ram-1,x	; map address 0 to every address window
	dex
	bpl banks
	sta cnt
	stx ultimem_ioram	; RAM at I/O3, I/O2, RAM123
	stx ultimem_blk		; RAM at BLK5, BLK3, BLK2, BLK1
	.)
	.(
	ldx ramsize
init	stx ultimem_blk1
	stx blk1
	dex
	bpl init
	.)
	printmsg(testing)
	.(
cmploop	lda cnt
	jsr hex2
	lda #$9d
	jsr chrout
	jsr chrout
	ldx ramsize
compare	stx ultimem_blk1
	stx ultimem_blk2
	stx ultimem_blk3
	stx ultimem_blk5
	cpx blk1
	bne cmp1_fail
	cpx blk2
	bne cmp2_fail
	cpx blk3
	bne cmp3_fail
	cpx blk5
	bne cmp5_fail
	dex
	bpl compare
	inc cnt
	bne cmploop
	.)
	cli
	printmsg_rts(banking_ok)
cmp1_fail
	lda #$31
	.byte $2c
cmp2_fail
	lda #$32
	.byte $2c
cmp3_fail
	lda #$33
	.byte $2c
cmp5_fail
	lda #$35
	sta blk_number
	txa
	pha
	printmsg(banking_fail)
	pla
	cli
	;; print two hexadecimal digits in .A
hex2	tax
	lsr
	lsr
	lsr
	lsr
	jsr hex1
	txa
	;; print one hexadecimal digit in .A
hex1	and #$f
	ora #$30
	cmp #$3a
	bcc doprint
	adc #$41-$3a-1
doprint	jmp chrout

ultimem_not_detected
	.byte "ULTIMEM NOT DETECTED", 0
testing	.byte "TESTING...", 0
banking_fail
blk_number = banking_fail+4
	.byte $0d, "BLK1 MISMATCH AT ", 0
banking_ok
	.byte "OK", 0

	;; UltiMem unit type identifier
unit_id	.byte ultimem_id_512k, ultimem_id_8m
unit_id_end
	;; UltiMem RAM size (in multiples of 8k), minus 1
unit_ramsize
	.byte 63, 127
