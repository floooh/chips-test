        *=$0801
        ; BASIC stub: "1 SYS 2061"
        !byte $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00
        jmp entrypoint

        *=$0900
entrypoint:
        SEI
        LDA #$7F
        STA $DC0D
        LDA #$01
        STA $D01A
        STA $D015
        LDA #<irq
        STA $0314
        LDA #>irq
        STA $0315
        LDA #$1B
        STA $D011
        LDA #$FA
        STA $D012
        LDA #$E6
        STA $D001
        LDX #$33
        LDY #$00
        STA $D017
-
        LDA tab_c0ac,X
        PHA
        AND #$0F
        STA $CE00,X
        STA $CE34,Y
        STA $CE68,X
        STA $CE9C,Y
        PLA
        LSR
        LSR
        LSR
        LSR
        STA $CF00,X
        STA $CF34,Y
        STA $CF68,X
        STA $CF9C,Y
        INY
        DEX
        BPL -
        CLI
        RTS

irq:
        NOP
        NOP
        NOP
        NOP
        LDY #$67
        INC $CFFF
        DEC $CFFF
-
mod_c064 = * + 1
        LDX $CE18,Y
mod_c067 = * + 1
        LDA $CF4F,Y
        STA $D020
        STX $D020
        STA $D020
        STX $D020
        STA $D020
        STX $D020
        STA $D020
        STX $D020
        STA $D020
        STX $D020
        STA $D020
        STX $D020
        LDA #$00
        DEY
        BPL -
        STA $D020
        LDA #$67
        DEC mod_c064
        BPL +
        STA mod_c064
+
        SEC
        SBC mod_c064
        STA mod_c067
        INC $D019
        JMP $EA31

tab_c0ac:
        !byte $09, $90, $09, $9B, $00, $99, $2B, $08
        !byte $90, $29, $8B, $08, $9C, $20, $89, $AB
        !byte $08, $9C, $2F, $80, $A9, $FB, $08, $9C
        !byte $2F, $87, $A0, $F9, $7B, $18, $0C, $6F 
        !byte $07, $61, $40, $09, $6B, $48, $EC, $0F 
        !byte $67, $41, $E1, $30, $09, $6B, $48, $EC
        !byte $3F, $77, $11, $11, $00, $00

