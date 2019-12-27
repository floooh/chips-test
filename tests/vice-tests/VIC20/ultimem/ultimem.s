;;; @file ultimem.s
;;; Vic UltiMem instructions and RAM configuration selector
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

WIDTH	= 21			; screen width

KEY_F1	= $85
KEY_F3	= $86
KEY_F5	= $87
KEY_F7	= $88

temp	= $fb			; temporary variable

ram_id	= $9f00			; RAM configuration ID
ram_cur	= $9f01			; currently selected RAM configuration
ram_blk	= $9f02			; enabled RAM banks (25 slots)
BLK1_EN		= $01		; ram_blk[] flag to enable BLK1
BLK2_EN		= $02		; ram_blk[] flag to enable BLK2
BLK3_EN		= $04		; ram_blk[] flag to enable BLK3
RAM123_EN	= $08		; ram_blk[] flag to enable RAM1 to RAM3
BLK5_EN		= $10		; ram_blk[] flag to enable BLK5
BLK_EN		= $17		; ram_blk[] flags to enable BLK[1235]
IO2_EN		= $20		; ram_blk[] flags to enable I/O2
IO3_EN		= $40		; ram_blk[] flags to enable I/O3
IO_EN		= IO2_EN|IO3_EN	; ram_blk[] falgs to enable I/O2 and I/O3
ram_crc	= ram_blk+25		; CRC-8 checksum
crc8	= $e7			; X^8 + X^7 + X^6 + X^5 + X^2 + X + 1

strout	= $cb1e			; output a NUL-terminated string
chrout	= $ffd2			; output a character
getin	= $ffe4			; read a key
plot	= $fff0			; set screen position
#define printmsg(x) lda#<x:ldy#>x:jsr strout
#define printmsg_rts(x) lda#<x:ldy#>x:jmp strout

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
	sta ultimem		; light up the LED
	lda #0
	ldx #15
reginit	sta ultimem,x		; initialize the UltiMem registers
	dex
	bne reginit
	lda #$f
	sta ultimem_blk
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
moreram	inx
	asl
	bne ramsz
ramtop	sty $4000
	lda ramsize,x
	sta config1
	ldy #ultimem_ioram_0ki
	sty ultimem_ioram	; RAM at I/O3; no memory at I/O2, RAM123
	ldy #$ff
	sty ultimem_io		; I/O3 at top of RAM
	sty ultimem_io+1
	iny
	sty ultimem_blk		; no memory at BLK5, BLK3, BLK2, BLK1
	sty ultimem_blk2
	cpx ram_id
	bne ram_init
	sec
	sbc #"A"
	cmp ram_cur
	bcc ram_init
	jsr crc
	cmp ram_crc
	beq ram_ok
	ldx ram_id
ram_init
	stx ram_id
	tya
	ldx #ram_crc-ram_id-1
clr	dex
	sta ram_id+1,x
	bne clr
	ldx #presets_end-presets
preset	lda presets-1,x
	dex
	sta ram_blk,x
	bne preset
ram_ok
	.)
	cli
	lda #8
	sta $900f
	printmsg(initmsg)
loop	jsr getin
main	cmp #KEY_F1		; f1 => usage instructions 1
	bne not_f1
	printmsg(usage1)
	printmsg(usage1b)
	jmp loop
not_f1	cmp #KEY_F3		; f3 => usage instructions 2
	bne not_f3
	printmsg(usage2)
	printmsg(usage2b)
	jmp loop
not_f3	cmp #KEY_F5		; f5 => memory configuration
	beq ramcfg
	cmp #KEY_F7		; f7 => exit
	bne loop
	jsr crc
	sta ram_crc
ioram = *+1
	lda #0
	sta ultimem_ioram
	lda ultimem_ram
 	sta ultimem_io		; I/O2, I/O3 use the same 8K bank as RAM123
	lda #ultimem_cfg_dis|ultimem_cfg_noled
	sta ultimem		; disable the UltiMem
	rts
	;; memory configuration
ramcfg	printmsg(config)
	ldy #3
	jsr empty
	;; display the currently selected configuration
