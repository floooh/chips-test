
DEBUGCOLOR = $dbff
;DEBUGCOLOR = $d020

scrptr = $02

    * = $0801
    .byte $0c, $08, $00, $00, $9e, $20, $32, $35, $36, $30
    .byte 0,0,0

        * = $0a00

        lda #$17
        sta $d018
        lda #$0e
        sta $d020
        lda #$06
        sta $d021
        
        ldx #0
lp1:
        lda #$20
        sta $0400,x
        sta $0500,x
        sta $0600,x
        txa
        sta $0700,x
        sta $db00,x
        lda #$01
        sta $d800,x
        sta $d900,x
        sta $da00,x
        inx
        bne lp1

        ldx #0
lp2:
        lda texttab,x
        and #$3f
        sta $0400,x
        sta $0400+(5*40),x
        sta $0400+(10*40),x
        sta $0400+(15*40),x
        lda colortab,x
        sta $d800,x
        sta $d800+(5*40),x
        sta $d800+(10*40),x
        sta $d800+(15*40),x
        inx
        cpx #5*40
        bne lp2

        lda #%00110100
        sta $3fff

        SEI
        LDA #$01
        STA $D01A
        STA $DC0D
        STA $D019

        LDA #<irq
        LDY #>irq
        STA $0314
        STY $0315

        LDA #$10
        STA $D012
.if (OFFSET == 0)
        LDA #$F0
.else
        LDA #OFFSET
.endif
        STA $FF

        LDA #$1B
        STA $D011
        CLI

        jmp *

;-------------------------------------------------------------------------------

        .align 256
irq:
        jsr rastersync_lp

        inc DEBUGCOLOR

        ; Calculate a variable offset to delay by branching over nops
        lda #126 - 1
        sec
xPosOffset:
.if (CYCLE == 0)
        sbc #$21
.else
        sbc #CYCLE
.endif
        ; divide by 2 to get the number of nops to skip
        lsr
        sta sm1+1
        ; Force branch always
        clv

        ; Introduce a 1 cycle extra delay depending on the least significant bit of the x offset
        bcc sm1
sm1:
        bvc *
        ; The above branches somewhere into these nops depending on the x offset position
        .rept 126 / 2
        nop
        .next
        dec DEBUGCOLOR

        LDX $FF

        nop
        nop
irqlp:
        NOP
        NOP
        NOP

        LDA $D012
        AND #$07
        ORA #$10
        STA $D011

        LDY #$07
delay:
        DEY
        BNE delay

        sta DEBUGCOLOR
        DEX
        BNE irqlp

        LDA #$01
        STA $D019

        lda #0
        sta DEBUGCOLOR

.if (OFFSET == 0)
        ; check shift
        lda #%11111101
        sta $dc00
        lda $dc01
        and #%10000000
        pha
        bne shiftnotpressed
        
        ; check keys
        inc kdelay1+1
kdelay1: 
        lda #0
        and #$07
        bne skEND1
.endif
        lda #%11111101
        sta $dc00
        lda $dc01
        and #%00000010      ;W
        bne +
        inc $ff
+
        lda #%01111111
        sta $dc00
        lda $dc01
        and #%01000000      ;Q
        bne +
        dec $ff
+
skEND1:
        
.if (OFFSET == 0)
shiftnotpressed:
        pla
        beq shiftpressed
        
smod2:
        DEC $FF

        LDA $FF
        CMP #$1C
        BNE skp1

        LDA #$E6                ; inc zp
        STA smod2
        LDA #$FF
        STA smod2+1
skp1:
        LDA $FF
        CMP #$F0
        BNE skp2

        LDA #$c6                ; dec zp
        STA smod2
        LDA #$FF
        STA smod2+1
skp2:

shiftpressed:
.endif
        ; check keys
        inc kdelay+1
kdelay: lda #0
        and #$07
        bne skEND

        lda #%11111101
        sta $dc00
        ldy $dc01

        tya
        and #%00000100
        bne skA

        ldx xPosOffset+1
        dex
        cpx #$ff
        beq o1
        stx xPosOffset+1
o1:
        jmp skEND
skA:

        tya
        and #%00100000
        bne skS
        ldx xPosOffset+1
        inx
        cpx #126
        beq o2
        stx xPosOffset+1
