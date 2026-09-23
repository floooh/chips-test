;**************************************************************************
;*
;* FILE  bascan.asm
;* Copyright (c) 2010 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;*
;* DESCRIPTION
;*
;******
	processor 6502
	
SCREENSHOTEXIT  equ 0

TEST_NAME	eqm	"BASCAN"
TEST_REVISION	eqm	"R??"
LINE	equ	48+8*16+6

	seg.u	zp
;**************************************************************************
;*
;* SECTION  zero page
;*
;******
	org	$02
cycle_zp:
	ds.b	1
test_num_zp:
	ds.b	1
guard_zp:
	ds.b	1

;**************************************************************************
;*
;* common startup and raster code
;*
;******
HAVE_TEST_RESULT	equ	1
HAVE_STABILITY_GUARD	equ	1
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
	dc.b	13,"MEASURING $DC04 AT CYCLE $00...",0

show_params:
	lda	$d3
	sec
	sbc	#20
	tay
	lda	curr_reg+1
	jsr	update_hex
	lda	curr_reg
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
	jsr	$ab1e

	lda     #>BUFFER
	sta     chksrc+1
	lda     #>refdata
	sta     chkref+1

	ldy     #9
chklp2:
	ldx     #0
chklp1:
chksrc = * + 1
        lda     BUFFER,x
chkref = * + 1
        cmp     refdata,x
        bne     failed
        inx
        bne     chklp1
        inc     chksrc+1
        inc     chkref+1
        dey
        bne     chklp2

        lda     #5
        sta     $d020
        lda     #0
        sta     $d7ff
        lda     #<passed_msg
        ldy     #>passed_msg

        jmp     chkcont
failed:
        lda     #10
        sta     $d020
        lda     #$ff
        sta     $d7ff

        lda     #<failed_msg
        ldy     #>failed_msg
chkcont:
        jsr     $ab1e

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
        dc.b    "DONE",13,13,0

passed_msg:
        dc.b    "PASSED",13,13,0

failed_msg:
        dc.b    "FAILED",13,13,0

result_msg:
	dc.b	"(RESULT AT $4000-$4900)",0

filename:
	dc.b	"BARESULT"
FILENAME_LEN	equ	.-filename
	
;**************************************************************************
;*
;* NAME  test_prepare
;*
;******
test_prepare:

; setup measurement timer
	ldx	#%00000000
	stx	$dc0e
	ldx	#$ff
	stx	$dc04
	stx	$dc05
	
; prepare test
	lda	#0
	sta	cycle_zp
	sta	test_num_zp
	jsr	setup_test

; setup initial raster line
	lda	#$1b | (>LINE << 7)
	sta	$d011
	lda	#<LINE
	sta	$d012

	rts

	
;**************************************************************************
;*
;* NAME  test_perform
;*
;******
test_perform:
; check guard
	lda	$dc06
	sta	guard_zp

	lda	cycle_zp
	jsr	delay
	inc	$d021
	dec	$d021
	ldx	cycle_zp
pt_sm2:
	sta	BUFFER,x

	ldy	guard_zp
	ldx	#0
	jsr	update_guard
	jsr	check_guard
	sta	$0427
	lda	#1
	sta	$d827
	
; cosmetic print out
	jsr	show_params

; increase cycle
	inc	cycle_zp
	bne	pt_ex1

	inc	test_num_zp
	lda	test_num_zp
	cmp	#NUM_TESTS
	bne	pt_skp1

	lda	#$60
	sta	test_perform
	sta	test_done
	bne	pt_ex1

pt_skp1:
	jsr	setup_test
pt_ex1:
	rts

setup_test:
	lda	test_num_zp
	asl
	tax
; X=test_num_zp * 2
	lda	buftab,x
	sta	pt_sm2+1
	lda	buftab+1,x
	sta	pt_sm2+2
	lda	regtab,x
	sta	curr_reg
	lda	regtab+1,x
	sta	curr_reg+1

	lda	test_num_zp
	asl
	asl
	asl
	tax
; X=test_num_zp * 8
	ldy	#0
st_lp1:
	lda	tailtab,x
	sta	dl_tail,y
	inx
	iny
	cpy	#8
	bne	st_lp1
	rts

	
NUM_TESTS	equ	9
buftab:
	dc.w	BUFFER+$0000
	dc.w	BUFFER+$0100
	dc.w	BUFFER+$0200
	dc.w	BUFFER+$0300
	dc.w	BUFFER+$0400
	dc.w	BUFFER+$0500
	dc.w	BUFFER+$0600
	dc.w	BUFFER+$0700
	dc.w	BUFFER+$0800
regtab:
	dc.w	$dc04,$dc05,$d012,$dc04,$dc05,$d012,$dc04,$dc05,$d012
tailtab:
; test #0
	lda	$dc04
	stx	$dc0e
	eor	#$ff
; test #1
	lda	$dc05
	stx	$dc0e
	eor	#$ff
; test #2
	lda	$d012
	stx	$dc0e
	nop
	nop
; test #3
	stx	$dc0e
	lda	$dc04
	eor	#$ff
; test #4
	stx	$dc0e
	lda	$dc05
	eor	#$ff
; test #5
	stx	$dc0e
	lda	$d012
	nop
	nop
; test #6
	lsr	$dc0e,x
	lda	$dc04
	eor	#$ff
; test #7
	lsr	$dc0e,x
	lda	$dc05
	eor	#$ff
; test #8
	lsr	$dc0e,x
	lda	$d012
	nop
	nop

curr_reg:
	dc.w	0

	align	256
delay:
	ldx	#%00011001	; force timer start
	stx	$dc0e
	ldx	#%00000000	; value for stop timer
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
	txa
; Acc=0, X=0
dl_tail:
	stx	$dc0e
	lda	$dc04
	nop
	nop
	rts

;**************************************************************************
;*
;* NAME  ref_data
;*
;******

BUFFER		equ	$4000
BUFFER_END	equ	$4900

refdata:
        incbin "dump-c64.bin"

; eof
