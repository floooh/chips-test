;**************************************************************************
;*
;* FILE  spritescan.asm
;* Copyright (c) 2010 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;*
;* DESCRIPTION
;*   Scan for sprite coordinate related anomalies
;*
;******
	processor 6502

SCREENSHOTEXIT  equ 0

    if VICTYPE = 6569
                  ;123456789012345678901234567890
TEST_NAME eqm     "SPRITESCAN (6569/PAL)"
    endif
    if VICTYPE = 6572
                  ;123456789012345678901234567890
TEST_NAME eqm     "SPRITESCAN (6572/DREAN)"
    endif
TEST_REVISION	eqm	"R01"
LINE	equ	48+8*7+6
SPRPOS	equ	LINE+6
	
	seg.u	zp
;**************************************************************************
;*
;* SECTION  zero page
;*
;******
	org	$02
enable_zp:
	ds.b	1
xpos0_zp:
	ds.b	1
xpos1_zp:
	ds.b	1
xposmsb_zp:
	ds.b	1
bufptr_zp:
	ds.w	1
spr0_zp:
	ds.b	1
spr1_zp:
	ds.b	1
msbmask0_zp:
	ds.b	1
msbmask1_zp:
	ds.b	1
bit_zp:
	ds.b	1
bit2_zp:
	ds.b	1
polarity_zp:
	ds.b	1
tptr_zp:
	ds.w	1
offs_zp:
	ds.b	1
passid_zp:
	ds.b	1

;**************************************************************************
;*
;* common startup and raster code
;*
;******
HAVE_TEST_RESULT	equ	1
;HAVE_STABILITY_GUARD	equ	1
HAVE_TEST_CONTROLLER	equ	1
	include	"../common/startup.asm"

	include	"../common/scandump.asm"

;**************************************************************************
;*
;* NAME  test_present
;*
;******
test_present:
	jsr	show_info

	lda	#<measure_msg
	ldy	#>measure_msg
	jsr	$ab1e
	rts

measure_msg:
	dc.b	13,"SCAN #.: PASS . (#.), X=$......",0

show_params:
; sprnum 
	lda	$d3
	sec
	sbc	#25
	tay
	lda	spr0_zp
	ora	#"0"
	sta	($d1),y

; pass
	lda	$d3
	sec
	sbc	#17
	tay
	lda	passid_zp
	and	#$3f
	sta	($d1),y

; sprnum 
	lda	$d3
	sec
	sbc	#13
	tay
	lda	spr1_zp
	ora	#"0"
	sta	($d1),y
	

; coordinate
	lda	$d3
	sec
	sbc	#6
	tay
	ldx	#"0"
	lda	xposmsb_zp
	and	msbmask0_zp
	beq	sp_skp1
	inx
sp_skp1:
	txa
	sta	($d1),y
	iny
	lda	xpos0_zp
	jsr	update_hex

	rts
	
;**************************************************************************
;*
;* NAME  test_result
;*
;******
test_result:
	lda	#<done_msg
	ldy	#>done_msg
	jsr	$ab1e

    jsr checkref	

	lda	#<result_msg
	ldy	#>result_msg
	jsr	$ab1e


	ldx	#<filename
	ldy	#>filename
	lda	#FILENAME_LEN
	jsr	$ffbd
	jsr	save_file

	rts

done_msg:
	dc.b	"DONE",13,13,0

result_msg:
	dc.b	13,13,"(RESULT AT $6F00-$8000)",0

filename:
	dc.b	"SSCRESULT"
FILENAME_LEN	equ	.-filename
	
;**************************************************************************
;*
;* NAME  test_prepare
;*
;******
test_prepare:
; setup info area
	ldx	#0
	txa
tpr_lp1:
	sta	BUFFER,x
	inx
	bne	tpr_lp1

	ldx	#IDENT_LEN
tpr_lp2:
	lda	ident-1,x
	sta	BUFFER-1,x
	dex
	bne	tpr_lp2
	lda	cycles_per_line
	sta	BUFFER+32
	lda	num_lines
	sta	BUFFER+33
	lda	#1
	sta	BUFFER+34
	
; prepare test
	lda	#0
	sta	enable_zp

; clear sprite area
	ldx	#$80
tpr_lp3:
	sta	$3f80-1,x
	dex
	bne	tpr_lp3
	
; setup initial raster line
	lda	#$1b | (>LINE << 7)
	sta	$d011
	lda	#<LINE
	sta	$d012

	rts

ident:
	dc.b	TEST_NAME," ",TEST_REVISION
IDENT_LEN	equ	.-ident
	
;**************************************************************************
;*
;* NAME  test_perform
;*
;******
test_perform:
	lda	enable_zp
	bne	tp_skp1
	rts
