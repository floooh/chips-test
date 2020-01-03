*=$1201

!byte	$0b,$12,$00,$00,$9e,$34,$36,$32,$31,$00,$00,$00

; setup test-screen (2 x Char 0-255)
	ldx	#$00
	ldy	#$00
loop	tya
	sta	$1000,y
	sta	$1100,y
	lda	#$00
	sta	$9400,y
	sta	$9500,y
	iny
	bne	loop
	
;synchronize with the screen
	sei
	ldx	#$2e	; make sure next command really STARTS at line #$0b and not SOMEWHERE in it
raster0	cpx	$9004
	bne	raster0
start	ldx	#$30	; wait for this raster line (times 2)
raster1	cpx	$9004
	bne	raster1	; 3 cycles - at this stage, the inaccuracy is 7 clock cycles
			;            the processor is in this place 2 to 9 cycles
			;            after $9004 has changed
	ldy	#9	; 2 cycles
	bit	$24	; 3 cycles
raster2
	ldx	$9004	; 4 cycles	
	txa		; 2 cycles
	bit	$24	; 3 cycles
	ldx	#24	; 2 cycles
minus11	dex		; 2 cycles * 24 = 48 cycles
	bne	minus11	; 3 cycles * 24 - 1 = 71 cycles - first spend some time (so that the whole
	cmp	$9004	; 4 cycles                        loop will be 2 raster lines)
	bcs	plus21	; 3 cycles (-1 on when carry clear) save one cycle if $9004 changed too late
plus21	dey		; 2 cycles
	bne	raster2	; 3 cycles (2 beim letzten)
			; now it is fully synchronized
			; 6 cycles have passed since last $9004 change

!byte	$a9	; lda #
test
!byte	$00	; value to test at $9000

	sta	$9000	; set $9000 at cycle 13
	sta	$1000	; show on screen
	ldy	#$00	; wait loop (about 1285 cycles)
branch	iny
	bne	branch

	lda	#$0c	; reset $9000 at cycle 26
	sta	$9000	; to original value (decimal 12)

	lda	#$ef	; check for SPACE-key
	sta	$9120
	lda	$9121
	cmp	#$fe
	bne	start	; continue display-loop until SPACE pressed

	inc	test	; incease value $9000 is going to be set to
	bmi	end	; If value has reached 128 exit

	ldy	#$00	; wait about second to avoid next keypress registering too fast
waity	ldx	#$00
waitx	inx
	bne	waitx
	iny
	bne	waity
	
	jmp	start
	
end	lda	#$00
	sta	test
	rts
