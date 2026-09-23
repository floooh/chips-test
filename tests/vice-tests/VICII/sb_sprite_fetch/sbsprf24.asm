
DOMOVE = 0

;---------------------------------------------------------------------------
        !cpu 6510
;---------------------------------------------------------------------------
        !ct scr
;---------------------------------------------------------------------------
;LABELS        
        GHOSTBYTE        = %00110011  ;will be shown as BYTE_S1
                                      ;if Sprite starts in SB 
                                      ;beyond cycle #58  
                                      ;(X_POS >= $164)
        SB_COL          = $0f
        BG_COL          = $07
        
        SPR_Y_POS           = $7a
        FIRST_OPEN_SB_LINE  = $76
        VIC_BANK            = $03
        GHOSTBYTE_REG       = (4-VIC_BANK)*$4000-1
        D8_COL              = $00
;---------------------------------------------------------------------------
        ;SPR0-2 and SPR7 unused
        SPR3_X          = $1c-$1c
        SPR4_X          = $34-$1c
        SPR5_X          = $4c-$1c
;         SPR6_X          = $64
        VAL_D010        = $78
;---------------------------------------------------------------------------
;SB_DATA_PREFETCH_ON_SB
        BYTE_S0         = %10101100
        BYTE_S1         = %01001101 ;must occur as GHOSTBYTE
                                    ;at sprite read-cycle for
                                    ;for spr6 (1st half-
                                    ;cycle of cycle #7)
        BYTE_S2         = %00100110
        
        MOVEMENT_SPEED_LO   = $80   ;set values to make spr6 move
        MOVEMENT_SPEED_HI   = $00   ;to the left
        
;         ACT_ANIM        = $01   ;activate mini-animation
;                                 ;in the pre-fetch sprite line
;---------------------------------------------------------------------------
;MEM_LOCATION
        x_stack         = $02
        y_stack         = $03
        SCREEN          = $0400
;---------------------------------------------------------------------------
        *= $0801
        !byte $0c, $08, $00, $00, $9e, $20
        !byte $32, $30, $36, $32
        !byte 0,0,0
        *= $080e
start:
        sei
        lda #$34
        sta $01
 
        lda #$20
        ldx #0
-
        sta $400,x
        sta $500,x
        sta $600,x
        sta $700,x
        inx
        bne -

        inc $01
        
        lda #$7f
        sta $dc0d
        sta $dd0d
        lda $dc0d
        lda $dd0d
        
        lda #$00
        sta $d01a
        
        jsr set_timer
        
        lda #$81
        sta $d01a
        
        lda #SB_COL
        sta $d020
        lda #BG_COL
        sta $d021
        
        lda #VIC_BANK
        sta $dd00
        lda #$15
        sta $d018
        
        lda #$78
        sta $d015
        lda #$00
        sta $d01b
        ;lda #$00
        sta $d01c
        
        lda #$00
        sta $d017

        lda #SPR_Y_POS
        sta $d007
        sta $d009
        sta $d00b
        sta $d00d
        
        lda #SPR3_X
        sta $d006
        lda #SPR4_X
        sta $d008
        lda #SPR5_X
        sta $d00a
        lda #SPR6_X
        sta $d00c
        lda #VAL_D010
        sta $d010
        
        lda #$05
        sta $d025
        lda #$06
        sta $d026
        
        lda #$02
        sta $d02a
        lda #$01
        sta $d02b
        lda #$0c
        sta $d02c
        lda #$06
        sta $d02d

        !if ACT_ANIM=1 {
            lda #$80
            sta byte_s0_reg
            lda #$00
            sta byte_s1_reg
            sta byte_s2_reg
            sta direction_reg
            sta .keep_direction+1
        }
        
mark_first_rasline_with_sprites
        ldx #$27
.printscreen_loop
        lda #D8_COL
        sta $d800+8*$28,x
        sta $d800+9*$28,x
        sta $d800+10*$28,x
        sta $d800+11*$28,x
        lda #$20
        sta SCREEN+10*$28,x
        sta SCREEN+11*$28,x
        lda #$63
        sta SCREEN+9*$28,x
        lda textline,x
        sta SCREEN+8*$28,x
        dex
        bpl .printscreen_loop
set_sprite_pt
        ldx #63
.spr_pt_loop    
        txa
        sta SCREEN+$03f8+3-60,x
        dex
        cmp #60
        bne .spr_pt_loop
        
        lda #<frame1_irq
        ldx #>frame1_irq
        sta $fffe
        stx $ffff
        ldy #FIRST_OPEN_SB_LINE+1
        lda #$1b
        sta $d011
        
        cpy $d012
        bne *-3
        dey
        sty $d012
        dec $d019
        dec $01
        cli
.worst_test    
        inc $7000,x
        bne .worst_test
        beq .worst_test
 ;---------------------------------------------------------------------------
        *=  $0a00
;---------------------------------------------------------------------------
set_timer    
        ldx #$3e
        
        lda $d012
        bne *-3         ; hier ist man 2-9 Takte in Rasline 0, also x=0,...,7

        stx $dc04       ; 6 Takte nach Lesen von $d012
        sta $dc05       ;10 Takte nach Lesen von $d012
        ldx #$11        ;12 Takte nach Lesen von $d012
var_kill    
        ldy #$08        ; Schleife hat 9*5 + 1 = 46 Takte
        dey             ; 
        bpl *-1         ;58 Takte(+x) vom letzten Vergleich an
        
        cmp $d012       ;62 Takte(+x)
        ;---------------;
        beq *+2         ; je nach x wird hier ein Takt mehr verbraucht
                        ; 2 Takte(+x) vom letzten $d012-Vergleich an
        
        lda $cf13,y     ; 7 Takte(+x) hier wird $d012 immer in der neuen Rasline gelesen!
        cmp #$08        ; 9 Takte(+x) (beachte y_reg=$ff)
        bne var_kill    ;12 Takte(+x) und die Schleife wird bis ausschliesslich Line 8 durchlaufen 
                        ;
                        ;nach Verlassen der Schleife ist man genau 11 Takte in Line 8
        
        ldy #$03        ;Schleife hat 2*5 + 1 = 11 Takte
        dey             ;
        bne *-1         ;22 Takte in Line 8
        nop             ;24 Takte in Line 8
        
        stx $dc0e       ; Timer startet nach dem 28.Takt in Line 8
        rts
;---------------------------------------------------------------------------
frame1_irq    
        pha
        inc $01
        lda $dc04
        lsr
        bcs .skip1
.skip1        
        asr #$03    ;!by $4b,$03
        bcc .skip2
        bcs .skip2
.skip4        
        bne .end
.skip2        
        bne .skip4
.end        
        stx x_stack
        sty y_stack
        
        ldx #$18
        ldy #$17
        bit $ea
        sty $d016    ;right sb of line $76
        inc $d016    ;last write cycle of INC at cycle #62
        ldx #10
        dex
        bne *-1
        nop
        
        sty $d016    ;right sb line $77
        inc $d016    ;last write cycle of INC at cycle #62
        ldx #10
        dex
        bne *-1
        nop
        
        sty $d016    ;right sb line $78
        inc $d016    ;last write cycle of INC at cycle #62
        ldx #8
        dex
        bne *-1
        
byte_s0_reg = *+1
        lda #BYTE_S0
        sta $d000       ;prepare a VIC reg (here $D000) with BYTE_S0
byte_s1_reg = *+1
        lda #BYTE_S1    ;write BYTE_S1 to GHOSTBYTE register
        sta GHOSTBYTE_REG
        sty $d016       ;right sb line $79
        inc $d016       ;this inc ends @cycle #62 in line $79
        ldy #$00
byte_s2_reg = *+1
        lda #BYTE_S2
        sta $d000,y     ;here the "magic" happens
                        ;[sta abs,y] has cycles R-R-R-R-W
                        ;the cpu processes these cycles during
                        ;the first halfcycles of cycles #4..#8 of line $7a
                        ;Taking VIC-Cycles into account this gives
                        ;#4#5#6#7#8
                        ;RvRvRvRvWv (v=vic access)
                        ;usually the sprite fetch for spr6 is done @cyc #7+#8
                        ;as Sprite DMA is not turned on yet, these cycles will
                        ;still be available for the cpu
                        ;Thus
                        ;#7 halfcycle 1: BYTE_S0 is put on the vic bus
                        ;#7 halfcycle 2: vic reads data from vic bus (=BYTE_S0)
                        ;#8 halfcycle 1: BYTE_S2 is put on vic bus due to W-cycle
                        ;         VIC reads from GHOSTBYTE reg during
                        ;this cpu access (=BYTE_S1)
                        ;#8 halfcycle 2: vic reads data from vic bus (=BYTE_S2)
        bit $ea
        lda #GHOSTBYTE      ;restore GHOSTBYTE (in case a different pattern
        sta GHOSTBYTE_REG   ;in upper/lower border is needed)
        ldx #5
        dex
        bne *-1
        
        ldx #$18
        ldy #$17
        tya
        bit $ea        
        sty $d016           ;right sb line $7a
        stx $d016
        
        sta $d016-$18,x     ;right sb line $7b (BL!)
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $7c
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $7d
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $7e
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $7f
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $80
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $81
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $82
        inc $d016
        
        sta $d016-$17,y    ;right sb line $83 (BL!)
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $84
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $85
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $86
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $87
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $88
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $89
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $8a
        inc $d016
        
        sta $d016-$17,y    ;right sb line $8b (BL!)
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $8c
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $8d
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $8e
        inc $d016
        
        ldx #8
        dex
        bne *-1
        bit $ea
        sty $d016    ;right sb line $8f
        inc $d016
        
        ldx #10
        dex
        bne *-1
        nop
        sty $d016    ;right sb line $90
        inc $d016
        
        ldx #9
        dex
        bne *-1
        ldx #$18
        ldy #$17
        bit $ea
        sty $d016    ;right sb of line $91
        inc $d016

        ldx #9
        dex
        bne *-1
        ldx #$18
        lda #$07
        bit $ea
        sty $d016    ;right sb of line $92
        inc $d016

        nop
        nop
        nop
        nop
        bit $ea
        
        sta $d016-$18,x    ;right sb line $93 (BL!)
        inc $d016
        
        lda #$fa
        cmp $d012
        bne *-3
        lda #$10
        sta $d011
        
        lda #$fe
        cmp $d012
        bne *-3
        lda #$1b
        sta $d011
        