tp_skp1:

	ldx	#0
tp_lp1:
	inc	$d020
	lda	sprtab,x
tp_sm1:
SM_SUT_YREG	equ	.+1
	sta	$d001
SM_SUP_YREG	equ	.+1
	sta	$d009

	lda	xpos0_zp
SM_SUT_XREG	equ	.+1
	sta	$d000
	lda	xpos1_zp
SM_SUP_XREG	equ	.+1
	sta	$d008
	lda	xposmsb_zp
	sta	$d010

	dec	$d020
	
; wait until one line before the measurement line
	lda	postab1,x
tp_lp2:
	cmp	$d012
	bne	tp_lp2
	lda	#0
	sta	$d021
;******
; before measurement line
	lda	$d01e

; wait until one line after the measurement line
	lda	postab2,x
tp_lp3:
	cmp	$d012
	bne	tp_lp3
	jsr	twelve
;******
; after measurement line
	lda	$d01e
	beq	tp_skp3
	lda	bit_zp
tp_skp3:
	sta	$d021
	ldy	xpos0_zp
	ora	(bufptr_zp),y
	sta	(bufptr_zp),y
	lda	#6
	sta	$d021
	
	inc	xpos0_zp
	bne	tp_skp4
	lda	msbmask0_zp
	eor	xposmsb_zp
	sta	xposmsb_zp
	inc	bufptr_zp+1
tp_skp4:

	inc	xpos1_zp
	bne	tp_skp5
	lda	msbmask1_zp
	eor	xposmsb_zp
	sta	xposmsb_zp
tp_skp5:
	
	inx
	cpx	#16
	bne	tp_lp1

	lda	xpos0_zp
	bne	pt_ex1
	lda	xposmsb_zp
	and	msbmask0_zp
	bne	pt_ex1

	dec	xpos0_zp
	lda	msbmask0_zp
	sta	xposmsb_zp
	lda	#0
	sta	enable_zp
pt_ex1:
	jsr	show_params
	
	rts


	
SPLITPOS1	equ	SPRPOS+2
postab1:
	dc.b	SPLITPOS1+8*0, SPLITPOS1+8*1, SPLITPOS1+8*2, SPLITPOS1+8*3
	dc.b	SPLITPOS1+8*4, SPLITPOS1+8*5, SPLITPOS1+8*6, SPLITPOS1+8*7
	dc.b	SPLITPOS1+8*8, SPLITPOS1+8*9, SPLITPOS1+8*10,SPLITPOS1+8*11
	dc.b	SPLITPOS1+8*12,SPLITPOS1+8*13,SPLITPOS1+8*14,SPLITPOS1+8*15
SPLITPOS2	equ	SPRPOS+5
postab2:
	dc.b	SPLITPOS2+8*0, SPLITPOS2+8*1, SPLITPOS2+8*2, SPLITPOS2+8*3
	dc.b	SPLITPOS2+8*4, SPLITPOS2+8*5, SPLITPOS2+8*6, SPLITPOS2+8*7
	dc.b	SPLITPOS2+8*8, SPLITPOS2+8*9, SPLITPOS2+8*10,SPLITPOS2+8*11
	dc.b	SPLITPOS2+8*12,SPLITPOS2+8*13,SPLITPOS2+8*14,SPLITPOS2+8*15
sprtab:
	dc.b	SPRPOS+8*0,    SPRPOS+8*0,    SPRPOS+8*0,    SPRPOS+8*3
	dc.b	SPRPOS+8*3,    SPRPOS+8*3,    SPRPOS+8*6,    SPRPOS+8*6
	dc.b	SPRPOS+8*6,    SPRPOS+8*9,    SPRPOS+8*9,    SPRPOS+8*9
	dc.b	SPRPOS+8*12,   SPRPOS+8*12,   SPRPOS+8*12,   SPRPOS+8*15
	

;**************************************************************************
;*
;* NAME  test_controller
;*
;******
test_controller:

tc_lp1:
	jsr	get_seq
	asl
	tax
	lda	tc_tab,x
	sta	tc_sm1+1
	lda	tc_tab+1,x
	sta	tc_sm1+2
tc_sm1:
	jsr	tc_sm1
	jmp	tc_lp1

cmd_exit:
	pla
	pla
	inc	test_done
	rts

CMD_END		equ	$00
CMD_CLEAR	equ	$01
CMD_CLEAR_PASS	equ	$02
CMD_PATTERN	equ	$03
CMD_PASS	equ	$04
CMD_ACCUMULATE	equ	$05
tc_tab:
	dc.w	cmd_exit
	dc.w	cmd_clear
	dc.w	cmd_clear_pass
	dc.w	cmd_pattern
	dc.w	cmd_pass
	dc.w	cmd_accumulate

	mac	ENDE
	dc.b	CMD_END
	endm

