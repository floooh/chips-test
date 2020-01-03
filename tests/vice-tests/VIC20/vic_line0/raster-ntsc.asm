*=$0334
seed	= $1fff

	sei
rasteri	lda	#$83	; wait for line 131 only in interlace-mode
	cmp	$9004
	bne	rasteri
	sei
	ldy	#$0c
start	ldx 	#$36	; counter wait for 48 rasterlines to show effect on visible area
	lda	#$00	; rasterline to check
raster	cmp	$9004
	bne	raster
wait	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	dex
	bne	wait	; wait 65 cycles (one NTSC-rasterline)
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	bit	$24
	lda	#$1b
	sty	$900f	; do raster-effect
	sta	$900f	; show for some cycles, then restore

	lda	seed
       	beq	doEor
	asl
	bcc	noEor
doEor	eor	#$1d
noEor	sta	seed
	tax
wait2	dex		; wait random amount of cycles
	bne	wait2

	lda	#$ef
	sta	$9120
	lda	$9121
	cmp	#$fe
	bne	start
	lda	#$f7
	sta	$9120
	cli
	lda	#$00	
	sta	$c6	; clear keyboard buffer
	rts
