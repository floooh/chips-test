
	.include "basicloader.asm"
	.code
	jmp main
	
	.include "vicii_reg_timing_gfx.asm"
	.code

DEBUGSTABLE = 0	
	
REGENPOS = $0400
SPRBASE0 = REGENPOS+1016
SPRBASE1 = REGENPOS+1017
SPRBASE2 = REGENPOS+1018
SPRBASE3 = REGENPOS+1019
SPRBASE4 = REGENPOS+1020
SPRBASE5 = REGENPOS+1021
SPRBASE6 = REGENPOS+1022
SPRBASE7 = REGENPOS+1023
INCADDR = 2023
MX8     = $D010  ; Mob 8th bit
YSCROLL = $D011
RASTER  = $D012
ME      = $D015  ; Mob enable
XSCROLL = $D016
MYE     = $D017  ; Mob Y-expand
MEMPTR  = $D018
MMC     = $D01C  ; Mob multicolor mode
MXE     = $D01D  ; Mob X-expand
EC      = $D020  ; Exterior color
BC0     = $D021  ; Background color #0
BC1     = $D022  ; Background color #1
BC2     = $D023  ; Background color #2
BC3     = $D024  ; Background color #3
MM0     = $D025  ; Mob multicolor #0
MM1     = $D026  ; Mob multicolor #1
MC0     = $D027  ; Mob color #0
MC1     = $D028  ; Mob color #1
MC2     = $D029  ; Mob color #2
MC3     = $D02A  ; Mob color #3
MC4     = $D02B  ; Mob color #4
MC5     = $D02C  ; Mob color #5
MC6     = $D02D  ; Mob color #6
MC7     = $D02E  ; Mob color #7

CHARPOS  = $3000
SPRITE1  = $3240
SPRITE2  = $3280
SPRITE3  = $32C0

YSCROLL_NORM = $1B ; Normal yscroll value
YSCROLL_ECM = $5B ; Extended color mode

TEXT_COL = 155
ALT_MOB_COL = 11   ; Different color from the test color to visualize the sprite boundaries
EC_COL = 11
TEST_COL = 8
;TEST_COL = 4

SPRITEX_POS  = 56
SPRITEX_POS2 = 130
SPRITEX_POS3 = 200
SPRITEY_POS = 91

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

	.macro testtext color, incdec
		; Clear A register
		lda	#0
		nop
		nop
		sty	color
		sta	color
		stx	XSCROLL
		sta	XSCROLL
		incdec
		nop
		; Set XSCROLL to 40 columns
		lda	#8
		sta	XSCROLL
		; Clear A again
		lda	#0
		; Set background in side border
		sty	color
		; Open side border
		dec	XSCROLL
		sta	color
		; Restore x-scroll register
		lda	#8
		sta	XSCROLL
		lda	#EC_COL
		sta	EC
		cmp	$ff
	.endmacro
	
	.macro test_border_sta reg,v1,v2, color, x_value
		nop
		lda	#v1
		ldx #v2
		sta	reg
		stx	reg
		; Waste cycles
		nop
		nop
		nop		
		nop
		nop
		nop
		lda	#0
		ldx	#x_value
		; Set XSCROLL to 40 columns		
		stx	XSCROLL
		; Set background in side border
		sty	color
		nop
		; Open side border
		sta	XSCROLL
		sta	color
		; Restore x-scroll register
		lda	#8
		sta	XSCROLL
		lda	#EC_COL
		sta	EC
		cmp	$ff
	.endmacro

	.macro test_border_rmw reg,v1,v2, opcode, color, x_value
		nop
		lda	#v1
		ldx #v2
		sta	reg
		stx	reg
		; Waste cycles
		nop
		nop		
		nop
		nop
		nop
		nop
		lda	#0
		ldx	#x_value
		; Set XSCROLL to 40 columns
		stx	XSCROLL
		; Set background in side border
		sty	color
		; Open side border
		opcode XSCROLL
		sta	color
		; Restore x-scroll register
		lda	#8
		sta	XSCROLL
		lda	#EC_COL
		sta	EC
		cmp	$ff
	.endmacro
	
	.macro test_sprite color, stolen
		lda	#ALT_MOB_COL
		nop
		nop
		sty	color
		sta	color
		cycles (49-stolen)
	.endmacro

	.macro test_sprite_xpos color, stolen
		lda	#ALT_MOB_COL
		nop
		nop
		sty	color
		sta	color
		stx	$D00E
		inx                 ; inx is here between STA. As we shift over 21 positions extra cycles between STAs are needed.
		lda #SPRITEX_POS3   ; together with this load there are 4 extra cycles between the STA opcodes.
		sta $D00E
		cycles (37-stolen)
	.endmacro

	.macro test_sprite_expand color, stolen, v1, v2
		lda	#ALT_MOB_COL
		ldx	#v2
		nop
		sty	color
		sta	color
		stx MXE
		nop
		inc	$D00E
		ldx	#v1
		stx MXE
		cycles (31-stolen)
	.endmacro

