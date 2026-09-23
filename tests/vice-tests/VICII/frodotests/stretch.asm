
        *=$0801
        ; BASIC stub: "1 SYS 2061"
        !byte $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00
        jmp entrypoint

        *=$0900
entrypoint:
        SEI
        LDA #<irq
        STA $0314
        LDA #>irq
        STA $0315
        LDX #$3E
-
        LDA spritedata,X
        STA $0340,X
        DEX
        BPL -

        LDX #$07
-
        LDA #$0D
        STA $07F8,X
        LDA #$02
        STA $D027,X
        DEX
        BPL -

        LDX #$26
-
        LDA vicregtable,X
        STA $D000,X
        DEX
        BPL -

        LDX #$7F
        STX $DC0D
-
        TXA
        AND #$07
        ORA #$10
        STA $CF00,X
        LDA #$00
        STA $CE80,X
        DEX
        BPL -

        STA $3FFF
        LDX #$17
-
        LDA rastercolors,X
        STA $CE88,X
        STA $CEA0,X
        STA $CEB8,X
        STA $CED0,X
        STA $CEE0,X
        DEX
        BPL -

        JSR sub_c167
        CLI
        RTS

irq:
        LDX #$01
        LDY #$08
        NOP
        NOP
        NOP
        BIT $EA
-
        LDA $CEFF,X
        STA $D011
        LDA $CE80,X
        DEC $D016
        STA $D021
        STY $D016
        LDA $CF80,X
        STA $D017
        EOR #$FF
        STA $D017
        INX
        BPL -
        INC $D019
        JSR sub_c114
        JMP $EA31

spritedata:
        !byte $00, $00, $00
        !byte $03, $fb, $00
        !byte $07, $7e, $00
        !byte $35, $df, $00
        !byte $1d, $77, $00
        !byte $b7, $5d, $00
        !byte $bd, $83, $7e
        !byte $ef, $01, $de
        !byte $bb, $01, $78
        !byte $ae, $03, $70
        !byte $eb, $00, $00
        !byte $ba, $03, $60
        !byte $ee, $03, $d8
        !byte $fb, $02, $f6
        !byte $fe, $83, $bd
        !byte $9f, $ba, $00
        !byte $37, $ee, $00
        !byte $3d, $fb, $00
        !byte $07, $7e, $00
        !byte $03, $df, $00
        !byte $00, $00, $00

vicregtable:
        !byte $e8, $34, $20, $34, $50, $34, $80, $34
        !byte $b0, $34, $e0, $34, $10, $34, $40, $34
        !byte $c1, $18, $33, $00, $00, $ff, $08, $ff
        !byte $15, $01, $01, $ff, $ff, $ff, $00, $00
        !byte $00, $00, $00, $00, $00, $01, $0a

rastercolors:
        !byte $00, $0b, $0c, $0f, $01, $0f, $0c, $0b
        !byte $00, $06, $0e, $0d, $01, $0d, $0e, $06
        !byte $00, $09, $02, $0a, $01, $0a, $02, $09

sub_c114:
        ldx #$1f
        lda #$00
-
        STA $CF80,X
        STA $CFA0,X
        STA $CFC0,X
        STA $CFE0,X
        DEX
        BPL -

        STA mod_c14d
        LDA #$07
        STA mod_c135
        LDA #$80
        STA mod_c145
--
mod_c135 = * + 1
        LDX #$00
        LDY $0380,X
        LDA $0388,X
        STA mod_c151
        LDX #$14
-
        LDA $CF82,Y
mod_c145 = * + 1
        ORA #$00
        STA $CF82,Y
        STY mod_c15a
mod_c14d = * + 1
        LDA #$00
        AND #$07
mod_c151 = * + 1
        ADC #$00
        STA mod_c14d
        LSR
        LSR
        LSR
        CLC
mod_c15a = * + 1
        ADC #$00
        TAY
        DEX
        BNE -
        LSR mod_c145
        DEC mod_c135
        BPL --

sub_c167:

mod_c168 = * + 1
        LDA #$00
        ASL
        AND #$3F
        TAY
        INC mod_c168
        LDX #$07
-
        LDA tab_c19e,Y
        LSR
        LSR
        CLC
        ADC #$08
        STA $0388,X
        TYA
        ADC #$0A
        AND #$3F
        TAY
        DEX
        BPL -

        LDX #$07
        LDA mod_c168
        AND #$3F
        TAY
-
        LDA tab_c19e,Y
        STA $0380,X
        TYA
        ADC #$0A
        AND #$3F
        TAY
        DEX
        BPL -
        RTS

tab_c19e:
        !byte $20, $23, $26, $29, $2C, $2F, $31, $34
        !byte $36, $38, $3A, $3C, $3D, $3E, $3F, $3F 
        !byte $3F, $3F, $3F, $3E, $3D, $3C, $3A, $38
        !byte $36, $34, $31, $2F, $2C, $29, $26, $23
        !byte $20, $1C, $19, $16, $13, $10, $0E, $0B
        !byte $09, $07, $05, $03, $02, $01, $00, $00
        !byte $00, $00, $00, $01, $02, $03, $05, $07
        !byte $09, $0B, $0E, $10, $13, $16, $19, $1C 
        !byte $00, $00, $00, $00