o2:
skS:
skEND:
        lda #>($0400+(1*40)+2)
        sta scrptr+1
        lda #<($0400+(1*40)+2)
        sta scrptr

        lda xPosOffset+1
        jsr hexout

        lda #>($0400+(3*40)+2)
        sta scrptr+1
        lda #<($0400+(3*40)+2)
        sta scrptr

        lda $ff
        jsr hexout

        lda $0402+(1*40)
        sta $0402+(40*6)
        sta $0402+(40*11)
        sta $0402+(40*16)

        lda $0403+(1*40)
        sta $0403+(40*6)
        sta $0403+(40*11)
        sta $0403+(40*16)

        lda $0402+(3*40)
        sta $0402+(2*40)+(40*6)
        sta $0402+(2*40)+(40*11)
        sta $0402+(2*40)+(40*16)

        lda $0403+(3*40)
        sta $0403+(2*40)+(40*6)
        sta $0403+(2*40)+(40*11)
        sta $0403+(2*40)+(40*16)

        ; make test exit after 3 frames
        dec framecount
        bne +
        lda #0
        sta $d7ff
+

        JMP $EA7E

framecount: .byte 3

;-------------------------------------------------------------------------------

hexout:
        ldy #0
        pha
        lsr
        lsr
        lsr
        lsr
        tax
        lda hextab,x
        sta (scrptr),y
        iny
        pla
        and #$0f
        tax
        lda hextab,x
        sta (scrptr),y
        clc
        lda scrptr
        adc #3
        sta scrptr
        bcc sk
        inc scrptr+1
sk:
        rts


hextab:
        .byte $30, $31, $32, $33, $34, $35, $36, $37, $38, $39
        .byte $01, $02, $03, $04, $05, $06

texttab:      ;1234567890123456789012345678901234567890
        .text "A12345678901234567890123456789012345678Z"
        .text "B.XX.(A-S)..RANGE.21-29.SHOULD.BE.FLD..Y"
        .text "C...........PRESS.SHIFT.TO.HOLD........X"
        .text "D.XX (Q-W)..TO SCROLL.MANUALLY.........W"
        .text "E12345678901234567890123456789012345678V"

colortab:     ;1234567890123456789012345678901234567890
        .text "@KL@KL@KL@KL@KL@KL@KL@KL@KL@KL@KL@KL@KL@"
        .text "KLAALAAAAAL@AAAAA@AAAAA@AAAAAAKAAKAAAL@K"
        .text "L@KL@KL@KL@KAAAAAKAAAAAKAAKAAAA@KL@KL@KL"
        .text "@KAAKAAAAAKLAALAAAAAA@AAAAAAAA@KL@KL@KL@"
        .text "KL@KL@KL@KL@KL@KL@KL@KL@KL@KL@KL@KL@KL@K"

        .align 256

; the lightpen is usually connected to pin 6 of joystick port 1, which is the
; same as used for fire on a regular joystick (and space on the keybpard)
; this line is then directly connected to both the cia (bit 4 of cia1 port B)
; and the lightpen input of the vic, which means that the lightpen line of the
; vic can be artificially written to by toggling said cia port bit.

rastersync_lp:

         ; acknowledge vic irq
;         lda $d019
;         sta $d019

         ldx #$ff
         ldy #$00
         ; prepare cia ports
         stx $dc00     ; port A = $ff (inactive)
         sty $dc02     ; ddr A = $00  (all input)
         stx $dc03     ; ddr B = $ff  (all output)
         stx $dc01     ; port B = $ff (inactive)
         ; now trigger the lp latch
         sty $dc01     ; port B = $00 (active)
         stx $dc01     ; port B = $ff (inactive)
         lda $d013     ; get x-position (pixels, divided by two)
         ; restore cia setup
         stx $dc02     ; ddr A = $ff  (all output)
         sty $dc03     ; ddr B = $00  (all input)
         stx $dc01     ; port B = $ff (inactive)
         ldx #$7f
         stx $dc00     ; port A = $7f

         ; divide x-pos by 4 to get x-position in cycles (0..62)
         lsr
         lsr
         ; delay 62 - n cycles
         lsr
         sta timeout+1
         bcc timing   ; 2, +1 extra cycle if even
timing:  clv
timeout: bvc timeout
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

         rts