stable:
	cmp RASTER
	bne	stable
	ldx	#$0B
	nop
	nop
	nop
;	nop
stable1:
.if DEBUGSTABLE = 1
	inc	EC
	lda	RASTER
	dec	EC
.else 
	inc	INCADDR+1
	lda	RASTER
	dec	INCADDR+1
.endif
	lda	RASTER
	lda	RASTER
	lda	RASTER
	lda	RASTER
	lda	RASTER
	lda	RASTER
	inc	$ff
	nop
	nop
	nop
	cmp	RASTER
	beq	stable2
stable2:
	dex
	bne stable1
	rts

main:
	; Set position of regen-buffer and character definitions
	lda	#(REGENPOS/1024)*16 + (CHARPOS/1024)
	sta	MEMPTR

	jsr	welcome
	jsr	sprites
	jsr bitmap
again:
	sei

	; ============ Begin Frame ============
	; Prepare next test round
	;
	
	; Give sprite 7 contrasting color for x-pos and x-expand tests.
	lda	#TEST_COL
	sta	MC7
	; Use SPRITE3 definition for the x-pos and x-expand tests
	lda	#(SPRITE3/64)
	sta	SPRBASE7
	lda	#$80
	sta	MYE




	lda	#32
	jsr	stable
	
	cycles 28

	; Enable extended color mode
	lda	#YSCROLL_ECM
	sta	YSCROLL

	; Disable sprite multicolor
	lda	#$00
	sta	MMC

	lda	#0
	sta	BC1
	sta	BC2
	sta	BC3
	ldx	#$08
	stx	XSCROLL
	ldy #TEST_COL

	; ============ Line -1 ============
	; Display Exterior color timing.
	; Open side-border and show EC is displayed there.
	;
	testtext EC, nop
	testtext EC, nop
	testtext EC, nop
	testtext EC, nop
	testtext EC, nop
	testtext EC, nop
	testtext EC, nop

	; ============ Line  0 ============
	; Display Background #0 timing.
	; Open side-border and show BC0 is displayed there (continuation of last character bg).
	;
	jsr	badline
	testtext BC0, inx
	testtext BC0, inx
	testtext BC0, inx
	testtext BC0, inx
	testtext BC0, inx
	testtext BC0, inx
	testtext BC0, inx

	; ============ Line  1 ============
	; Display Background #1 timing.
	; Display timing of XSCROLL change in middle of line (increasing xscroll value each line). 
	; Open side-border and show BC1 is displayed there (continuation of last character bg).
	;
	jsr	badline
	testtext BC1, inx
	testtext BC1, inx
	testtext BC1, inx
	testtext BC1, inx
	testtext BC1, inx
	testtext BC1, inx
	testtext BC1, inx

	; ============ Line  2 ============
	; Display Background #2 timing.
	; Display timing of XSCROLL change in middle of line (decreasing xscroll value each line). 
	; Open side-border and show BC2 is displayed there (continuation of last character bg).
	;
	jsr	badline
	testtext BC2, dex
	testtext BC2, dex
	testtext BC2, dex
	testtext BC2, dex
	testtext BC2, dex
	testtext BC2, dex
	testtext BC2, dex

	; ============ Line  3 ============
	; Display Background #3 timing.
	; Display timing of XSCROLL change in middle of line (decreasing xscroll value each line). 
	; Open side-border and show BC3 is displayed there (continuation of last character bg).
	;
	jsr	badline
	testtext BC3, dex
	testtext BC3, dex
	testtext BC3, dex
	testtext BC3, dex
	testtext BC3, dex
	testtext BC3, dex
	testtext BC3, dex
	
	; ============ Line  4 ============
	; Information line 0123456.. no tests performed
	;
	jsr	badline
	; Prepare sprite position test
	lda #$FF
	sta $D00E

	; Position sprite 7 on same height as 0
	lda #SPRITEY_POS
	sta	$D00F
	
	ldx	#86
