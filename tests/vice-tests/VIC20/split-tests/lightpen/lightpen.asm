;**************************************************************************
;*
;* FILE  lightpen.asm 
;* Copyright (c) 2010 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;*
;* DESCRIPTION
;*
;******
	processor 6502

TEST_NAME	eqm	"LIGHTPEN"
	ifconst HARD_NTSC
TEST_REVISION	eqm	"R01N"
	else
TEST_REVISION	eqm	"R01"
	endif

	seg.u	zp
;**************************************************************************
;*
;* SECTION  zero page
;*
;******
	org	$00
enable_zp:
	ds.b	1
guard_zp:
	ds.b	1
cycle_zp:
	ds.b	1
test_num_zp:
	ds.b	1

;**************************************************************************
;*
;* common startup and raster code
;*
;******
HAVE_TEST_RESULT	equ	1
HAVE_STABILITY_GUARD	equ	1
;HAVE_TEST_CONTROLLER	equ	1
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
	jsr	$cb1e
	rts

measure_msg:
	dc.b	13,"R=$----,C=$--...",0

show_params:
	lda	$d3
	sec
	sbc	#13
	tay
	lda	reg_under_test+1
	jsr	update_hex
	lda	reg_under_test
	jsr	update_hex

	lda	$d3
	sec
	sbc	#5
	tay
	lda	cycle_zp
	jmp	update_hex
;	rts

;**************************************************************************
;*
;* NAME  test_result
;*
;******
test_result:
	lda	#<done_msg
	ldy	#>done_msg
	jsr	$cb1e

	lda	#<stability_msg
	ldy	#>stability_msg
	jsr	$cb1e

	ldx	#0
	jsr	check_guard
	sta	BUFFER+40	; stability
	cmp	#1
	beq	tr_skp1

	tax
	lda	#0
	jsr	$ddcd
	
	lda	#<failed_msg
	ldy	#>failed_msg
	jsr	$cb1e
	jmp	tr_skp2

tr_skp1:
	lda	#<passed_msg
	ldy	#>passed_msg
	jsr	$cb1e
tr_skp2:

	lda	#<result_msg
	ldy	#>result_msg
	jsr	$cb1e

	ldx	#<filename
	ldy	#>filename
	lda	#FILENAME_LEN
	jsr	$ffbd
	jsr	save_file
	
	rts

done_msg:
	dc.b	"DONE",13,0

stability_msg:
	dc.b	13,"STABILITY: ",0
passed_msg:
	dc.b	"PASSED",13,0
failed_msg:
	dc.b	", FAILED!",13,0

result_msg:
	dc.b	13,13,"(RESULT: $17C0-$1C00)",0


filename:
	dc.b	"LPDUMP"
FILENAME_LEN	equ	.-filename

;**************************************************************************
;*
;* NAME  test_prepare
;*
;******
test_prepare:
; setup info area
	ldx	#HEADER_LEN
	lda	#0
tpr_lp1:
	sta	BUFFER-1,x
	dex
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


; prepare the actual test
	lda	#0
	sta	enable_zp
	sta	cycle_zp
	sta	test_num_zp
	jsr	setup_test


; calculate raster line
	lda	num_lines
	sec
	sbc	#4
; C=1, not less that 0, C=0, less than 1
	ror
	sta	raster_line
; C = lsb
	ror
	sta	raster_lsb



	lda	#%00100000
	sta	$9111
	lda	#%10100000
	sta	$9113
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
	lda	$9114
	sta	guard_zp
	lda	enable_zp
	beq	tp_ex1
	
	lda	cycle_zp
	jsr	delay
	tax

	lda	$900f
	eor	#$f7
	sta	$900f
	eor	#$f7
	sta	$900f

	txa
	ldx	cycle_zp
target_buf	equ	.+1
	sta	BUFFER,x


	ldx	#0
	ldy	guard_zp
	jsr	update_guard

; cosmetic print out
	jsr	show_params

; increase cycle
	inc	cycle_zp
	bne	tp_skp1

	inc	test_num_zp
	lda	test_num_zp
	cmp	#NUM_TESTS
	bne	tp_skp1

	lda	#$60		;RTS
	sta	test_perform
	inc	test_done
	bne	tp_ex1

tp_skp1:
	jsr	setup_test

tp_ex1:
	lda	#1
	eor	enable_zp
	sta	enable_zp
	rts


setup_test:
	lda	test_num_zp
	asl
	tax
; X=test_num_zp * 2
	lda	buftab,x
	sta	target_buf
	lda	buftab+1,x
	sta	target_buf+1
	lda	regtab,x
	sta	reg_under_test
	lda	regtab+1,x
	sta	reg_under_test+1
	rts

	
NUM_TESTS	equ	4
buftab:
	dc.w	BUFFER_RES+$0000
	dc.w	BUFFER_RES+$0100
	dc.w	BUFFER_RES+$0200
	dc.w	BUFFER_RES+$0300
regtab:
	dc.w	$9003,$9004,$9006,$9007



	align	256
delay:
	eor	#$ff
	lsr
	sta	dl_sm1+1
	bcc	dl_skp1
dl_skp1:
	clv
dl_sm1:
	bvc	dl_skp1
	ds.b	127,$ea
;******
; start of test
	ldx	#%00000000
	stx	$9111
reg_under_test	equ	.+1
	lda	$ffff
	ldx	#%00100000
	stx	$9111
	rts


HEADER_LEN	equ	$40
BUFFER		equ	$17c0
BUFFER_RES	equ	$1800
BUFFER_END	equ	$1c00

; eof