;**************************************************************************
;*
;* NAME  cmd_clear_pass, cmd_clear
;*
;******
	mac	CLEAR
	dc.b	CMD_CLEAR
	dc.w	{1}		;BUFFER
	endm

	mac	CLEAR_PASS
	dc.b	CMD_CLEAR_PASS
	dc.b	{1}		;passid
	endm

cmd_clear_pass:
	lda	#%00000001
	sta	bit_zp
	lda	#<CORR_BUF
	sta	tptr_zp
	lda	#>CORR_BUF
	sta	tptr_zp+1
	jsr	ccl_common
	jsr	get_seq
	sta	passid_zp
	rts

	
cmd_clear:
	jsr	get_seq
	sta	tptr_zp
	jsr	get_seq
	sta	tptr_zp+1

ccl_common:
	ldx	#2
	ldy	#0
	tya
ccl_lp1:
	sta	(tptr_zp),y
	iny
	bne	ccl_lp1
	inc	tptr_zp+1
	dex
	bne	ccl_lp1

	rts

;**************************************************************************
;*
;* NAME  cmd_pattern
;*
;******
	mac	PATTERN
	dc.b	CMD_PATTERN
	dc.b	{1}		;offset
	endm

	mac	sprpatt
	dc.b	[{1}>>16]&$ff,[{1}>>8]&$ff,{1}&$ff
	endm

cmd_pattern:
	jsr	get_seq
	sta	offs_zp

	ldy	#0
cpt_lp1:
	jsr	get_seq
	sta	$3f80+3*2,y
	sta	$3f80+3*10,y
	sta	$3f80+3*18,y
	iny
	cpy	#6
	bne	cpt_lp1

	ldy	#0
cpt_lp2:
	jsr	get_seq
	sta	$3fc0+3*2,y
	sta	$3fc0+3*10,y
	sta	$3fc0+3*18,y
	iny
	cpy	#6
	bne	cpt_lp2

	rts

;**************************************************************************
;*
;* NAME  cmd_pass
;*
;******
	mac	PASS
	dc.b	CMD_PASS
	dc.b	{1}		;SUT
	dc.b	{2}		;SUP
	endm

cmd_pass:

; sprite under test
	jsr	get_seq
	sta	spr0_zp
	tay
	lda	bittab,y
	sta	msbmask0_zp
	lda	#1
	sta	$d027,y
	lda	#$fe
	sta	$07f8,y
	tya
	asl
	tay
	sty	SM_SUT_XREG
	iny
	sty	SM_SUT_YREG
	
; measurement sprite
	jsr	get_seq
	sta	spr1_zp
	tay
	lda	bittab,y
	sta	msbmask1_zp
	lda	#12
	sta	$d027,y
	lda	#$ff
	sta	$07f8,y
	tya
	asl
	tay
	sty	SM_SUP_XREG
	iny
	sty	SM_SUP_YREG

; setup sprite enables
	lda	msbmask0_zp
	ora	msbmask1_zp
	sta	$d015

; setup sprite positions
	lda	#0
	sta	xpos0_zp
; Acc = xposmsb
	ldy	offs_zp
	sty	xpos1_zp
	bpl	st_skp2
; sign extend toggle msb
	lda	msbmask1_zp
st_skp2:
	sta	xposmsb_zp

; setup pointer
	lda	#<CORR_BUF
	sta	bufptr_zp
	lda	#>CORR_BUF
	sta	bufptr_zp+1

; run test
	inc	enable_zp
st_lp1:
	lda	enable_zp
	bne	st_lp1

; update mask for the next pass
	asl	bit_zp

	rts
	

bittab:
	dc.b	%00000001
	dc.b	%00000010
	dc.b	%00000100
	dc.b	%00001000
	dc.b	%00010000
	dc.b	%00100000
	dc.b	%01000000
	dc.b	%10000000

;**************************************************************************
;*
;* NAME  cmd_accumulate
;*
;******
	mac	ACCUMULATE
	dc.b	CMD_ACCUMULATE
	dc.w	{1}		;BUFFER
	dc.b	{2}		;mask
	dc.b	{3}		;polarity
	endm

cmd_accumulate:
	jsr	get_seq
	sta	tptr_zp
	jsr	get_seq
	sta	tptr_zp+1
	jsr	get_seq
	sta	bit_zp
	jsr	get_seq
	sta	polarity_zp
	beq	cac_skp1
	lda	bit_zp
cac_skp1:
	sta	bit2_zp
	
	lda	#<CORR_BUF
	sta	bufptr_zp
	lda	#>CORR_BUF
	sta	bufptr_zp+1