line4:
	dex
	bne	line4

	ldx #SPRITEX_POS2
	cycles 6
	
	; ============ Line  5 ============
	; Display sprite #0 color timing.
	; Display sprite #7 xposition timing.
	;
	test_sprite_xpos MC0, 10
	test_sprite_xpos MC0, 10
	test_sprite_xpos MC0, 10
	test_sprite_xpos MC0, 10
	test_sprite_xpos MC0, 10
	test_sprite_xpos MC0, 10
	test_sprite_xpos MC0, 10

	; ============ Line  6 ============
	; Display sprite #1 color timing.
	; Display sprite #7 xposition timing.
	;
	cycles 8
	test_sprite_xpos MC1, 12
	test_sprite_xpos MC1, 12
	test_sprite_xpos MC1, 12
	test_sprite_xpos MC1, 12
	test_sprite_xpos MC1, 12
	test_sprite_xpos MC1, 12
	test_sprite_xpos MC1, 12

	; ============ Line  7 ============
	; Display sprite #2 color timing.
	; Display sprite #7 xposition timing.
	;
	cycles 6
	test_sprite_xpos MC2, 14
	test_sprite_xpos MC2, 14
	test_sprite_xpos MC2, 14
	test_sprite_xpos MC2, 14
	test_sprite_xpos MC2, 12
	test_sprite MC2, 24
	; Prepare sprite 7 for x-expand test
	ldx #SPRITEX_POS2-8
	stx	$D00E
	ldx	#$7F
	stx	MXE
	nop
	test_sprite MC2, 6

	; ============ Line  8 ============
	; Display sprite #3 color timing.
	; Display sprite #7 X-expansion timing.
	;
	test_sprite_expand MC3, 14, $7F, $FF
	test_sprite_expand MC3, 14, $7F, $FF
	test_sprite_expand MC3, 14, $7F, $FF
	test_sprite_expand MC3, 14, $7F, $FF
	test_sprite_expand MC3, 12, $7F, $FF
	test_sprite_expand MC3, 12, $7F, $FF
	test_sprite_expand MC3, 12, $7F, $FF

	; ============ Line  9 ============
	; Display sprite #4 color timing.
	; Display sprite #7 X-expansion timing.
	;
;	cycles 6
	ldx #SPRITEX_POS2-8
	stx	$D00E
	test_sprite_expand MC4, 14, $FF, $7F
	test_sprite_expand MC4, 14, $FF, $7F
	test_sprite_expand MC4, 14, $FF, $7F
	test_sprite_expand MC4, 14, $FF, $7F
	test_sprite_expand MC4, 12, $FF, $7F
	test_sprite_expand MC4, 12, $FF, $7F
	test_sprite_expand MC4, 12, $FF, $7F

	; ============ Line 10 ============
	; Display sprite #5 color timing.
	; Display sprite #7 X-expansion timing.
	;
	cycles 7
