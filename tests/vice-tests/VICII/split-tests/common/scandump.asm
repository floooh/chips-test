;**************************************************************************
;*
;* FILE  scandump.asm
;* Copyright (c) 2010 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;*
;* DESCRIPTION
;*   Common code for scanning tests providing a data-dump.
;*
;******

;**************************************************************************
;*
;* NAME  show_info
;*
;* DESCRIPTION
;*   Show initial info on screen.
;*
;******
show_info:
	lda	#14
	sta	646
	sta	$d020
	lda	#6
	sta	$d021

	lda	#<timing_msg
	ldy	#>timing_msg
	jsr	$ab1e
	lda	#0
	ldx	cycles_per_line
	jsr	$bdcd
	lda	#<cycles_line_msg
	ldy	#>cycles_line_msg
	jsr	$ab1e
	lda	#1
	ldx	num_lines
	jsr	$bdcd
	lda	#<lines_msg
	ldy	#>lines_msg
	jsr	$ab1e
	
	rts

timing_msg:
	dc.b	147,TEST_NAME," ",TEST_REVISION," / TLR",13,13
	dc.b	"TIMING: ",0
cycles_line_msg:
	dc.b	" CYCLES/LINE, ",0
lines_msg:
	dc.b	" LINES",13,0

;**************************************************************************
;*
;* NAME  test_prepare
;*
;* DESCRIPTION
;*   Update hex byte on the current line.
;*
;******
update_hex:
	pha
	lsr
	lsr
	lsr
	lsr
	jsr	uh_common
	pla
	and	#$0f
uh_common:
	tax
	lda	htab,x
	sta	($d1),y
	iny
	rts

htab:
	dc.b	"0123456789",1,2,3,4,5,6


;**************************************************************************
;*
;* NAME  save_file
;*
;* DESCRIPTION
;*   Save dump file.
;*
;******
sa_zp	equ	$fb

save_file:
; set device num to 8 if less than 8.
	lda	#8
	cmp	$ba
	bcc	sf_skp1
	sta	$ba
sf_skp1:

	lda	#<save_to_disk_msg
	ldy	#>save_to_disk_msg
	jsr	$ab1e
sf_lp1:
	jsr	$ffe4
	cmp	#"N"
	beq	sf_ex1
	cmp	#"Y"
	bne	sf_lp1

	lda	#<filename_msg
	ldy	#>filename_msg
	jsr	$ab1e
	ldx	$d3
	ldy	#0
sf_lp2:
	cpy	$b7
	beq	sf_skp2
	lda	($bb),y
	jsr	$ffd2
	iny
	bne	sf_lp2		; always taken
sf_skp2:
	stx	$d3

	ldx	#<namebuf
	ldy	#>namebuf
;	lda	#NAMEBUF_LEN	; don't care
	jsr	$ffbd

	ldy	#0
sf_lp3:
	jsr	$ffcf
	sta	($bb),y
	cmp	#13
	beq	sf_skp3
	iny
	cpy	#NAMEBUF_LEN
	bne	sf_lp3
sf_skp3:
	sty	$b7

	lda	#$80
	sta	$9d
	lda	#1
	ldx	$ba
	tay
	jsr	$ffba
	ldx	#<BUFFER
	ldy	#>BUFFER
	stx	sa_zp
	sty	sa_zp+1
	lda	#sa_zp
	ldx	#<BUFFER_END
	ldy	#>BUFFER_END
	jsr	$ffd8

	lda	#<ok_msg
	ldy	#>ok_msg
	jsr	$ab1e

	lda	#<again_msg
	ldy	#>again_msg
	jsr	$ab1e
	jmp	sf_lp1
sf_ex1:
	lda	#<ok_msg
	ldy	#>ok_msg
	jsr	$ab1e
	rts

save_to_disk_msg:
	dc.b	13,13,"SAVE TO DISK? ",0
again_msg:
	dc.b	13,"SAVE AGAIN? ",0
filename_msg:
	dc.b	13,"FILENAME: ",0
ok_msg:
	dc.b	13,"OK",13,0
NAMEBUF_LEN	equ	32
namebuf:
	ds.b	NAMEBUF_LEN


; eof
