; Sprite Restart Y-Match
; ======================
; Author: David Horrocks
; Date: 2019-06-22
;
; Notes:
; This test checks that sprite0 (presumably any sprite) can be 
; restarted and displayed by y-matching in cycle 55 but not 
; y-matching cycle 58. This becomes possible when the sprite display
; was already on because the current line is the also the last line 
; of the sprite's previous display run. A very tight sprite collision
; check is used to confirm that sprite0 is restarted and displayed. 

    processor   6502
    
    include "include\vic.asm"
    include "include\cia.asm"
    include "include\sid.asm"
    
    seg    code
    org    $0801
 
SpriteDataAddress equ $2000
SpriteDataEmptyAddress equ $2040
SpriteDataPointAddress equ $2080
framecounter equ $04

raster_start_lsb equ $01
raster_start_msb equ $00
sprite0_def_xpos equ $5c
sprite0y equ $86
sprite1_def_xpos equ $5c
sprite0_def_xpos_msb equ $00
sprite1_def_xpos_msb equ $00
sprite_def_height equ 21
topbordercounter equ $1F
keyrepeatlevel equ $10
SPRITE_MAIN_BIT equ $40
SPRITE_AUX_BIT equ $80
SCREENPOS_XPOS equ $400+40
SCREENPOS_COLLISION equ $405+40
drawptr equ $FA

    subroutine
;* Basic line!
    dc.w .EndLine
    dc.w 0
    dc.b $9e,"2061",0
;        0 SYS2061
.EndLine:
    dc.w 0

    org $080D
    subroutine
Start
    sei
    tsx
    stx $ff
    lda #<nmiroutine
    ldx #>nmiroutine
    sta $fffa
    stx $fffb

    lda #<irqstart
    ldx #>irqstart
    sta $fffe
    stx $ffff
    
    lda #$2f
    sta $00
    lda #%00000101 ; select all RAM and I/O
    sta $01
    
    ; Initalise CIA
    lda #0
    sta CIA1_CRA ; stop timer a
    sta CIA2_CRA ; stop timer b
	lda #$7f       
	sta CIA1_ICR ; disable CIA1 irqs
	sta CIA2_ICR ; disable CIA2 nmis	
	lda CIA1_ICR ; reset CIA1 interrupts
	lda CIA2_ICR ; reset CIA2 interrupts   
        
    ; Initalise VIC
    subroutine
    jsr waitframe
    lda #0
    sta BORDERCOLOUR    
    lda #$0b
    sta SCROLLY ; DEN = 0, RSEL = 1, Y-SCROLL = 3
    lda #$08
    sta SCROLLX ; CSEL = 1, X-SCROLL = 0
    lda #$00
    sta SPRITEENABLE
    lda #0
    sta SPRITEMULTICOLOR
    lda #$00
    sta SPRITEYEXPAND
    lda #$00
    sta SPRITEXEXPAND
    lda #$00
    sta $3fff
    sta $39ff    
    lda #0
    sta framecounter;
        
    subroutine    
    lda #$08
    sta SCROLLX
    lda #$0B ; DEN = 0, RSEL = 1, YSCROLL = 3
    sta SCROLLY
    lda #$15
    sta VICMCR
    lda #6
    sta BACKGROUNDCOLOUR1

    lda #raster_start_lsb
    sta RASTER
    lda SCROLLY
    and #$7f
    ora #raster_start_msb
    sta SCROLLY        

    subroutine
    ldy #0
.1    
    lda #$1
    sta $d800,y
    sta $d900,y
    sta $da00,y
    sta $db00,y
    iny
    bne .1

    subroutine
    ldy #0
.1    
    lda #$20
    sta $0400,y
    sta $0500,y
    sta $0600,y
    sta $0700,y
    iny
    bne .1

    subroutine
    ldx #63
.1    
    lda SpriteData,x
    sta SpriteDataAddress,x
    lda SpriteDataEmpty,x
    sta SpriteDataEmptyAddress,x
    lda SpriteDataPoint,x
    sta SpriteDataPointAddress,x
    dex
    bpl .1

    subroutine
    lda #(SpriteDataEmptyAddress / 64)
    ldx #7
.1    
    sta $07f8,x
    dex
    bpl .1

    subroutine
    lda #(SpriteDataEmptyAddress / 64)
    ldx #0
    sta $07f8,x
    lda #(SpriteDataPointAddress / 64)
    ldx #1
    sta $07f8,x

    subroutine
    ldx #5
    stx SPRITECOLOUR0
    ldx #1
    stx SPRITECOLOUR1
    
    subroutine
    ldx #0
.1
    lda SpritePositions,x
    sta $d000,x   
    lda #sprite0y
    sta $d001,x
    inx
    inx
    cpx #$10
    bne .1
    lda #sprite0_def_xpos_msb + sprite1_def_xpos_msb
    sta SPRITEXMSB

    jsr position_init_sprite0
    jsr position_init_sprite1

    subroutine
    jsr waitframe
    lda SPRITEDATACOL
    lda #$1B ; DEN = 1, RSEL = 1, YSCROLL = 3
    sta SCROLLY
    jsr prep_raster_sprite_y
    lda #$03
    sta SPRITEENABLE    

    lda #0
    sta keyrepeatcounter
    sta ready
        
	lda #$01
	sta VICIER ; enable raster irq        
    inc VICIFR ; clear VIC irqs    
    cli       