!if DOMOVE = 1 {        
d00c_lobyte = *+1
        lda #$00
        sec
        sbc #MOVEMENT_SPEED_LO
        sta d00c_lobyte
        lda $d00c
        sbc #MOVEMENT_SPEED_HI
        sta $d00c
        bcs .no_overflow
        lda $d010
        eor #$40
        sta $d010
.no_overflow
}
        lda $d00c
        and #$0f
        cmp #$0a
        bcs .no_number
        ;ora #$30
        adc #$3a    ;#$3a includes ora #$30 + "+9" (for compensation of sbc) + 1 
                    ;(as sbc will start with carry cleared)
        ;sec
.no_number
        sbc #$09    ;this is "-$09" or "-$0a" depending on carry set or cleared
        sta SCREEN+8*$28+$27

        lda $d00c
        lsr
        lsr
        lsr
        lsr
        cmp #$0a
        bcs .no_number2
        ;ora #$30
        adc #$3a    ;like above
        ;sec
.no_number2
        sbc #$09    ;like above
        sta SCREEN+8*$28+$26
        
        lda $d010
        and #$40
        beq .below_0x100
        lda #$01
.below_0x100
        ora #$30
        sta SCREEN+8*$28+$25
        
        lda #SPR_Y_POS
        sta $d007
        sta $d009
        sta $d00b
        sta $d00d
