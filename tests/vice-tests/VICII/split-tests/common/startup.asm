;**************************************************************************
;*
;* FILE  startup.asm
;* Copyright (c) 2010 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;*
;* DESCRIPTION
;*
;******
	processor 6502


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
	jsr	check_time
	sta	cycles_per_line
	stx	num_lines

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

	ldx	cycles_per_line
	cpx	#63
	bcc	sa_fl1		;<63, fail
	cpx	#66		;>=66, fail
	bcs	sa_fl1
	lda	time1-63,x
	sta	is_sm1+1

	ifconst	HAVE_STABILITY_GUARD
	jsr	start_guard
	endif
	
	jsr	test_prepare

	jsr	wait_vb

	lda	#1
	sta	$d019
	sta	$d01a

	cli
	
sa_lp2:
	if	0
; be evil to timing to provoke glitches
	inx
	bpl	sa_lp2
	inc	$4080,x
	dec	$4080,x
	endif
	ifconst	HAVE_TEST_CONTROLLER
	jsr	test_controller
	endif
	ifconst	HAVE_TEST_RESULT
	lda	test_done
	beq	sa_lp2
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
	else
	jmp	sa_lp2
	endif

vectors:
	dc.w	nmi_entry,0,irq_stable

sa_fl1:
nmi_entry:
	sei
	lda	#$37
	sta	$01
	jmp	$fe66

cycles_per_line:
	dc.b	0
num_lines:
	dc.b	0
	ifconst HAVE_TEST_RESULT
test_done:
	dc.b	0
	endif
time1:
	dc.b	irq_stable2_pal, irq_stable2_ntscold, irq_stable2_ntsc
	
;**************************************************************************
;*
;* NAME  irq_stable, irq_stable2
;*
;******
irq_stable:
	sta	accstore	; 4
	sty	ystore		; 4
	stx	xstore		; 4
	inc	$d019		; 6
	inc	$d012		; 6
is_sm1:
	lda	#<irq_stable2_pal	; 2
	sta	$fffe		; 4
	tsx			; 2
	cli			; 2
;10 * nop for PAL (63)
;11 * nop for NTSC (65)
	ds.b	11,$ea		; 22
; guard
is_lp1:
	sei
	inc	$d020
	jmp	is_lp1	

;28 cycles for NTSC (65)
;27 cycles for NTSCOLD (64)
;26 cycles for PAL (63)
irq_stable2_ntsc:
	dc.b	$2c		; bit <abs>
irq_stable2_ntscold:
	dc.b	$24		; bit <zp>
irq_stable2_pal:
	dc.b	$ea
	jsr	twelve		; 12
	jsr	twelve		; 12
;---
	txs			; 2
	dec	$d019		; 6
	dec	$d012		; 6
	lda	#<irq_stable	; 2
	sta	$fffe		; 4	=46
	lda	$d012		; 4
	eor	$d012		; 4
	beq	is2_skp1	; 2 (3)
is2_skp1:

	jsr	test_perform
#if SCREENSHOTEXIT	
	dec framecount
	bne skp
	lda #0
	sta $d7ff
skp:
#endif
accstore	equ	.+1
	lda	#0
xstore	equ	.+1
	ldx	#0
ystore	equ	.+1
	ldy	#0
	rti

framecount: dc.b 5
	
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
;* NAME  check_time
;*
;* DESCRIPTION
;*   Determine number of cycles per raster line.
;*   Acc = number of cycles.
;*   X = LSB of number of raster lines.
;*
;******
check_time:
	lda	#0
	sta	$dc0e
	jsr	wait_vb
;--- raster line 0
	lda	#$fe
	sta	$dc04
	sta	$dc05		; load timer with $fefe
	lda	#%00010001
	sta	$dc0e		; start one shot timer
ct_lp1:
	bit	$d011
	bpl	ct_lp1
;--- raster line 256
	lda	$dc05		; timer msb
	eor	#$ff		; invert
