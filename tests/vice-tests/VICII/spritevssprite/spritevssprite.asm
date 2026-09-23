
	.code

	.word	basicLoader
basicLoader:
	; 2019 SYS(2080):PW.SOFT.
	.byte	$18, $08, $e3, $07, $9e, $28, $32, $30
	.byte	$38, $30, $29, $3a, $8f, $20, $50, $57
	.byte	$2E, $53, $4F, $46, $54, $2E, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00

	.code
	jmp main

spriteAnum = 0
spriteBnum = 1

SPRITEPOS = $0340
VRAM = $0400
SPRBASE0 = VRAM+1016
SPRBASE1 = VRAM+1017
SPRBASE2 = VRAM+1018
SPRBASE3 = VRAM+1019
SPRBASE4 = VRAM+1020
SPRBASE5 = VRAM+1021
SPRBASE6 = VRAM+1022
SPRBASE7 = VRAM+1023

    .macro cycles nr_cycles
		.if ((nr_cycles .mod 2) = 1)
			cmp	$ff
			.repeat ((nr_cycles-3) / 2)
				cld
			.endrep
		.else
			.repeat (nr_cycles / 2)
				cld
			.endrep
		.endif
	.endmacro
	
	.macro test_mmc yval,res_addr
		lda	#yval
		sta	$d011
		lda	$d01e ; Mob 2 Mob collision
		lda	$d01e ; Mob 2 Mob collision
		sta	res_addr,x
		inc	$d021 ; to show stableness
		cycles 31
	.endmacro

stable:
	cmp $d012
	bne	stable
	ldx	#$0B
	nop
	nop
	nop
;	nop
stable1:
	inc	$d020
	lda	$d012
	dec	$d020
	lda	$d012
	lda	$d012
	lda	$d012
	lda	$d012
	lda	$d012
	lda	$d012
	inc	$ff
	nop
	nop
	nop
	cmp	$d012
	beq	stable2
stable2:
	dex
	bne stable1
	rts

;------------------------------------------------------------------------------
main:
	jsr	init_sprite
	jsr	init_colorram

;testagain:
	lda	#$00
	sta	test_pos
	sta passfail
	lda #$CC
	sta $3FFF

	lda	#SPRITEPOS/64
	sta	SPRBASE0 + spriteAnum
	sta	SPRBASE0 + spriteBnum
	lda	#(1 << spriteAnum) | (1 << spriteBnum)
	sta	$d015

	; both sprites at exact same position
	lda	#60
	sta	$D000 + (spriteAnum * 2)
	sta	$D000 + (spriteBnum * 2)
	lda	#50
	sta	$D001 + (spriteAnum * 2)
	sta	$D001 + (spriteBnum * 2)

	lda #1
	sta $D027 + spriteAnum
	lda #7
	sta $D027 + spriteBnum

main_loop:

	sei
	lda	#38
	jsr	stable
;	cycles 63
	cycles 39
	
	inc	$d020
	lda	test_pos
	tax
	clc
	adc	#60
	; both sprites at exact same position
	sta	$D000 + (spriteAnum * 2)
	sta	$D000 + (spriteBnum * 2)

	test_mmc $10,VRAM+ 0*40
	test_mmc $11,VRAM+ 1*40
	test_mmc $12,VRAM+ 2*40
	test_mmc $13,VRAM+ 3*40
	test_mmc $14,VRAM+ 4*40
	test_mmc $15,VRAM+ 5*40
	test_mmc $16,VRAM+ 6*40
	test_mmc $17,VRAM+ 7*40

	test_mmc $10,VRAM+ 8*40
	test_mmc $11,VRAM+ 9*40
	test_mmc $12,VRAM+10*40
	test_mmc $13,VRAM+11*40
	test_mmc $14,VRAM+12*40
	test_mmc $15,VRAM+13*40
	test_mmc $16,VRAM+14*40
	test_mmc $17,VRAM+15*40

	test_mmc $10,VRAM+16*40
	test_mmc $11,VRAM+17*40
	test_mmc $12,VRAM+18*40
	test_mmc $13,VRAM+19*40
	
	lda	#$00
	sta	$d020
	sta	$d021
	
	lda cycle_nrs,x
	sta	VRAM+20*40,x
	
	ldx	test_pos
	inx
	cpx	#40
	beq	verify
verify_end:
	stx	test_pos

rescolor=*+1
	lda	#$00
	sta	$d020

	; ========= Lower  Border =========
	; Use $D011 register to detect start of new frame
	;
raster_end:
	lda	$D011
	and	#$80
	beq	raster_end
raster_begin:
	lda	$D011
	and	#$80
	bne	raster_begin

	lda	#$00
	sta	$d020
	sta	$d021

	jmp	main_loop