;        
        !if ACT_ANIM=1 {
direction_reg = *+1
            lda #$00
            beq .keep_direction
            
            lda .keep_direction+1
            eor #$10
            sta .keep_direction+1
            lda direction_reg
            ldx #$00
.keep_direction
            beq .move_pixel_right
.move_pixel_right
            ror
            ror byte_s0_reg
            ror byte_s1_reg
            ror byte_s2_reg
            ror direction_reg
            jmp .end_pixel_move
.move_pixel_left
            rol
            rol byte_s2_reg
            rol byte_s1_reg
            rol byte_s0_reg
            rol direction_reg            
.end_pixel_move
        }
        lda #<frame1_irq
        ldx #>frame1_irq
        sta $fffe
        stx $ffff
        
        ldy #FIRST_OPEN_SB_LINE
        lda #$1b
        sta $d011
        sty $d012
        
        ldx #%11111110
        stx $dc00

        ldx $dc01
        cpx #%11101111  ; F1
        bne +
        
spritexlopatch1=*+1        
        inc $d00c
        bne +
        lda $d010
spritexmsbpatch1=*+1
        eor #$40
        sta $d010
+
        
        cpx #%11011111  ; F3
        bne +
        
spritexlopatch2=*+1        
        dec $d00c
spritexlopatch3=*+1        
        lda $d00c
        cmp #$ff
        bne +
        lda $d010