display	ldy ram_cur
	tya
	clc
	adc #"A"
	sta bankcur
	lda ram_blk,y
	sta temp
	ldx #10
	clc
	.(
cfgloop	txa
	pha
	ldy #13
	jsr plot
	lda#<bank_on:ldy#>bank_on
	lsr temp
	bcs cfg_on
	lda#<bankoff
cfg_on	jsr strout
	pla
	tax
	inx
	cpx #17
	bcc cfgloop
	.)
	ldy #17
	clc
	jsr plot
	printmsg(banksel)
	;; implement the currently selected configuration
	.(
	ldy ram_cur
	lda ram_blk,y
	lsr
	lsr
	lsr
	and #(RAM123_EN|IO2_EN|IO3_EN)/8
	tax
	lda ioram_cfg,x
	sta ioram
	lda ram_blk,y
	and #BLK_EN
	cmp #BLK5_EN
	bcc noadj
	eor #BLK5_EN|RAM123_EN		; move the bit
noadj	sta temp
	ldx #4
shift	lsr temp
	php
	ror
	plp
	ror
	dex
	bne shift
	.)
	sta ultimem_blk
	;; set up the address registers
	tya			; multiply by 5
	asl
	asl
	sty temp
	adc temp     		; C=0
	sta ultimem_ram
	ldy #0
	tax
	.(
bankadr	inx
	txa
	sta ultimem_blk1,y
	iny
	iny
	cpy #8
	bcc bankadr
	.)
	;; allow configuration changes with the keyboard
	.(
cfgloop	jsr getin
	beq cfgloop
	cmp #"1"
	bcc nocfg
	cmp #"8"
	bcc flip
	cmp #"A"
	bcc nocfg
	cmp config1
	bcc setbank
	beq setbank
nocfg	cmp #KEY_F1
	bcc cfgloop		; invalid key (reread)
	cmp #KEY_F7+1
	bcs cfgloop		; invalid key
	jmp main		; f1 to f7 => go to main menu
flip	sbc #"0"		; C=0 before, C=1 after
	tax
	lda pow2,x
	ldx ram_cur
	eor ram_blk,x
	sta ram_blk,x
jump	jmp display
setbank	sta bankcur
	sec
	sbc #"A"
	sta ram_cur
	bcs jump		; branch always (C=1)
	.)

	;; display Y empty lines
empty	lda #$20
	.(
	ldx #21
spaces	jsr chrout
	dex
	bne spaces
	.)
	lda #CR
	jsr chrout
	dey
	bne empty
	rts

	;; compute a CRC-8 checksum of ram_id,ram_cur,ram_blk
	;; A=CRC-8, Y=0, X=trashed
crc	lda #0
	tax
	.(
bytes	eor ram_id,x
	ldy #8
bits	asl
	bcc noadd
	eor #crc8
noadd	dey
	bne bits
	inx
	cpx #ram_crc-ram_id
	bcc bytes
	.)
	rts

CLR	= $93
#define INIT	$13,$11,$11,$11,$11,$11,$11,$11,$11,$11
#define UPPER	$8,$8e
WHITE	= $5
GREEN	= $1e
BLUE	= $1f
YELLOW	= $9e
RVS_ON	= $12
RVS_OFF	= $92
CR	= $0d
#define rvs(x) RVS_ON,x,RVS_OFF
#define rvs2(x,y) RVS_ON,x,y,RVS_OFF
#define rvs3(x,y,z) RVS_ON,x,y,z,RVS_OFF

initmsg	.byte CLR,UPPER,WHITE
	.byte rvs($20)
	.byte $20,rvs3($20,$a1,$b6)
	.byte $aa,rvs3($20,$20,$a1)
	.byte $a1,RVS_ON,$20,$20,$20,$df,$a1,$20,RVS_OFF,$a1
	.byte RVS_ON,$20,$20,$20,$df,CR

	.byte BLUE,rvs($20)
	.byte $20,rvs3($20,$a1,$b6)
	.byte $20,$a2,$20,$ac,$bb,rvs($20)
	.byte $ac,$bb,rvs($20)
	.byte $ac,$a2,$bb,rvs($20)
	.byte $ac,$bb,RVS_ON,$20,CR

	.byte WHITE,rvs($20)
	.byte $20,rvs3($20,$a1,$b6)
	.byte $20,rvs($20)
	.byte $20,rvs($a1)
	.byte $a1,rvs2($20,$a1)
	.byte $a1,rvs3($20,$a1,$ac)
	.byte $be,rvs2($20,$a1)
	.byte $a1,RVS_ON,$20,CR

	.byte BLUE,$df,RVS_ON,$20,$20,$a1,$20,$a7,RVS_OFF,$df,rvs2($20,$a1)
	.byte $a1,rvs2($20,$a1)
	.byte $a1,rvs3($20,$a1,$20)
	.byte $a1,rvs2($20,$a1)
	.byte $a1,RVS_ON,$20,CR

	.byte GREEN, "GO4RETRO.COM/PRODUCTS", CR

	.byte WHITE,RVS_ON,"F1",RVS_OFF," INSTRUCTIONS",CR
	.byte RVS_ON,"F3",RVS_OFF," MORE INSTRUCTIONS",CR
	.byte RVS_ON,"F5",RVS_OFF," CONFIGURE MEMORY",CR
	.byte RVS_ON,"F7",RVS_OFF," EXIT",CR

	.byte 0