; Acc = cycles per line
;--- scan for raster wrap
ct_lp2:
	ldx	$d012
ct_lp3:
	cpx	$d012
	beq	ct_lp3
	bmi	ct_lp2
	inx
; X = number of raster lines (LSB)
twelve:
	rts


	ifconst	HAVE_STABILITY_GUARD
;**************************************************************************
;*
;* NAME  start_guard
;*
;* DESCRIPTION
;*   Setup a stability guard timer that counts cycles per line.
;*   It will be check to be the same value at the same spot in the raster
;*   routine each IRQ.
;*
;******
MAX_GUARD_CYCLES	equ	65
MAX_GUARDS		equ	4
start_guard:

; clear guard buffer
	lda	#0
	ldx	#MAX_GUARD_CYCLES
sg_lp1:
	sta	guard-1,x
	dex
	bne	sg_lp1
	sta	guard_count

; clear guard counters
	ldx	#MAX_GUARDS
sg_lp2:
	sta	guard_count-1,x
	sta	guard_last_cycle,x
	dex
	bne	sg_lp2

; setup and start guard timer
	ldx	#%00000000
	stx	$dc0f
	ldy	cycles_per_line
	dey
	sty	$dc06
	stx	$dc07
	lda	#%00010001
	sta	$dc0f

	rts

;**************************************************************************
;*
;* NAME  update_guard
;*
;* DESCRIPTION
;*   Update guard counts.
;*   IN: Y=cycle, X=guard# (0-3)
;*
;******
update_guard:
	cpy	cycles_per_line ; >= cycles_per_line
	bcs	ug_fl1		; yes, fault!
	tya
	sta	guard_last_cycle,x
	lda	guard_mask,x
	and	guard,y
	bne	ug_ex1
	lda	guard_mask,x
	ora	guard,y
	sta	guard,y
	inc	guard_count,x
ug_ex1:
	rts
ug_fl1:
	lda	#$80
	ora	guard_count,x
	sta	guard_count,x
	rts

guard_mask:
	dc.b	%00000001
	dc.b	%00000010
	dc.b	%00000100
	dc.b	%00001000
	
guard_count:
	ds.b	MAX_GUARDS
guard_last_cycle:
	ds.b	MAX_GUARDS
guard:
	ds.b	MAX_GUARD_CYCLES

;**************************************************************************
;*
;* NAME  check_guard
;*
;* DESCRIPTION
;*   Check guard counts.
;*   IN: X=guard# (0-3)
;*   
;******
check_guard:
	lda	guard_count,x
	rts

	endif	;HAVE_STABILITY_GUARD

	ifconst	HAVE_ADJUST
;**************************************************************************
;*
;* NAME  adjust_timing
;*
;******
adjust_timing:
	ldx	cycles_per_line
	lda	time2-63,x
	sta	tm1_zp
	lda	time3-63,x
	sta	tm2_zp
	
	lda	#<test_start
	sta	ptr_zp
	lda	#>test_start
	sta	ptr_zp+1
	ldx	#>[test_end-test_start+255]
at_lp1:
	ldy	#0
	lda	#$d8		; cld
	cmp	(ptr_zp),y
	bne	at_skp1
	iny
	cmp	(ptr_zp),y
	bne	at_skp1
	dey
	lda	tm1_zp
	sta	(ptr_zp),y
	iny
	lda	tm2_zp
	sta	(ptr_zp),y
at_skp1:
	inc	ptr_zp
	bne	at_lp1
	inc	ptr_zp+1
	dex
	bne	at_lp1

	rts

; eor #$00 (2),  bit $ea (3),  nop; nop (4)
time2:
	dc.b	$49, $24, $ea
time3:
	dc.b	$00, $ea, $ea

;******
; end of line marker
	mac	EOL
	ds.b	2,$d8
	endm

	endif	;HAVE_ADJUST
; eof
