
        !convtab pet
        !cpu 6510

;expanded=0      ; +8k

VICCOLSVADDRHI = $9002
VICRASTERLINE  = $9004
VICCHARADDR    = $9005
VICOSC1FREQ    = $900A
VICOSC2FREQ    = $900B
VICOSC3FREQ    = $900C
VICNOISEFREQ   = $900D
VICVOLAUXCOL   = $900E
VICBGBORDERCOL = $900F

!if (VIA=1) {
VIABASE        = $9110
} else {
VIABASE        = $9120
}
VIAORB = VIABASE + $00
VIAORA = VIABASE + $01
VIATIMER1LO = VIABASE + $06
VIATIMER1HI = VIABASE + $07
VIATIMER2LO = VIABASE + $08
VIATIMER2HI = VIABASE + $09
VIAACR  = VIABASE + $0b
VIAPCR  = VIABASE + $0c
VIAIFR  = VIABASE + $0d
VIAIER  = VIABASE + $0e

charsperline = 22
numrows = 23

!if (expanded=1) {
basicstart = $1201      ; expanded
screenmem = $1000
colormem = $9400
} else {
basicstart = $1001      ; unexpanded
screenmem = $1e00
colormem = $9600
}

;-------------------------------------------------------------------------------

                * = basicstart
                !word basicstart + $0c
                !byte $0a, $00
                !byte $9e
!if (expanded=1) {
                !byte $34, $36, $32, $32
} else {
                !byte $34, $31, $31, $30
}
                !byte 0,0,0

                * = basicstart + $0d
start:
        jmp start2
                * = basicstart + $00ff
;-------------------------------------------------------------------------------
start2:
                jsr $fd52
                jsr $fdf9
                jsr $e518

                sei


                lda #$20
                ldx #0
-
                lda #$20
                sta screenmem,x
                sta screenmem+$100,x
                lda #$03
                sta colormem,x
                sta colormem+$100,x
                inx
                bne -

                ldx #$3
-
                lda viatext,x
                sta screenmem+(charsperline*3)+(charsperline-4),x
                dex
                bpl -

                lda     #0
                sta     $E9

!if (VIA=1) {
                LDA     #<irq
                STA     $318
                LDA     #>irq
                STA     $319
} else {
                LDA     #<irq
                STA     $314
                LDA     #>irq
                STA     $315
}
                lda #$ff
                ldx #4
-
                sta VIABASE,x
                inx
                cpx #$10
                bne -

                ; disable interupts
                lda     #$7f
                sta     VIAIER

                ; clear flags
                lda     #$ff
                sta     VIAIFR

                ; wait for rasterline $068
loc_A7DA:
                LDA     VICRASTERLINE
                BNE     loc_A7DA

loc_A7DF:
                LDA     VICRASTERLINE
                CMP     #$68 ; 'h'
                BNE     loc_A7DF

                lda     #$60    ; $40 in bandits
                sta     VIAACR
                lda     #$88    ; $DE in bandits
                sta     VIAPCR

                lda     #<$4243
                sta     VIATIMER1LO
                sta     VIATIMER2LO
                lda     #>$4243
                sta     VIATIMER1HI
                sta     VIATIMER2HI

                ; enable timer 1 irq
                lda     #$c0
                sta     VIAIER

                ; clear flags
                lda     #$7f
                sta     VIAIFR

                lda     VIATIMER1LO     ; clear flags
                lda     VIATIMER2LO     ; clear flags
                lda     VIAORB
                lda     VIAORA

!if (VIA=2) {
                cli
}
loop

framewait       lda #5
                bne framewait
                

                lda #>(screenmem+(charsperline*3))
                sta $03
                lda #<(screenmem+(charsperline*3))
                sta $02

                lda screenmem
                jsr printhex
                lda screenmem+1
                jsr printhex
                lda screenmem+2
                jsr printhex

                lda screenmem+charsperline
                jsr printhex
                lda screenmem+charsperline+1
                jsr printhex
                lda screenmem+charsperline+2
                jsr printhex

                ldy #5
                
                ldx #2
-
                lda     colormem+(0*charsperline)+0,x
                and #$0f
                cmp #5
                beq +
                ldy #2
+
                lda     colormem+(1*charsperline)+0,x
                and #$0f
                cmp #5
                beq +
                ldy #2
+
                dex
                bpl -

                sty $900f
            
                lda #0      ; success
                cpy #5
                beq +
                lda #$ff    ; failure
+
                sta $910f

                jmp loop