; accumulate result
	ldx	#2
	ldy	#0
cac_lp1:
	lda	(bufptr_zp),y
	eor	polarity_zp
	beq	cac_skp2
	lda	bit_zp
cac_skp2:
	eor	bit2_zp
	ora	(tptr_zp),y
	sta	(tptr_zp),y
	iny
	bne	cac_lp1
	inc	bufptr_zp+1
	inc	tptr_zp+1
	dex
	bne	cac_lp1
	
	rts

	
;**************************************************************************
;*
;* NAME  get_seq
;*
;******
get_seq:
	inc	gs_sm1+1
	bne	gs_skp1
	inc	gs_sm1+2
gs_skp1:
gs_sm1:
	lda	seq-1
	rts
	

;**************************************************************************
;*
;* NAME  test sequence
;*
;******
	mac	full_sprite_scan
_buf	set	{1}
_sut	set	{2}
_sup1	set	{3}
_sup2	set	{4}
_sup3	set	{5}
	
	CLEAR	_buf
	PATTERN	0	;offset
	sprpatt	%100000000000000000000000 ;SUT #0
	sprpatt	%100000000000000000000000 ;SUT #1
	sprpatt	%100000000000000000000000 ;SUP #0
	sprpatt	%100000000000000000000000 ;SUP #1
	CLEAR_PASS "A"
	PASS	_sut,_sup1
	PASS	_sut,_sup2
	PASS	_sut,_sup3
	ACCUMULATE _buf,%00000001,%00000000

	PATTERN	-23	;offset
	sprpatt                        %100000000000000000000000 ;SUT #0
	sprpatt                        %100000000000000000000000 ;SUT #1
	sprpatt %111111111111111111111110 ;SUP #0
	sprpatt %111111111111111111111110 ;SUP #1
	CLEAR_PASS "B"
	PASS	_sut,_sup1
	PASS	_sut,_sup2
	PASS	_sut,_sup3
	ACCUMULATE _buf,%00000010,%00000111

	PATTERN	-1	;offset
	sprpatt  %100000000000000000000000 ;SUT #0
	sprpatt  %100000000000000000000000 ;SUT #1
	sprpatt %001111111111111111111111 ;SUP #0
	sprpatt %001111111111111111111111 ;SUP #1
	CLEAR_PASS "C"
	PASS	_sut,_sup1
	PASS	_sut,_sup2
	PASS	_sut,_sup3
	ACCUMULATE _buf,%00000100,%00000111

	PATTERN	0	;offset
	sprpatt %000000000000000000000001 ;SUT #0
	sprpatt %000000000000000000000001 ;SUT #1
	sprpatt %000000000000000000000001 ;SUP #0
	sprpatt %000000000000000000000001 ;SUP #1
	CLEAR_PASS "D"
	PASS	_sut,_sup1
	PASS	_sut,_sup2
	PASS	_sut,_sup3
	ACCUMULATE _buf,%00001000,%00000000

	endm


seq:
	full_sprite_scan BUFFER_RES+$0000,0,2,4,6
	full_sprite_scan BUFFER_RES+$0200,1,3,5,7
	full_sprite_scan BUFFER_RES+$0400,2,4,6,0
	full_sprite_scan BUFFER_RES+$0600,3,5,7,1
	full_sprite_scan BUFFER_RES+$0800,4,6,0,2
	full_sprite_scan BUFFER_RES+$0a00,5,7,1,3
	full_sprite_scan BUFFER_RES+$0c00,6,0,2,4
	full_sprite_scan BUFFER_RES+$0e00,7,1,3,5

	ENDE
	

;**************************************************************************
;*
;* NAME  ref_data
;*
;******

checkref:
    lda #>reference
    sta chkrefaddr+1+1
    lda #>BUFFER
    sta chkbufaddr+1+1

    ldy #$11

chklp2:    

    ldx #32
chklp1:
chkrefaddr:
    lda reference,x
chkbufaddr:
    cmp BUFFER,x
    bne chkfailed
    inx
    bne chklp1

    inc chkrefaddr+1+1
    inc chkbufaddr+1+1
    
    dey
    bne chklp2
    
    lda #5
    sta $d020
    lda #$00
    sta $d7ff
    
    rts

chkfailed:
    lda #10
    sta $d020
    lda #$ff
    sta $d7ff
    rts
    
reference:
    if VICTYPE = 6569
    incbin "dumps/dump6569.bin"         ; $1100 bytes
    endif
    if VICTYPE = 6572
    incbin "dumps/dump6572.bin"         ; $1100 bytes
    endif


CORR_BUF	equ	$5000


BUFFER		equ	$6f00
BUFFER_RES	equ	$7000
BUFFER_END	equ	$8000



; eof
