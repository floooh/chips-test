 
        *=$0801
        ; BASIC stub: "1 SYS 2061"
        !byte $0a,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00
        jmp entrypoint

        JMP l0874
entrypoint:
        JSR l091d
        LDA $0314
        LDX $0315
        CMP #<irq1
        BNE +
        CPX #>irq1
        BEQ ++
+
        SEI
        STA mod_08b9
        STX mod_08ba
        LDA #<irq1
        LDX #>irq1
        STA $0314
        STX $0315
++
        LDA #$1B
        STA $D011
        LDA #$34
        STA $D012

        LDX #$0E
        CLC
        ADC #$03
        TAY
        LDA #$00
        STA $FB
-
        LDA $FB
        STA $D000,X
        ADC #$18
        STA $FB
        TYA
        STA $D001,X
        DEX
        DEX
        BPL -

        LDA #$7F
        STA $DC0D
        STA $DD0D
        LDX #$01
        STX $D01A
        LDA $DC0D
        LSR $D019
        LDY #$FF
        STY $D015
        CLI
        RTS

l0874:
        SEI
        LDA #$1B
        STA $D011
        LDA #$81
        STA $DC0D
        LDA #$00
        STA $D01A
        LDA mod_08b9
        STA $0314
        LDA mod_08ba
        STA $0315
        BIT $DD0D
        CLI
        RTS

irq1:
        LDA #<irq2
        STA $0314
        LDA #>irq2
        STA $0315
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        INC $D012
        LDA #$01
        STA $D019
        CLI
        LDY #$09
-
        DEY
        BNE -
        NOP
        NOP
        NOP
        NOP
        NOP
mod_08b9 = * + 1
mod_08ba = * + 2
        JMP *

irq2:
        LDA #<irq1
        STA $0314
        LDA #>irq1
        STA $0315
        LDX $D012
        NOP
        BIT $24
        CPX $D012
        BEQ +
+
        DEX
        DEX
        STX $D012
        LDX #$01
        STX $D019
        LDX #$02
-
        DEX
        BNE -
        NOP
        NOP
        LDA #$14
        STA $FB
        LDX #$C8
irqlp:
        LDY #$02
-
        DEY
        BNE -
        DEC $D016
        STX $D016
        NOP
        DEC $FB
        BMI irq2end

        CLC
        LDA $D011
        SBC $D012
        AND #$07
        BNE irqlp
        DEC $FB
        NOP
        NOP
        NOP
        NOP
        DEC $D016
        STX $D016
        LDY #$02
-
        DEY
        BNE -
        NOP
        NOP
        NOP
        DEC $FB
        BPL irqlp
irq2end:
        JMP $EA81

l091d:
        LDA $0318
        LDY $0319
        PHA
        LDA #<(l0941+1)
        STA $0318
        LDA #>(l0941+1)
        STA $0319
        LDX #$81
        STX $DD0D
        LDX #$00
        STX $DD05
        INX
        STX $DD04
        LDX #$DD
        STX $DD0E
l0941:
        LDA #$40
        PLA
        STA $0318
        STY $0319
        RTS
