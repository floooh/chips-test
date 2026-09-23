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

SCREENSHOTEXIT  equ 0

TEST_NAME	eqm	"LIGHTPEN"
TEST_REVISION	eqm	"R04"

	
	seg.u	zp
;**************************************************************************
;*
;* SECTION  zero page
;*
;******
	org	$02
ptr_zp:
	ds.w	1
rptr_zp:
	ds.w	1
cnt_zp:
	ds.b	1	
enable_zp:
	ds.b	1
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
	dc.b	13,"MEASURING $D011 AT CYCLE $00...",0

show_params:
	lda	$d3
	sec
	sbc	#18
	tay
	lda	pt_sm1+1
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

	lda	#<stability_msg
	ldy	#>stability_msg
	jsr	$ab1e

	ldx	#0
	jsr	check_guard
	cmp	#1
	beq	tr_skp1

	tax
	lda	#0
	jsr	$bdcd
	
	lda	#<failed_msg
	ldy	#>failed_msg
	jsr	$ab1e
	jmp	tr_skp2

tr_skp1:
	lda	#<passed_msg
	ldy	#>passed_msg
	jsr	$ab1e
tr_skp2:

	lda	#0
tr_lp1:
	sta	cnt_zp
	ldx	cnt_zp
	lda	ref_data,x
	sta	rptr_zp
	lda	ref_data+1,x
	sta	rptr_zp+1
	beq	tr_ex1

	lda	#<BUFFER
	sta	ptr_zp
	lda	#>BUFFER
	sta	ptr_zp+1

; check for matches
	ldx	#5
	ldy	#0
tr_lp2:
	lda	(ptr_zp),y
	cmp	(rptr_zp),y
	bne	tr_mismatch	;Z=1
	iny
	bne	tr_lp2
	inc	ptr_zp+1
	inc	rptr_zp+1
	dex
	bne	tr_lp2
	jmp	tr_match

tr_mismatch:
	lda	cnt_zp
	clc
	adc	#4
	jmp	tr_lp1

tr_match:
	lda	#5
	sta	$d020
	
	lda     #0      ; success
	sta     $d7ff

	lda	#<matches_msg
	ldy	#>matches_msg
	jsr	$ab1e

	ldx	cnt_zp
	lda	ref_data+2,x
	ldy	ref_data+3,x
	jsr	$ab1e

	lda	#<matches2_msg
	ldy	#>matches2_msg
	jsr	$ab1e

	jmp	tr_ex2
	
tr_ex1:
	lda	#10
	sta	$d020
        lda     #$ff    ; failure
        sta     $d7ff
	lda	#<nomatches_msg
	ldy	#>nomatches_msg
	jsr	$ab1e

tr_ex2:
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
	dc.b	"DONE",13,0

stability_msg:
	dc.b	13,"STABILITY: ",0
passed_msg:
	dc.b	"PASSED",13,0
failed_msg:
	dc.b	", FAILED!",13,0

matches_msg:
	dc.b	13,5,"> MATCHES ",0
matches2_msg:
	dc.b	" <",154,0

nomatches_msg:
	dc.b	13,5,"> NO MATCHES! <",154,0

result_msg:
	dc.b	13,13,"(RESULT AT $4000-$4500)",0


filename:
	dc.b	"LPDUMP"
FILENAME_LEN	equ	.-filename

;**************************************************************************
;*
;* NAME  test_prepare
;*
;******
test_prepare:
	lda	#%11111111
	sta	$dc00
	lda	#%00000000
	sta	$dc02
	lda	#%11111111
	sta	$dc01
	sta	$dc03
	lda	#$0f
	sta	$d019		; clear interrupts

	lda	#0
	sta	enable_zp
	sta	cycle_zp
	sta	test_num_zp
	jsr	setup_test

;	lda	#$1b | (>LINE << 7)
	lda	#$9b
	sta	$d011
	lda	num_lines
	sec
	sbc	#4
;	lda	#<LINE
	sta	$d012

	rts

	
;**************************************************************************
;*
;* NAME  test_perform
;*
;******
test_perform:
	lda	$dc06
	sta.w	guard_zp
	lda	enable_zp
	beq	pt_ex1

	lda	cycle_zp
	jsr	delay
	inc	$d020
	dec	$d020

	ldx	cycle_zp
pt_sm2:
	sta	BUFFER,x
	lda	$d019
	and	#$0f
	sta	BUFFER+$0400,x
	lda	#$0f
	sta	$d019		; clear interrupts

	ldx	#0
	ldy	guard_zp
	jsr	update_guard

; cosmetic print out
	jsr	show_params

; increase cycle
	inc	cycle_zp
	bne	pt_skp1

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
	lda	enable_zp
	eor	#1
	sta	enable_zp

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
	sta	pt_sm1+1
	lda	regtab+1,x
	sta	pt_sm1+2
	rts

	
NUM_TESTS	equ	4
buftab:
	dc.w	BUFFER+$0000
	dc.w	BUFFER+$0100
	dc.w	BUFFER+$0200
	dc.w	BUFFER+$0300
regtab:
	dc.w	$d011,$d012,$d013,$d014


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
	stx	$dc01
pt_sm1:
	lda	$d011
	ldx	#%11111111
	stx	$dc01
	rts

BUFFER		equ	$4000
BUFFER_END	equ	$4500



;**************************************************************************
;*
;* NAME  ref_data
;*
;******
ref_data:
	dc.w	tab6567,msg6567
	dc.w	tab6569r1,msg6569r1
	dc.w	tab6569r3r4,msg6569r3r4
	dc.w	tab6572,msg6572
	dc.w	tab8562r4,msg8562r4
	dc.w	tab8564,msg8564
	dc.w	tab8564r5,msg8564r5
	dc.w	tab8565r2,msg8565r2
	dc.w	tabdtv3pal,msgdtv3pal
	dc.w	0
;---
msg6567:
	dc.b	"6567",0
tab6567:
	incbin	"dumps/dump6567.bin"
;---
msg6569r1:
	dc.b	"6569R1",0
tab6569r1:
	incbin	"dumps/dump6569r1.bin"
;---
msg6569r3r4:
	dc.b	"6569R3/R4",0
tab6569r3r4:
	incbin	"dumps/dump6569.bin"
;---
msg6572:
	dc.b	"6572",0
tab6572:
	incbin	"dumps/dump6572.bin"
;---
msg8562r4:
	dc.b	"8562R4",0
tab8562r4:
	incbin	"dumps/dump8562r4.bin"
;---
msg8564:
	dc.b	"8564",0
tab8564:
	incbin	"dumps/dump8564.bin"
;---
msg8564r5:
	dc.b	"8564R5",0
tab8564r5:
	incbin	"dumps/dump8564r5.bin"
;---
msg8565r2:
	dc.b	"8565R2",0
tab8565r2:
	incbin	"dumps/dump8565.bin"
;---
msgdtv3pal:
	dc.b	"DTV3 PAL",0
tabdtv3pal:
	incbin	"dumps/dumpdtv3.bin"
;---

; eof
