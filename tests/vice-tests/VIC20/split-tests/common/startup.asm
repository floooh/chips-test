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
	org	$1001
;**************************************************************************
;*
;* Basic line!
;*
;******
start_of_line:
	dc.w	end_line
	dc.w	0
	dc.b	$9e,"4117 /T.L.R/",0
;	        0 SYS4117 /T.L.R/
end_line:
	dc.w	0

;**************************************************************************
;*
;* NAME  startofcode
;*
;******
startofcode:
	sei
	lda	#$7f
	sta	$912e
	sta	$912d
	sta	$911e
	sta	$911d
	ifconst	HARD_NTSC
	lda	#65
	ldx	#<261
	else
	jsr	check_time
	endif
	sta	cycles_per_line
	stx	num_lines
	jsr	calc_frame_time
	
	jsr	test_present

	sei
	lda	#<irq_stable
	sta	$0314
	lda	#>irq_stable
	sta	$0315

	ifconst	HAVE_STABILITY_GUARD
	jsr	start_guard
	endif

	jsr	test_prepare

	lda	cycles_per_frame
	sec
	sbc	#TIMING_ADJUST
	sta	adjust

; Calculate timer load value for stable point
	lda	cycles_per_frame
	sec
	sbc	#2
	sta	timer_stable_value
	lda	cycles_per_frame+1
	sbc	#0
	sta	timer_stable_value+1

; Calculate timer load value for slipping
	ldy	timer_stable_value+1
	ldx	timer_stable_value
	bne	soc_skp1
	dey
soc_skp1:
	dex
; X/Y = timer slide value

	lda	#$30		;BMI <rel>
	bit	raster_lsb
	bpl	soc_skp2
	lda	#$10		;BPL <rel>
soc_skp2:
	sta	SOC_SM_RASTER_LSB
	sta	IS_SM_RASTER_LSB
	
	lda	raster_line
soc_lp1:
	cmp	$9004
	beq	soc_lp1
soc_lp2:
	cmp	$9004
	bne	soc_lp2
soc_lp3:
	bit	$9003
SOC_SM_RASTER_LSB	equ	.
	bmi	soc_lp3
	stx	$9124
	sty	$9125
	lda	#%11000000
	sta	$912e
	cli
	
soc_lp4:
	if	0
; be evil to timing to provoke glitches
	inx
	bpl	soc_lp4
	inc	$4080,x
	dec	$4080,x
	endif
	ifconst	HAVE_TEST_CONTROLLER
	jsr	test_controller
	endif
	ifconst	HAVE_TEST_RESULT
	lda	test_done
	beq	soc_lp4
	sei
	jsr	$fd52
	jsr	$fdf9
	jsr	test_result
soc_lp5:
	jmp	soc_lp5
	else
	jmp	soc_lp4
	endif

cycles_per_line:
	dc.b	0
num_lines:
	dc.b	0
cycles_per_frame:
	dc.w	0
timer_stable_value:
	dc.w	0
adjust:
	dc.b	0
	ifconst HAVE_TEST_RESULT
test_done:
	dc.b	0
	endif
raster_line:
	dc.b	0
raster_lsb:
	dc.b	0
	
;**************************************************************************
;*
;* NAME  irq_stable
;*   
;******
TIMING_ADJUST	equ	48
irq_stable:
	lda	adjust
	sec
	sbc	$9124		; ack and adjust
	sta	is_time
	clv
is_time	equ	.+1
	bvc	is_time
	dc.b	$a2,$a2,$a2,$a2,$a2,$a2,$a2,$a2,$a2,$a2,$24,$ea

	bit	$9003
IS_SM_RASTER_LSB	equ	.
	bmi	is_ex1

	lda	timer_stable_value
	sta	$9126
	lda	timer_stable_value+1
	sta	$9127

	jsr	test_perform

	jmp	is_ex2

is_ex1:
	lda	$900f
	eor	#$a5
	sta	$900f
	eor	#$a5
	sta	$900f
is_ex2:
	jmp	$eb18


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
	lda	#%01000000
	sta	$912b
;--- wait vb
ct_lp1:
	lda	$9004
	beq	ct_lp1
ct_lp2:
	lda	$9004
	bne	ct_lp2
;--- raster line 0
	lda	#$fe
	sta	$9124
	sta	$9125		; load timer with $fefe
;	lda	#%00010001
;	sta	$dc0e		; start one shot timer
ct_lp3:
	bit	$9004
	bpl	ct_lp3
;--- raster line 256
	lda	$9125		; timer msb
	eor	#$ff		; invert
; Acc = cycles per line
;--- scan for raster wrap
	pha

ct_lp4:
	ldy	$9003
	ldx	$9004
ct_lp5:
	tya
	ldy	$9003
	cpx	$9004
	beq	ct_lp5
	inx
	cpx	$9004
	beq	ct_lp4
	dex
	asl
	txa
	rol
	tax
	inx

	pla
; X = number of raster lines (LSB)

twelve:
	rts

;**************************************************************************
;*
;* NAME  calc_frame_time
;*   
;* DESCRIPTION
;*   Calculate the number of cycles per frame
;*   
;******
calc_frame_time:
	lda	#0
	tay
	ldx	cycles_per_line
cft_lp1:
	clc
	adc	num_lines
	bcc	cft_skp1
	iny
cft_skp1:
	iny
	dex
	bne	cft_lp1
	sta	cycles_per_frame
	sty	cycles_per_frame+1
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
MAX_GUARD_CYCLES	equ	71
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
	lda	#%01000000
	sta	$911b
	ldy	cycles_per_line
	dey
	dey
	sty	$9114		;T2 low order latch
	stx	$9115		;T2 High order latch (loads and starts)

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
	iny
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

; eof