;--------------------------------
	
verify:

    lda $d020
    pha
    lda #$0b
    sta $d020

verify_state=*+1
    lda #0
    bne verify2

    ; we need to check 20 lines, 5 lines per "packet"
	ldx #0
	stx rescolor
	stx passfail

verify_loop1:
	lda #$05
	sta $D800+(0*5*40),x
	lda VRAM+(0*5*40),x
	cmp expected_result+(0*5*40),x
	beq verify_ok0
	lda #$02
	sta $D800+(0*5*40),x
	lda #$ff
	sta passfail
verify_ok0:

	lda #$05
	sta $D800+(1*5*40),x
	lda VRAM+(1*5*40),x
	cmp expected_result+(1*5*40),x
	beq verify_ok1
	lda #$02
	sta $D800+(1*5*40),x
	lda #$ff
	sta passfail
verify_ok1:

	inx
	cpx #(5*40)
	bne verify_loop1

	; Output results
	ldx #$0b
	lda passfail
	beq noerror2
	ldx #$0a
noerror2:
	stx rescolor

    pla
    sta $d020
    ldx #1
    stx verify_state
    ldx #39
    jmp verify_end

verify2:
	ldx #0
verify_loop2:

	lda #$05
	sta $D800+(2*5*40),x
	lda VRAM+(2*5*40),x
	cmp expected_result+(2*5*40),x
	beq verify_ok2
	lda #$02
	sta $D800+(2*5*40),x
	lda #$ff
	sta passfail
verify_ok2:

	lda #$05
	sta $D800+(3*5*40),x
	lda VRAM+(3*5*40),x
	cmp expected_result+(3*5*40),x
	beq verify_ok_last
	lda #$02
	sta $D800+(3*5*40),x
	lda #$ff
	sta passfail

verify_ok_last:
	inx
	cpx #(5*40)
	bne verify_loop2
	

    pla
    sta $d020

	; Output results
	ldx #$05
	lda passfail
	sta $d7ff
	beq noerror
	ldx #$02
noerror:
	stx rescolor

    ldx #0
    stx verify_state
    jmp verify_end

test_pos:
	.byte	$00
	
passfail:
	.byte	$00
	
cycle_nrs:
	.byte	$30,$31,$32,$33,$34,$35,$36,$37,$38,$39
	.byte	$30,$31,$32,$33,$34,$35,$36,$37,$38,$39
	.byte	$30,$31,$32,$33,$34,$35,$36,$37,$38,$39
	.byte	$30,$31,$32,$33,$34,$35,$36,$37,$38,$39
	
; sprite data, diagonal bar (\)
sprite_bytes:
	.byte	$80,$00,$00
	.byte	$40,$00,$00
	.byte	$20,$00,$00
	.byte	$10,$00,$00
	.byte	$08,$00,$00
	.byte	$04,$00,$00
	.byte	$02,$00,$00
	.byte	$01,$00,$00
	.byte	$00,$80,$00
	.byte	$00,$40,$00
	.byte	$00,$20,$00
	.byte	$00,$10,$00
	.byte	$00,$08,$00
	.byte	$00,$04,$00
	.byte	$00,$02,$00
	.byte	$00,$01,$00
	.byte	$00,$00,$80
	.byte	$00,$00,$40
	.byte	$00,$00,$20
	.byte	$00,$00,$10
	.byte	$00,$00,$08

init_sprite:
	ldx	#$00
init_sprite_1:
	lda	sprite_bytes,x
	sta	SPRITEPOS,x
	inx
	bne	init_sprite_1
	rts

init_colorram:
	ldx	#$00
	lda	#$0F
init_colorram_1:
	sta	$D800,x
	sta	$D900,x	
	sta	$DA00,x	
	sta	$DB00,x	
	inx
	bne	init_colorram_1
	rts
	
expected_result:
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$03
	.byte	$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$03,$03
	.byte	$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$03,$03,$03
	.byte	$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$03,$03,$03,$03
	.byte	$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$03,$03,$03,$03,$03
	.byte	$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$03,$03,$03,$03,$03,$03
	.byte	$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$03,$03,$03,$03,$03,$03,$03
	.byte	$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$03,$03,$03,$03,$03,$03,$03,$03
	.byte	$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$03,$03,$03,$03,$03,$03,$03,$03,$03
	.byte	$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03
	.byte	$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03
	.byte	$03,$03,$03,$03,$03,$03,$03,$03,$03,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03
	.byte	$03,$03,$03,$03,$03,$03,$03,$03,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03
	.byte	$03,$03,$03,$03,$03,$03,$03,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03,$03
	.byte	$03,$03,$03,$03,$03,$03,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
