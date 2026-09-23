;**************************************************************************
;*
;* FILE  onescreen.asm
;* Copyright (c) 2010 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;*
;* DESCRIPTION
;*   Common code for "static" one-screen split tests.
;*
;******

	ifnconst LABEL_SCRADDR
LABEL_SCRADDR	equ	$0400
	endif
	ifnconst LABEL_LOWERCASE
LABEL_LOWERCASE	equ	0
	endif

;**************************************************************************
;*
;* NAME  show_label_bar
;*
;* DESCRIPTION
;*   Check guards and display status on the label bar.
;*
;******
NAME_POS	equ	2
GUARD_POS	equ	27
CONF_POS	equ	32
label_msg:
	dc.b	147,"0123456789012345678901234567890123456789",19,0
name_msg:
	dc.b	TEST_NAME,29,TEST_REVISION,0
;	dc.b	"modesplit",29,"r02",0

show_label_bar:
	lda	#14
	sta	646
	sta	$d020
	lda	#6
	sta	$d021

	lda	#<label_msg
	ldy	#>label_msg
	jsr	$ab1e

	lda	#1
	sta	646

	lda	#NAME_POS
	sta	$d3
	lda	#<name_msg
	ldy	#>name_msg
	jsr	$ab1e
	
	lda	#CONF_POS
	sta	$d3
	lda	#0
	ldx	cycles_per_line
	jsr	$bdcd
	
	inc	$d3
	lda	#1
	ldx	num_lines
	jsr	$bdcd

	lda	#1
	sta	$d800+GUARD_POS+0
	sta	$d800+GUARD_POS+1
	sta	$d800+GUARD_POS+2
	lda	#"-"
	sta	$0400+GUARD_POS+0
	sta	$0400+GUARD_POS+1
	sta	$0400+GUARD_POS+2
	rts


;**************************************************************************
;*
;* NAME  show_guards
;*
;* DESCRIPTION
;*   Check guards and display status on the label bar.
;*
;******
show_guards:
	ldx	#1
shg_lp1:
	jsr	check_guard
	ora	#"0"
	cmp	#"9"+1
	bcc	shg_skp1
	lda	#"!"
shg_skp1:
	sta	LABEL_SCRADDR+GUARD_POS,x
	dex
	bpl	shg_lp1

	lda	#"?"
	ldy	guard_count+0
	cpy	#1
	bne	shg_skp2
	ldy	guard_count+1
	cpy	#1
	bne	shg_skp2
	ldy	guard_last_cycle+0
	cpy	guard_last_cycle+1
	bne	shg_skp2
	if	LABEL_LOWERCASE
	lda	#"P"
	else
	lda	#"P"&$3f
	endif
shg_skp2:
	sta	LABEL_SCRADDR+GUARD_POS+2
	rts


; eof