usage1	.byte INIT, GREEN, "USING THE MENU      ", CR
	.byte YELLOW,RVS_ON,"STOP",RVS_OFF,WHITE,","
	.byte YELLOW,RVS_ON,"3",RVS_OFF,WHITE,",",
	.byte YELLOW,RVS_ON,"8",RVS_OFF,WHITE," GO TO BASIC ",CR
	.byte YELLOW,RVS_ON,"RETURN",RVS_OFF,WHITE,"   SELECT ITEM ",CR
	.byte YELLOW,RVS_ON,"SHIFT",RVS_OFF,WHITE,"    HIDE ULTIMEM",CR
	.byte "(WITH THE ABOVE KEYS)",CR
	.byte "                     ",CR,0
usage1b	.byte GREEN, "NAVIGATING THE MENU: ", CR
	.byte YELLOW,RVS_ON,"CRSR",RVS_OFF,WHITE,"  MOVE LINE/PAGE ",CR
	.byte YELLOW,RVS_ON,"HOME",RVS_OFF,WHITE,"  GO TO TOP     ",CR
	.byte YELLOW,RVS_ON,"A",RVS_OFF,WHITE,"-"
	.byte YELLOW,RVS_ON,"Z",RVS_OFF,WHITE
	.byte "   SEARCH FOR     ",CR
	.byte "      INITIAL LETTER",CR
	.byte YELLOW,RVS_ON,"SHIFT",RVS_OFF,WHITE," TO CHANGE DIR. "
	.byte 0
usage2	.byte INIT, GREEN, "MENU STRUCTURE      ", WHITE, CR
	.byte "ENTRIES ARE FILES    ", CR
	.byte "OR ", YELLOW, "DIRECTORIES", WHITE, ".      ",CR
	.byte YELLOW,"_",WHITE, " GOES UP ONE LEVEL. ", CR
	.byte "CURRENT SELECTION IS ", CR
	.byte "IN THE CENTER,       ", CR
	.byte "HIGHLIGHTED IN ", BLUE, RVS_ON, "BLUE", RVS_OFF, WHITE, ". ", CR
	.byte 0
usage2b	.byte "ROM IMAGES ARE ALIGN-", CR
	.byte "ED AT 8K BOUNDARIES  ", CR
	.byte "FOR INSTANT STARTING.", CR
	.byte "PROGRAMS ARE STORED  ", CR
	.byte "IN UNCOMPRESSED FORM."
	.byte 0
config	.byte INIT, GREEN, "MEMORY CONFIGURATION", WHITE, CR
	.byte RVS_ON," 1 ",RVS_OFF," BLK1 RAM ",CR
	.byte RVS_ON," 2 ",RVS_OFF," BLK2 RAM ",CR
	.byte RVS_ON," 3 ",RVS_OFF," BLK3 RAM ",CR
	.byte RVS_ON," 4 ",RVS_OFF," 3 KB RAM ",CR
	.byte RVS_ON," 5 ",RVS_OFF," BLK5 RAM ",CR
	.byte RVS_ON," 6 ",RVS_OFF," I/O2 RAM ",CR
	.byte RVS_ON," 7 ",RVS_OFF," I/O3 RAM ",CR
	.byte RVS_ON,"A",RVS_OFF,"-",RVS_ON,
config1	.byte "Y",RVS_OFF
	.byte " MEMORY BANK: ", CR, 0
banksel	.byte YELLOW, RVS_ON, " "
bankcur	.byte "A ", WHITE, CR, 0

	.byte 0
bank_on	.byte YELLOW, RVS_ON, " ENABLED", WHITE, CR, 0
bankoff	.byte YELLOW, RVS_ON, "DISABLED", WHITE, CR, 0
#if (bank_on/256) - (bankoff/256)
#error "page boundary crossed"
#endif
ultimem_not_detected
	.byte "ULTIMEM NOT DETECTED", 0
	;; UltiMem unit type identifier
unit_id	.byte ultimem_id_512k, ultimem_id_8m
unit_id_end
	;; RAM size (8K, 16K, 32K, 64K, 128K, 256K, 512K, 1024K, 2048K) / 40K
ramsize	.byte "AAAACFLYZ"
	;; configuration presets (default value of ram_blk[])
presets	.byte BLK_EN|IO_EN, RAM123_EN|BLK_EN|IO_EN
	.byte RAM123_EN|BLK5_EN|IO_EN
	.byte RAM123_EN, BLK1_EN, BLK1_EN|BLK2_EN, BLK1_EN|BLK2_EN|BLK3_EN
presets_end
	;; powers of 2
pow2	.byte 1,2,4,8,16,32,64
	;; RAM[123], I/O2 and I/O3 configuration table
ioram_cfg
	.byte $00, $03, $00, $03, $0c, $0f, $0c, $0f
	.byte $30, $33, $30, $33, $3c, $3f, $3c, $3f