spritexmsbpatch2=*+1
        eor #$40
        sta $d010
+

incdecdelay=*+1
        lda #0
        and #$0f
        bne ++

        cpx #%10111111  ; F5
        bne +
        inc spritenumber
+
        cpx #%11110111  ; F7
        bne +
        dec spritenumber
+

++
        inc incdecdelay

spritenumber=*+1
        lda #3
        and #3
        tax
        lda spritesxlo,x
        sta spritexlopatch1
        sta spritexlopatch2
        sta spritexlopatch3
        lda spritesxmsb,x
        sta spritexmsbpatch1
        sta spritexmsbpatch2
        
        txa
        clc
        adc #'3'
        sta SCREEN+(8*40)+7

;irq_end        

framecount=*+1
        lda #3
        bne +
        lda #0
        sta $d7ff
+
        dec framecount
        
        dec $d019
        dec $01
        pla
        ldx x_stack
        ldy y_stack
nmi        
        rti

spritesxlo:
        !byte $06, $08, $0a, $0c
spritesxmsb:
        !byte $08, $10, $20, $40

;---------------------------------------------------------------------------
textline    
        ;!text "0123456789012345678901234567890123456789"
        !text "sprite 6:y($d00d)=$7a;x($d010,$d00c)=   "
;---------------------------------------------------------------------------
        *= $0f00
sprite_data
data_for_spr3
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%10000001
        !by %11111111,%11111111,%11111101
        !by %11111111,%11111111,%11100001
        !by %11111111,%11111111,%11111101
        !by %11111111,%11111111,%10000001
        !by %11111111,%11111111,%11111111
        !by $00
data_for_spr4
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%10111101
        !by %11111111,%11111111,%10111101
        !by %11111111,%11111111,%10000001
        !by %11111111,%11111111,%11111101
        !by %11111111,%11111111,%11111101
        !by %11111111,%11111111,%11111111
        !by $00
data_for_spr5
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%10000001
        !by %11111111,%11111111,%10111111
        !by %11111111,%11111111,%10000001
        !by %11111111,%11111111,%11111101
        !by %11111111,%11111111,%10000011
        !by %11111111,%11111111,%11111111
        !by $00
data_for_spr6
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%11111111
        !by %11111111,%11111111,%10000001
        !by %11111111,%11111111,%10111111
        !by %11111111,%11111111,%10000001
        !by %11111111,%11111111,%10111101
        !by %11111111,%11111111,%10000001
        !by %11111111,%11111111,%11111111
        !by $00
;---------------------------------------------------------------------------
!eof
;---------------------------------------------------------------------------