;	ldx #SPRITEX_POS2-8
;	stx	$D00E
	test_sprite MC5, 13
	test_sprite MC5, 9
	test_sprite MC5, 9
	test_sprite MC5, 9
	test_sprite MC5, 7
	test_sprite MC5, (7+30)
	; Prepare sprite 7 for multicolor test
	lda	#(SPRITE2/64)
	sta	REGENPOS+1023
	ldx	#$FF
	stx	MXE
	lda	#$00
	sta	MYE
	ldx	#$80
	stx	MMC
	lda #ALT_MOB_COL
	sta MC7
	test_sprite MC5, 7

	; ============ Line 11 ============
	; Display sprite #6 color timing.
	;
	
	; Position sprite 7 below 6
	ldx #SPRITEY_POS+56
	stx	$D00F
	cycles 5
	test_sprite MC6, 9
	test_sprite MC6, 9
	test_sprite MC6, 9
	test_sprite MC6, 9
	test_sprite MC6, 7
	test_sprite MC6, 7
	test_sprite MC6, 7
	
	; ============ Line 12 ============
	; Display sprite #7 color timing (multicolor).
	;

	; Restore horizontal position of sprite 7
	ldx #SPRITEX_POS
	stx	$D00E
	cycles 5
	test_sprite MC7, 9
	test_sprite MC7, 9
	test_sprite MC7, 9
	test_sprite MC7, 9
	test_sprite MC7, 7
	test_sprite MC7, 7
	test_sprite MC7, 7

	; ============ Line 13 ============
	; Display sprite multicolor #0 color timing (multicolor).
	;
	cycles 13
	test_sprite MM0, 7
	test_sprite MM0, 7
	test_sprite MM0, 7
	test_sprite MM0, 7
	test_sprite MM0, 5
	test_sprite MM0, 5
	test_sprite MM0, 5

	; ============ Line 14 ============
	; Display sprite multicolor #0 color timing (multicolor).
	;
	cycles 15
	test_sprite MM1, 5
	test_sprite MM1, 5
	test_sprite MM1, 5
	test_sprite MM1, 5
	test_sprite MM1, 5
	test_sprite MM1, 5
	test_sprite MM1, 5
	
	cycles 23
	; Prepare color registers
	ldx	#TEST_COL
	stx	BC1
	; Disable ECM for next 3 tests
	ldx	#YSCROLL_NORM
	stx YSCROLL

	; ============ Line 15 ============
	; Open side-border using STA opcode.
	; Each scanline starting with a different X-Scroll value.
	;
	test_border_sta BC0, TEST_COL, 0,  BC0, 9
	test_border_sta BC0, TEST_COL, 0,  BC0, 10
	test_border_sta BC0, TEST_COL, 0,  BC0, 11
	test_border_sta BC0, TEST_COL, 0,  BC0, 12
	test_border_sta BC0, TEST_COL, 0,  BC0, 13
	test_border_sta BC0, TEST_COL, 0,  BC0, 14
	test_border_sta BC0, TEST_COL, 0,  BC0, 15
	jsr	badline

	; ============ Line 16 ============
	; Open side-border using DEC opcode.
	; Each scanline starting with X-Scroll value of 8.
	;
	test_border_rmw XSCROLL, $18, 8,  dec, BC0, 8
	test_border_rmw XSCROLL, $18, 8,  dec, BC0, 8
	test_border_rmw XSCROLL, $18, 8,  dec, BC0, 8
	test_border_rmw XSCROLL, $18, 8,  dec, BC0, 8
	test_border_rmw XSCROLL, $18, 8,  dec, BC0, 8
	test_border_rmw XSCROLL, $18, 8,  dec, BC0, 8
	test_border_rmw XSCROLL, $18, 8,  dec, BC0, 8
	jsr	badline

	; ============ Line 17 ============
	; Open side-border using INC opcode.
	; Each scanline starting with a X-Scroll value of 15.
	;
	test_border_rmw YSCROLL, $3B, YSCROLL_NORM,  inc, BC0, 15
	test_border_rmw YSCROLL, $3B, YSCROLL_NORM,  inc, BC0, 15
	test_border_rmw YSCROLL, $3B, YSCROLL_NORM,  inc, BC0, 15
	test_border_rmw YSCROLL, $3B, YSCROLL_NORM,  inc, BC0, 15
	test_border_rmw YSCROLL, $3B, YSCROLL_NORM,  inc, BC0, 15
	test_border_rmw YSCROLL, $3B, YSCROLL_NORM,  inc, BC0, 15
	test_border_rmw YSCROLL, $3B, YSCROLL_NORM,  inc, BC0, 15
	jsr	badline

	; ============ Line 18 ============
	; Open side-border using ROR opcode.
	; Each scanline starting with a different X-Scroll value.
	;
	test_border_rmw YSCROLL, YSCROLL_ECM, YSCROLL_NORM,  ror, BC0, 9
	test_border_rmw YSCROLL, YSCROLL_ECM, YSCROLL_NORM,  ror, BC0, 10
	test_border_rmw YSCROLL, YSCROLL_ECM, YSCROLL_NORM,  ror, BC0, 11
	test_border_rmw YSCROLL, YSCROLL_ECM, YSCROLL_NORM,  ror, BC0, 12
	test_border_rmw YSCROLL, YSCROLL_ECM, YSCROLL_NORM,  ror, BC0, 13
	test_border_rmw YSCROLL, YSCROLL_ECM, YSCROLL_NORM,  ror, BC0, 14
	test_border_rmw YSCROLL, YSCROLL_ECM, YSCROLL_NORM,  ror, BC0, 15
	jsr	badline

	; ============ Line 19 ============
	; Open side-border using LSR opcode.
	; Each scanline starting with a different X-Scroll value.
	;
	test_border_rmw YSCROLL, $7B, YSCROLL_NORM,  lsr, BC0, 9
	test_border_rmw YSCROLL, $7B, YSCROLL_NORM,  lsr, BC0, 10
	test_border_rmw YSCROLL, $7B, YSCROLL_NORM,  lsr, BC0, 11
	test_border_rmw YSCROLL, $7B, YSCROLL_NORM,  lsr, BC0, 12
	test_border_rmw YSCROLL, $7B, YSCROLL_NORM,  lsr, BC0, 13
	test_border_rmw YSCROLL, $7B, YSCROLL_NORM,  lsr, BC0, 14
	test_border_rmw YSCROLL, $7B, YSCROLL_NORM,  lsr, BC0, 15
	jsr	badline

	; ============ Line 20 ============
	; Open side-border using ROR opcode.
	; Each scanline starting with a different X-Scroll value (with bit #3 set).
	;
	test_border_rmw BC0, TEST_COL, 0,  rol, BC0, 9
	test_border_rmw BC0, TEST_COL, 0,  rol, BC0, 9
	test_border_rmw BC0, TEST_COL, 0,  rol, BC0, 10
	test_border_rmw BC0, TEST_COL, 0,  rol, BC0, 10
	test_border_rmw BC0, TEST_COL, 0,  rol, BC0, 11
	test_border_rmw BC0, TEST_COL, 0,  rol, BC0, 11
	test_border_rmw BC0, TEST_COL, 0,  rol, BC0, 11
	jsr	badline

	; ============ Line 21 ============
	; Open side-border using ASL opcode.
	; Each scanline starting with a different X-Scroll value (with bit #3 set).
	;
	test_border_rmw BC0, TEST_COL, 0,  asl, BC0, 9
	test_border_rmw BC0, TEST_COL, 0,  asl, BC0, 9
	test_border_rmw BC0, TEST_COL, 0,  asl, BC0, 10
	test_border_rmw BC0, TEST_COL, 0,  asl, BC0, 10
	test_border_rmw BC0, TEST_COL, 0,  asl, BC0, 11
	test_border_rmw BC0, TEST_COL, 0,  asl, BC0, 11
	test_border_rmw BC0, TEST_COL, 0,  asl, BC0, 11

	; ============ Line 22 ============
	; Start of line shifted down.
	cycles 115
;	inc	BC0

	; Shift line down
	lda	#$0F
	sta	YSCROLL
	
	; Unused

	; ============ Line 23 ============
	; Unused

	; ============ Line 24 ============
	; Side border is kept open for a few lines at end of character screen.
	; This also keeps bottom border open as long the side border is re-openened each line.
	; However the character are cutoff at start of lower-border and no idle graphics is displayed.
	;
	cycles 141

	lda	#$0F
	sta	YSCROLL
	lda	#0
	sta	XSCROLL

	cycles 320
	cycles 63

;	lda	$DC01
;	and	#$10
;	beq	space_pressed
;   bne not pressed

	
;	cycles 20
;	inc EC
	
	.repeat 8
	lda	#0
	nop
	nop
	nop
	nop
	sta	XSCROLL
	sta	BC0
	lda	#8
	sta	XSCROLL
	cycles 39
	.endrep
	
	cycles 20
	
	.repeat 16
	lda	#0
	ldx	#TEST_COL
	stx	BC0
	nop
	sta	XSCROLL
	nop
	nop
	lda	#8
	sta	XSCROLL
	cycles 39
	.endrep
	
	; Restore background #0
	nop
	lda	#0
	sta	BC0
	
    dec framecount
    bne skp
    lda #0
    sta $d7ff
skp:
	
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
	
	inc	$fe
	bne	jitter
jitter:
	
;	lda	#EC_COL
;	sta EC
	jmp again
	
badline:
	ldx	#$08
	nop
	nop
	nop
rts12:
	rts

welcome:
	ldx	#$00
	lda	#>welcome_str_1
	sta welcome1+2
welcome1:
	lda	welcome_str_1,x
	beq	welcome_color_adj
	jsr	$ffd2
	inx
	bne	welcome1
	inc	welcome1+2
	jmp welcome1
	
; Lines 2,3 and 4 get higher bits set to select different background color.
welcome_color_adj:
	ldx	#40
welcome_color_adj1:
	lda	REGENPOS+40,x
	ora	#$40
	sta	REGENPOS+40,x
	lda	REGENPOS+80,x
	ora	#$80
	sta	REGENPOS+80,x
	lda	REGENPOS+120,x
	ora	#$C0
	sta	REGENPOS+120,x
	dex
	bpl	welcome_color_adj1

	; Special background color for ECM bit test
	ldx	#4
welcome_color_adj2:
	lda	REGENPOS+(18*40),x
	ora	#$40
	sta	REGENPOS+(18*40),x
	inx
	cpx #16
	bne welcome_color_adj2
	rts

framecount: .byte 5

welcome_str_1:
	.byte TEXT_COL, 147
	.byte      "BC0 ", 64, 64, 64, 64, 64, 64, "   ///////                   \"
	.byte      "BC1 ", 64, 64, 64, 64, 64, 64, "   ABCDEFG                   \"
	.byte      "BC2 ", 64, 64, 64, 64, 64, 64, "   ///////                   \"
	.byte      "BC3 ", 64, 64, 64, 64, 64, 64, "   ABCDEFG                   \"
	.byte      "0123456789012345678901234567890123456789"
	.byte      "MC0           \             SPRITE-XPOS", 13
	.byte      "MC1           \             TIMING", 13
	.byte      "MC2           \", 13
	.byte      "MC3         \   \\    0->1", 13
	.byte      "MC4         \   \\    1->0  X-EXPAND", 13
	.byte      "MC5         \   \\", 13
	.byte      "MC6", 13
	.byte      "MC7", 13
	.byte      "MM0", 13
	.byte      "MM1                   OPEN BORDER WITH:", 13
	.byte      "                          STA ABCDEFGHI\"
	.byte      "MCM @@@@@@@@@@@@          DEC ABCDEFGHI\"
	.byte      "BMM \\\\\\\\\\\\          INC ABCDEFGHI\"
	.byte      "ECM @@@@@@@@@@@@          ROR ABCDEFGHI\"
	.byte      "E+B \\\\\\\\\\\\          LSR ABCDEFGHI\"
	.byte      "                          ROL ABCDEFGHI\"
	.byte      "                          ASL ABCDEFGHI\"
	.byte      " \", 13
	.byte      "  VIC-II REGISTER TIMING BY P.WENDRICH",13
	.byte      " \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\"
	.byte 0
	
sprites:
	lda	#(SPRITE1/64)
	sta	SPRBASE0
	sta	SPRBASE1
	sta	SPRBASE2
	sta	SPRBASE3
	sta	SPRBASE4
	sta	SPRBASE5
	sta	SPRBASE6

	lda	#SPRITEX_POS
	sta	$D000
	sta	$D002
	sta	$D004
	sta	$D006
	sta	$D008
	sta	$D00A
	sta	$D00C
	sta	$D00E
	
	lda	#91
	ldx	#0
sprites1:
	sta	$D001,x
	clc
	adc	#$08
	inx
	inx
	cpx	#16
	bne	sprites1
	
	lda	#0
	sta MX8

	; Enable and X-expand all sprites
	lda	#255
	sta	ME
	sta	MXE
	rts

bitmap:
	ldx	#$00
	; bitmap data visible in the BMM and E+B tests
	lda	#BMVALUE
bitmap1:
	sta	$3400,x
	sta	$3500,x
	sta	$3600,x
	sta	$3700,x
	sta	$3800,x
	sta	$3900,x
	sta	$3A00,x
	sta	$3B00,x
	sta	$3C00,x
	sta	$3D00,x
	sta	$3E00,x
    sta	$3F00,x
	inx
	bne	bitmap1
	rts
	

