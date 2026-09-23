
        *=$0801
        ; BASIC stub: "1 SYS 2061"
        !byte $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00
        jmp entrypoint

        *=$0900
entrypoint:

        SEI
        LDA #$7F
        STA $DC0D
        LDA #$81
        STA $D01A
        LDA #<irq
        STA $0314
        LDA #>irq
        STA $0315
        LDA #$31
        STA $D012
        LDA #$1B
        STA $D011
        LDY #<$4000
        LDX #>$4000
        STX $FC
        STY $FB
        TYA
-
        STA ($FB),Y
        INY
        BNE -
        INC $FC
        BPL -
        LDA #>$4400
        STA $FC
        LDA #$32
        STA $01
--
        TYA
        LSR 
        LSR 
        LSR 
        TAX
        LDA tab_c15e,X
        ASL 
        ASL 
        ASL 
        TAX
        LDA #$D0
        ADC #$00
        STA mod_c04e
-
mod_c04e = * + 2
        LDA $D000,X
        STA ($FB),Y
        INX
        INY
        TXA
        AND #$07
        BNE -
        CPY #$00
        BNE --

        LDA #$37
        STA $01
-
        LDA $4400,Y
        STA $4C08,Y
        STA $5410,Y
        STA $5C18,Y
        STA $6420,Y
        STA $6C28,Y
        STA $7430,Y
        STA $7C38,Y
        INY
        BNE -

        LDA #$00
        STA $033C
        STA $033D
        LDA #$01
        STA $033E
-
        TYA
        ORA #$80
        STA $4000,Y
        STA $4028,Y
        STA $4050,Y
        STA $4078,Y
        STA $40A0,Y
        STA $40C8,Y
        STA $40F0,Y
        STA $4118,Y
        LDA #$EF
        STA $4140,Y
        INY
        CPY #$28
        BNE -
        CLI
        RTS

irq:
        LDA #$96
        STA $DD00
        NOP
        NOP
        LDY $033C
        JMP irqskp1

irqlp:
        NOP
irqlp2:
        LDA $CF00,Y
        STA $D016
        LDA $CE00,Y
        STA $D018
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        LDA $D012
        CMP #$78
        BPL irqskp2
        INY
        DEX
        BNE irqlp

irqskp1:
        LDA $CF00,Y
        STA $D016
        LDA $CE00,Y
        STA $D018
        INY
        LDX #$07
        JMP irqlp2

irqskp2:
        LDA #$97
        STA $DD00
        LDA #$16
        STA $D018
        LDA #$08
        STA $D016
        LDA $DC00
        AND $DC01
        TAX
        LDY $033E
        AND #$08
        BNE +

        INY
        CPY #$04
        BPL +
        STY $033E
+
        TXA
        AND #$04
        BNE +

        DEY
        CPY #$FC
        BMI +

        STY $033E
+
        LDA $033D
        CLC
        ADC $033E
        BPL +

        LDA $033E
        EOR #$FF
        CLC
        ADC #$01
        STA $033E
        LDA $033D
+
        STA $033D
        LSR 
        TAX
        AND #$07
        ORA #$08
        LDY $033C
        STA $CF00,Y
        TXA
        LSR 
        LSR 
        LSR 
        ASL 
        STA $CE00,Y
        DEC $033C
        LDA #$01
        STA $D019
        JMP $EA31

tab_c15e:
        !byte $14, $08, $09, $13, $20, $09, $13, $20 
        !byte $14, $05, $03, $08, $2D, $14, $05, $03
        !byte $08, $20, $06, $0F, $12, $20, $03, $3D
        !byte $36, $34, $20, $02, $19, $20, $0D, $05
        !byte $0F, $12, $00, $00