subroutine
.loop
    lda ready
    beq .loop
    clc
    lda spritecollision
    cmp #3
    bne .fail
    lda spritecollision+1
    cmp #3
    bne .fail
    lda #5
    sta BORDERCOLOUR
    lda #0
    sta $d7ff
    beq .done
.fail
    lda #2
    sta BORDERCOLOUR
    lda #$FF
    sta $d7ff
.done
    jmp .loop

SpritePositions 
    dc.w sprite0_def_xpos, sprite1_def_xpos, $0,$0,$0,$0,$0,$0

    subroutine
irqstart    
	pha
	txa
	pha
	tya
	pha
	
	lda #<.1
	ldx #>.1
	sta $fffe
	stx $ffff
	inc RASTER	
	inc VICIFR	
	cli
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
    jmp *
    
.1
	pla				;10 we are quick
	pla				;14
	pla				;18
	
    subroutine
	ldx #$5			;22 (5 * x + 1)
.1
	dex
	bne .1
	
	nop
	nop				;50
	nop				;52
	nop				;54
	lda RASTER		;need 56 if quick
	cmp RASTER		;need 60 - 63  if quick
	beq .2	
.2
	
    subroutine
	;do work here
	;cycle=4
	ldx #$1B 		;(5 * x + 1)
	dex
	bne *-1
    nop
    lda #$1E ; DEN = 1, RSEL = 1, YSCROLL = 6
    sta SCROLLY
    lda #sprite0y + sprite_def_height
    sta SPRITE0Y

    subroutine
	ldx #$C9
	dex
	bne *-1
    nop
    nop
    lda #(SpriteDataAddress / 64)
    sta $07f8
    inc BACKGROUNDCOLOUR0
    dec BACKGROUNDCOLOUR0
    lda #sprite0y
    sta SPRITE0Y
    subroutine
	ldx #$3 		;(5 * x + 1)
	dex
	bne *-1
    nop
    nop
    lda SPRITESPRITECOL
    lda SPRITESPRITECOL
    sta spritecollision
    lda #$1B ; DEN = 1, RSEL = 1, YSCROLL = 3
    sta SCROLLY

	ldx #$CA 		;(5 * x + 1)
	dex
	bne *-1
    nop
    nop
    nop
    nop
    
    lda SPRITESPRITECOL
    lda SPRITESPRITECOL
    sta spritecollision + 1
    
    jsr updateinfo
    lda #$ff    
    sta ready
    subroutine
    jsr position_init_sprite0;    
.1
    bit SCROLLY
    bpl .1
    lda #(SpriteDataEmptyAddress / 64)
    ldx #0
    sta $07f8,x
    subroutine
    jsr prep_raster_sprite_y  
	lda #<irqstart
	ldx #>irqstart
	sta $fffe
	stx $ffff
	inc VICIFR
	pla
	tay
	pla
	tax
	pla		
	rti    

    subroutine
updateinfo
    subroutine
    lda #<(SCREENPOS_XPOS+0)
    sta drawptr
    lda #>(SCREENPOS_XPOS+0)
    sta drawptr+1
    lda spritecollision
    jsr drawbytetext

    lda #<(SCREENPOS_XPOS+3)
    sta drawptr
    lda #>(SCREENPOS_XPOS+3)
    sta drawptr+1
    lda spritecollision+1
    jsr drawbytetext
    rts


    ; drawptr: the address to draw
    ; A: the byte
drawbytetext
    subroutine
    pha
    and #$f0
    lsr
    lsr
    lsr
    lsr
    cmp #$a    
    bcs .1
    adc #$30
    bcc .2
.1    
    sbc #9
.2
    ldy #0
    sta (drawptr),y
    subroutine
    pla
    and #$0f
    cmp #$a    
    bcs .1
    adc #$30
    bcc .2
.1    
    sbc #9
.2
    iny
    sta (drawptr),y
    rts
    
    subroutine
prep_raster_sprite_y
    sec
    lda SPRITE0Y
    sbc #2
    sta RASTER
    lda SCROLLY
    and #$7f
    ora #0
    sta SCROLLY
    rts

position_init_sprite0
    lda #sprite0y
    sta SPRITE0Y
    rts

position_init_sprite1
    lda #sprite0y + sprite_def_height
    sta SPRITE1Y
    rts

position_second_sprite0
    lda #sprite0y + sprite_def_height
    sta SPRITE0Y
    rts

nmiroutine
    ldx $ff
    txs
    jmp Start
    
    subroutine
waitframe
.1
    lda SCROLLY
    bpl .1
.2
    lda SCROLLY
    bmi .2
    rts

    subroutine
waitframe100
    lda SCROLLY
    bpl .1
    jsr waitframe    
.1
    lda SCROLLY
    bpl .1
    rts

checkrepeat
    subroutine
    sec
    lda keyrepeatcounter
    beq .1
    cmp #keyrepeatlevel
    bcs .2
.1
    inc keyrepeatcounter
.2
    rts

keyrepeatcounter
    dc.b 0

spritecollision
    dc.b 0, 0, 0

ready 
    dc.b 0

SpriteData
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111
    dc.b %11111111, %11111111 , %11111111


SpriteDataPoint
    dc.b %10000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000001

SpriteDataEmpty
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
    dc.b %00000000, %00000000 , %00000000