!if (0) {
via2_register_table:
; original values from bandits
                !byte   0 ;0 Port B output register
                !byte   0 ;1 Port A output register
                !byte $7F ;2 Data direction register B
                !byte   0 ;3 Data direction register A
                !byte $43 ;4 Timer 1 low byte
                !byte $42 ;5 Timer 1 high byte & counter
                !byte $43 ;6 Timer 1 low byte
                !byte $42 ;7 Timer 1 high byte
                !byte   0 ;8 Timer 2 low byte
                !byte   0 ;9 Timer 2 high byte
                !byte $FF ;a Shift register
                !byte $40 ;b Auxiliary control register
                !byte $DE ;c Peripheral control register
                !byte   0 ;d Interrupt flag register
                !byte $C0 ;e Interrupt enable register          Timer1 irq enabled
                !byte $FF ;f Port A (Sense cassette switch)
}

; timer values
unk_A1ED:
; NTSC
;            !byte <$2AA2
;            !byte <$179F
            !byte <($2AA2+2593)
            !byte <($179F+2594)
unk_A1EF:
; NTSC
;            !byte >$2AA2
;            !byte >$179F
            !byte >($2AA2+2593)
            !byte >($179F+2594)

irq:
                inc     VICBGBORDERCOL
!if (VIA=1) {
                pha
                txa
                pha
                tya
                pha
}
                CLD
                INC     $E9
                LDA     $E9
                AND     #1
                TAX             ; X: odd or even IRQ
                BEQ     loc_BE1C

                lda     VIAIFR
                sta     screenmem+(0*charsperline)+0

                LDA     unk_A1ED,X
                STA     VIATIMER1LO ; $9126

                lda     VIAIFR
                sta     screenmem+(0*charsperline)+1

                LDA     unk_A1EF,X
                STA     VIATIMER1HI ; $9127

                lda     VIAIFR
                sta     screenmem+(0*charsperline)+2

                ; odd IRQ only
                lda     #8
                STA     VICBGBORDERCOL

                ldy #$02
                lda #$05
                ldx     screenmem+(0*charsperline)+0
                cpx #$c0
                beq +
                tya
+
                sta     colormem+(0*charsperline)+0

                lda #$05
                ldx     screenmem+(0*charsperline)+1
                cpx #$c0
                beq +
                tya
+
                sta     colormem+(0*charsperline)+1

                lda #$05
                ldx     screenmem+(0*charsperline)+2
                cpx #$00
                beq +
                tya
+
                sta     colormem+(0*charsperline)+2



                PLA
                TAY
                PLA
                TAX
                PLA
                RTI

; ---------------------------------------------------------------------------

loc_BE1C:
                lda     VIAIFR
                sta     screenmem+(1*charsperline)+0

                LDA     unk_A1EF,X
                STA     VIATIMER1HI ; $9127

                lda     VIAIFR
                sta     screenmem+(1*charsperline)+1

                LDA     unk_A1ED,X
                STA     VIATIMER1LO ; $9126

                lda     VIAIFR
                sta     screenmem+(1*charsperline)+2


                LDY     #$6e
                STY     VICBGBORDERCOL

                ; delay
                LDX     #$25
loc_BE23:
                DEX
                BNE     loc_BE23

                LDY     #$08
                STY     VICBGBORDERCOL

                lda     #$10
                STA     VICVOLAUXCOL

                ldy #$02
                lda #$05
                ldx     screenmem+(1*charsperline)+0
                cpx #$c0
                beq +
                tya
+
                sta     colormem+(1*charsperline)+0

                lda #$05
                ldx     screenmem+(1*charsperline)+1
                cpx #$00
                beq +
                tya
+
                sta     colormem+(1*charsperline)+1

                lda #$05
                ldx     screenmem+(1*charsperline)+2
                cpx #$00
                beq +
                tya
+
                sta     colormem+(1*charsperline)+2

                lda framewait+1
                beq +
                dec framewait+1
+
                
                PLA
                TAY
                PLA
                TAX
                PLA
                RTI

viatext:
!if (VIA=1) {
    !scr "via1"
} else {
    !scr "via2"
}


printhex:
                pha
                lsr
                lsr
                lsr
                lsr
                tax
                ldy #0
                lda hexdigits,x
                sta ($02),y
                iny
                pla
                and #$0f
                tax
                lda hexdigits,x
                sta ($02),y

                lda $02
                clc
                adc #3
                sta $02
                lda $03
                adc #0
                sta $03

                rts

hexdigits:
                !scr "0123456789abcdef"
