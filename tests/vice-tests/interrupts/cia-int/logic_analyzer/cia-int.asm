;**************************************************************************
;*
;* FILE  cia-int.asm
;* Copyright (c) 2010 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;*
;* DESCRIPTION
;*
;******
	processor 6502

	seg.u	zp
;**************************************************************************
;*
;* SECTION  zero page
;*
;******
	org	$c0
ptr_zp:
	ds.w	1
cnt_zp:
	ds.b	1
test_zp:
	ds.b	1
int_zp:
	ds.b	1
time_zp:
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
	lda	#$7f
	sta	$dc0d
	lda	$dc0d

	jsr	test_present
	
	sei
	lda	#$35
	sta	$01

	ldx	#6
sa_lp1:
	lda	vectors-1,x
	sta	$fffa-1,x
	dex
	bne	sa_lp1

	jsr	test_prepare
	
	jsr	test_perform

enda:
	jmp	enda
	sei
	lda	#$37
	sta	$01
	ldx	#0
	stx	$d01a
	inx
	stx	$d019
	jsr	$fda3
	jsr	test_result
sa_lp3:
	jmp	sa_lp3

vectors:
	ifconst	TEST_IRQ
	dc.w	nmi_entry, 0, int_entry
	endif
	ifconst	TEST_NMI
	dc.w	int_entry, 0, int_entry
	endif
	
sa_fl1:
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
;* NAME  test_present
;*   
;******
test_present:
	lda	#14
	sta	646
	sta	$d020
	lda	#6
	sta	$d021

	lda	#<greet_msg
	ldy	#>greet_msg
	jsr	$ab1e

	lda	646
	ldx	#0
tpr_lp1:
	sta	$d800,x
	sta	$d900,x
	sta	$da00,x
	sta	$dae8,x
	inx
	bne	tpr_lp1
	
	rts

greet_msg:
	ifconst	TEST_IRQ
	dc.b	147,"CIA-INT (IRQ) R04 / TLR",13,13
	dc.b	"DC0C: A9 XX 60",13
	dc.b	13,13,13
	dc.b	"DC0C: A5 XX 60",13
	dc.b	13,13,13
	dc.b	"DC0B: 0D A9 XX 60",13
	dc.b	13,13,13
	dc.b	"DC0B: 19 FF XX 60",13
	dc.b	13,13,13
	dc.b	"DC0C: AC XX A9 09 28 60",13
	dc.b	0
	endif
	ifconst	TEST_NMI
	dc.b	147,"CIA-INT (NMI) R04 / TLR",13,13
	dc.b	"DD0C: A9 XX 60",13
	dc.b	13,13,13
	dc.b	"DD0C: A5 XX 60",13
	dc.b	13,13,13
	dc.b	"DD0B: 0D A9 XX 60",13
	dc.b	13,13,13
	dc.b	"DD0B: 19 FF XX 60",13
	dc.b	13,13,13
	dc.b	"DD0C: AC XX A9 09 28 60",13
	dc.b	0
	endif

;**************************************************************************
;*
;* NAME  test_result
;*   
;******
test_result:
	lda	#<done_msg
	ldy	#>done_msg
	jsr	$ab1e

	rts

done_msg:
	dc.b	"DONE",13,13,0


	
;**************************************************************************
;*
;* NAME  test_prepare
;*   
;******
test_prepare:
	lda	#$34
	sta	$01

; make LDA $xx mostly return xx ^ $2f.
	ldx	#2
tpp_lp1:
	txa
	eor	#$2f
	sta	$00,x
	inx
	bne	tpp_lp1

; make LDA $xxA9 mostly return xx.
	ldx	#0
	stx	$00a9
	inx
	stx	$01a9
	inx
	stx	$02a9
	inx
	stx	$03a9
	
	ldx	#>[end+$ff]
tpp_lp2:
	stx	tpp_sm1+2
tpp_sm1:
	stx.w	$00a9
	inx
	bne	tpp_lp2

; make LDA $xxFF,Y with Y=$19 mostly return xx.
	ldx	#0
	stx	$00ff+$19
	inx
	stx	$01ff+$19
	inx
	stx	$02ff+$19
	
	ldx	#>[end+$ff]
tpp_lp3:
	stx	tpp_sm2+2
	inc	tpp_sm2+2
tpp_sm2:
	stx.w	$00ff+$19
	inx
	bne	tpp_lp3

; make LDA $A9xx return xx.
	ldx	#0
tpp_lp4:
	txa
	sta	$a900,x
	inx
	bne	tpp_lp4

	inc	$01

; initial test sequencer
	lda	#0
	sta	test_zp

	rts

	
;**************************************************************************
;*
;* NAME  test_perform
;*   
;******
test_perform:
; stay away from bad lines
tp_lp00:
	lda	$d011
	bmi	tp_lp00
tp_lp01:
	lda	$d011
	bpl	tp_lp01
; just below the visible screen here
	
	inc	$d020

; kill all interrupts
	lda	#$7f
	sta	$dc0d
	sta	$dd0d
	lda	$dc0d
	lda	$dd0d

	ldx	test_zp
    stx $deff
	jsr	do_test

; restore data direction and ports
	ldy	#0
	lda	#$ff
	sta	$dc02
	sty	$dc03
	lda	#$7f
	sta	$dc00
	lda	#$3f
	sta	$dd02
	sty	$dd03
	lda	#$17
	sta	$dd00

	ldx	test_zp
	inx
	cpx	#NUM_TESTS
	bne	tp_skp1
	ldx	#0
