        *=$0801
        ; BASIC stub: "1 SYS 2061"
        !byte $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00
        jmp entrypoint

        *=$0900
entrypoint:
        JSR lC100
        STA $DC0D
        LDA #$01
        STA $D01A
        STA $D015
        LDA #<irq
        STA $0314
        LDA #>irq
        STA $0315
        LDA #$FA
        STA $D012
        LDA #$E6
        STA $D001
        LDX #$6F
        LDY #$00
        STY $D017
        LDA #$32
        STA $01
-
        LDA $D000,X
        STA $CE00,Y
        STA $CE70,Y
        LDA $D800,X
        STA $CF00,Y
        STA $CF70,Y
        INY
        DEX
        BPL -

        LDA #$37
        STA $01
        LDY #$0F
-
        LDA tab_c0dc,Y
        STA $CD00,Y
        STA $CD20,Y
        LDA #$18
        SEC
        SBC tab_c0dc,Y
        STA $CD10,Y
        STA $CD30,Y
        DEY
        BPL -

        LDY #$40
-
        LDA $CD00,Y
        STA $CD40,Y
        STA $CD80,Y
        DEY
        BPL -
        CLI
        RTS

irq:
        LDA #$13
        STA $D011
        NOP
        LDY #$6F
        INC $CFFF
        BIT $EA
-
mod_c07f = * + 1
        LDA $CD06,Y
        STA $D016
mod_c085 = * + 1
        LDX $CE53,Y
mod_c088 = * + 1
        LDA $CF1C,Y
        STA $3FFF
        STX $3FFF
        STA $3FFF
        STX $3FFF
        STA $3FFF
        STX $3FFF
        STA $3FFF
        STX $3FFF
        STA $3FFF
        STX $3FFF

        LDA #$00
        DEY
        BPL -

        STA $3FFF
        LDA #$08
        STA $D016
        LDA #$1B
        STA $D011
        LDA #$6F
        DEC mod_c085
        BPL +
        STA mod_c085
+
        SEC
        SBC mod_c085
        STA mod_c088

        LDA mod_c07f
        CLC
        SBC #$01
        AND #$1F
        STA mod_c07f

        INC $D019
        JMP $EA31

tab_c0dc:
        !byte $0c, $0c, $0d, $0e, $0e, $0F, $0F, $0F
        !byte $0F, $0F, $0F, $0F, $0E, $0E, $0D, $0C
        !byte $0C, $0B, $0A, $09, $09, $08, $08, $08
        !byte $08, $08, $08, $08, $09, $09, $0A, $0B
        !byte $00, $00, $00, $00

        !align 255,0,0
lC100:
        LDA #$1B
        STA $D011
        LDA #$7F
        SEI
        RTS