tp_skp1:
	stx	test_zp

	dec	$d020

	jmp	test_perform	; test forever
;	rts

NUM_TESTS	equ	5
scrtab:
	dc.w	$0400+40*3
	dc.w	$0400+40*7
	dc.w	$0400+40*11
	dc.w	$0400+40*15
	dc.w	$0400+40*19
addrtab:
	dc.b	$0c
	dc.b	$0c
	dc.b	$0b
	dc.b	$0b
	dc.b	$0c
convtab:
	dc.b	$ea,$ea		; NOP
	dc.b	$49,$2f		; EOR #$2f
	dc.b	$ea,$ea		; NOP
	dc.b	$ea,$ea		; NOP
	dc.b	$98,$ea		; TYA
offstab:
	dc.b	SEQTAB1
	dc.b	SEQTAB2
	dc.b	SEQTAB3
	dc.b	SEQTAB4
	dc.b	SEQTAB5
	
seqtab:
SEQTAB1	equ	.-seqtab
	dc.b	$0c,$a9		; LDA #<imm>
;	dc.b	$0d,%10000010	; timer B IRQ
	dc.b	$0e,$60		; RTS
	dc.b	$ff

SEQTAB2	equ	.-seqtab
	dc.b	$0c,$a5		; LDA <zp>
;	dc.b	$0d,%10000010	; timer B IRQ
	dc.b	$0e,$60		; RTS
	dc.b	$ff

SEQTAB3	equ	.-seqtab
	dc.b	$0f,%00000000	; TOD clock mode
	dc.b	$0b,$0d		; ORA <abs>
	dc.b	$0c,$a9		; $a9
;	dc.b	$0d,%10000010	; timer B IRQ
	dc.b	$0e,$60		; RTS
	dc.b	$ff

SEQTAB4	equ	.-seqtab
	dc.b	$0f,%00000000	; TOD clock mode
	dc.b	$0b,$19		; ORA <abs>,y
	dc.b	$0c,$ff		; $ff
;	dc.b	$0d,%10000010	; timer B IRQ
	dc.b	$0e,$60		; RTS
	dc.b	$ff

SEQTAB5	equ	.-seqtab
	dc.b	$02,%11111111   ; DDR out
	dc.b	$03,%11111111   ; DDR out
	dc.b	$0c,$ac		; LDY <abs>
;	dc.b	$0d,%10000010	; timer B IRQ
	dc.b	$0e,$a9		;
;	dc.b	$0f,%10000000	; ORA #<imm> or PHP 
	dc.b	$10,$28		; PLP
	dc.b	$11,$60		; RTS
	dc.b	$ff

	subroutine	do_test
	ifconst	TEST_IRQ
.cia_dut	equ	$dc00
.cia_sec	equ	$dd00
	endif
	ifconst	TEST_NMI
.cia_dut	equ	$dd00
.cia_sec	equ	$dc00
	endif

do_test:
	txa
	asl
	tay
	lda	scrtab,y
	sta	ptr_zp
	lda	scrtab+1,y
	sta	ptr_zp+1
	lda	addrtab,x
	sta	dt_sm1+1
	lda	convtab,y
	sta	dt_sm2
	lda	convtab+1,y
	sta	dt_sm2+1

	lda	.cia_dut+$08		; reset tod state
; set up test parameters
	ldy	offstab,x
dt_lp0:
	lda	seqtab,y
	bmi	dt_skp1
	sta	dt_sm0+1
	lda	seqtab+1,y
dt_sm0:
	sta	.cia_dut
	iny
	iny
	bne	dt_lp0
dt_skp1:

	ldy	#0
dt_lp1:
	sty	cnt_zp
    sty $dfff

	lda	#255
	sta	.cia_sec+$04
	ldx	#0
	stx	.cia_sec+$05
	sty	.cia_dut+$06
	stx	.cia_dut+$07
	stx	int_zp
	stx	time_zp
	bit	.cia_dut+$0d
	lda	#%10000010	; timer B IRQ
	sta	.cia_dut+$0d
	txa
	ldy	#%00011001
; X=0, Y=$19, Acc=0
;-------------------------------
; Test start
;-------------------------------
	cli
	sty	.cia_sec+$0e	; measurement
	sty	.cia_dut+$0f	; actual test
dt_sm1:
	jsr	.cia_dut+$0c
	sei
	ldx	#%01111111
	stx	.cia_dut+$0d
;-------------------------------
; Test end
;-------------------------------
dt_sm2:
	ds.b	2,$ea
; Acc=$Dx0D, int_zp, time_zp
	ldy	cnt_zp
	cmp	#0
	bne	dt_skp2
	lda	#"-"
dt_skp2:
	sta	(ptr_zp),y
	tya
	clc
	adc	#40
	tay
	lda	time_zp
	eor	#$7f
	clc
	sbc	#16
	ldx	int_zp
	bne	dt_skp3
	lda	#"-"
dt_skp3:
	sta	(ptr_zp),y

	ldy	cnt_zp
	iny
	cpy	#15
	bne	dt_lp1
	rts

int_entry:
	ldx	.cia_sec+$04
	bit	.cia_dut+$0d
	stx	time_zp
	inc	int_zp
	rti

end:
; eof
