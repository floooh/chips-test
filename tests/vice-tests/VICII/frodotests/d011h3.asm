        *=$0801
        ; BASIC stub: "1 SYS 2061"
        !byte $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00
        jmp entrypoint

        *=$0900
;    * = $c000
entrypoint:
        LDA #$7F
        STA $DC0D
        LDA #<irq
        STA $0314
        LDA #>irq
        STA $0315
        LDA #$FF
        STA $DD03
        LDA #<$0400
        STA $AE
        LDA #>$0400
        STA $AF
        LDX #$00
        LDA #$07
-
        STA $D800,X
        STA $D900,X
        STA $DA00,X
        STA $DB00,X
        INX
        BNE -

        LDX #$00
--
        TXA
        LDY #$27
-
        STA ($AE),Y
        DEY
        BPL -
        LDA $AE
        CLC
        ADC #$28
        STA $AE
        BCC +
        INC $AF
+
        INX
        CPX #$19
        BNE --

-
        LDA $D012
        BNE -
        LDA #$00
        STA $AC
        STA $AD
        STA $AE
        STA $02
        STA $D020
        STA $FD
        STA $FE
        STA $F7
        LDA #>tab_c800
        STA $F8
        LDA #$1B
        STA $D011
        LDA #$FF
        STA $D012
        LDA $D019
        STA $D019
        LDA #$81
        STA $D01A

loop:
        LDA #$01
        STA $028B
        JSR $FFE4
        CMP #$1D
        BNE skp1
        INC $AC
        BNE +
        INC $AD
+
        JMP loop
skp1:
        CMP #$9D
        BNE skp2
        LDA $AC
        BNE +
        DEC $AD
+
        DEC $AC
        JMP loop
skp2:
        CMP #$11
        BNE skp3

        INC $AE
        BNE +
        INC $02
+
        JMP loop
skp3:
        CMP #$91
        BNE skp4
        LDA $AE
        BNE +
        DEC $02
+
        DEC $AE
        JMP loop
skp4:
        CMP #$54
        BNE skp5
        LDA #$00
        STA $FD
        STA $FE
        JMP loop
skp5:
        CMP #$47
        BNE skp6

        LDA #$20
        STA $FD
        STA $FE
        JMP loop
skp6:
        CMP #$58
        BNE skp7

        LDA #$20
        STA $FE
        LDA #$00
        STA $FD
        JMP loop
skp7:
        CMP #$20
        BNE skp8

        LDA #$00
        STA $D01A
        LDA #<$EA31
        STA $0314
        LDA #>$EA31
        STA $0315
        LDA #$81
        STA $DC0D
        LDA #$1B
        STA $D011
        RTS

skp8:
        JMP loop

        !align 255,0,0
irq4:
        LDA #<irq5
        STA $0314
        LDA #$06
        LDA $DD00
        LDX $D012
        INX
        INX
        STX $D012
        DEC $D019
        CLI
        LDX #$0A
-
        DEX
        BNE -
        NOP
        NOP
        NOP
        NOP
        JMP $EA81

irq5:
        LDA #<irq4
        STA $0314
        DEC $D019
        LDY $AD
        INC $D020
        LDA $D012
        CMP $D012
        BNE +
+
        LDX #$0A
-
        DEX
        BNE -
        BIT $FF
        LDA $AC
        AND #$07
        ORA #$C8
        STA $D016
        LDA $AC
        LSR $AD
        ROR 
        LSR 
        LSR 
        LSR 
        BCS +
+
        LSR 
        BCS +
+
        BCS +
+
        LSR 
        BCS +
+
        BCS +
+
        BCS +
+
        BCS +
+
        LSR 
        BCS +
+
        BCS +
+
        BCS +
+
        BCS +
+
        BCS +
+
        BCS +
+
        BCS +
+
        BCS +
+
        LSR 
        BCC skp9
        LDX #$02
-
        BIT $FF
        DEX
        BNE -
skp9:
        LSR 
        BCC skp10
        LDX #$06
-
        DEX
        BNE -
        NOP
skp10:
mod_c284 = * + 1
        LDA #$18
        STA $D011
mod_c289 = * + 1
        LDA #$1B
        STA $D011
        LDA #$35
        STA $D012
        DEC $D020
        STY $AD
        LDA #>irq
        STA $0315
        JMP $EA81

        !align 255,0,0
irq:
        LDA #<irq2
        STA $0314
        LDA #$06
        LDA $DD00
        LDX $D012
        INX
        INX
        STX $D012
        DEC $D019
        CLI
        LDX #$0A
-
        DEX
        BNE -
        NOP
        NOP
        NOP
        NOP
        JMP $EA81

irq2:
        LDA #<irq3
        STA $0314
        DEC $D019
        LDY $AD
        INC $D020
        LDA $D012
        CMP $D012
        BNE +
+
        LDX #$24
-
        DEX
        BNE -
        BIT $FF
mod_c33f = * + 1
        LDA #$1A
        STA $D011
        LDX #$0A
-
        DEX
        BNE -
        BIT $FF
        BIT $FF

mod_c34d = * + 1
        LDA #$1A
        STA $D011
        LDX #$0A
-
        DEX
        BNE -
        BIT $FF
        BIT $FF
mod_c35b = * + 1
        LDA #$1A
        STA $D011
        LDX #$0A
-
        DEX
        BNE -
        BIT $FF
        BIT $FF
mod_c369 = * + 1
        LDA #$1A
        STA $D011
        LDX #$0A
-
        DEX
        BNE -
        BIT $FF
        BIT $FF
mod_c377 = * + 1
        LDA #$1A
        STA $D011
        LDX #$0A
-
        DEX
        BNE -
        BIT $FF
        BIT $FF
mod_c385 = * + 1
        LDA #$1A
        STA $D011
        LDX #$0A
-
        DEX
        BNE -
        BIT $FF
        BIT $FF
mod_c393 = * + 1
        LDA #$1A
        STA $D011
        LDX #$0A
-
        DEX
        BNE -
        BIT $FF
        BIT $FF
mod_c3a1 = * + 1
        LDA #$1A
        STA $D011
        DEC $D020
        LDA $AE
        STA $C800
        LDA #$FF
        STA $D012
        LDA #>irq
        STA $0315
        JMP $EA81

irq3:
        LDA #<irq4
        STA $0314
        LDA #>irq4
        STA $0315
        DEC $D019
mod_c3c8 = * + 1
        LDA #$1B
        STA $D011
        LDA #$2B
        STA $D012
        LDY #$00
        STY $FA
        LDA ($F7),Y
        ASL 
        ROL $FA
        ASL 
        ROL $FA
        ASL 
        ROL $FA
        CLC
        ADC #<tab_c446
        STA $F9
        LDA $FA
        ADC #>tab_c446
        STA $FA
        LDX $FD
        TXA
        ORA ($F9),Y
        STA mod_c33f
        INY
        TXA
        ORA ($F9),Y
        STA mod_c34d
        INY
        TXA
        ORA ($F9),Y
        STA mod_c35b
        INY
        TXA
        ORA ($F9),Y
        STA mod_c369
        INY
        TXA
        ORA ($F9),Y
        STA mod_c377
        INY
        TXA
        ORA ($F9),Y
        STA mod_c385
        INY
        TXA
        ORA ($F9),Y
        STA mod_c393
        INY
        TXA
        ORA ($F9),Y
        STA mod_c3a1

        INC $F7
        LDY #$00
        LDA ($F7),Y
        CMP #$FF
        BNE +
        LDA #$00
        STA $F7
+
        LDA #$1B
        ORA $FE
        STA mod_c3c8
        STA mod_c289
        LDA #$18
        ORA $FE
        STA mod_c284
        JMP $EA31

tab_c446:
        !byte $1b, $1b, $1b, $1b, $1b, $1b, $1b, $1b, $1a, $1a, $1a, $1a, $1a, $1a, $1a, $1a
        !byte $19, $19, $19, $19, $19, $19, $19, $19, $18, $18, $18, $18, $18, $18, $18, $18
        !byte $1f, $1f, $1f, $1f, $1f, $1f, $1f, $1f, $1e, $1e, $1e, $1e, $1e, $1e, $1e, $1e
        !byte $1d, $1d, $1d, $1d, $1d, $1d, $1d, $1d, $1c, $1c, $1c, $1c, $1c, $1c, $1c, $1c
        !byte $1c, $1b, $1b, $1b, $1b, $1b, $1b, $1b, $1c, $1a, $1a, $1a, $1a, $1a, $1a, $1a
        !byte $1c, $19, $19, $19, $19, $19, $19, $19, $1c, $18, $18, $18, $18, $18, $18, $18
        !byte $1c, $1f, $1f, $1f, $1f, $1f, $1f, $1f, $1c, $1e, $1e, $1e, $1e, $1e, $1e, $1e
        !byte $1c, $1d, $1d, $1d, $1d, $1d, $1d, $1d, $1c, $1d, $1c, $1c, $1c, $1c, $1c, $1c
        !byte $1c, $1d, $1b, $1b, $1b, $1b, $1b, $1b, $1c, $1d, $1a, $1a, $1a, $1a, $1a, $1a
        !byte $1c, $1d, $19, $19, $19, $19, $19, $19, $1c, $1d, $18, $18, $18, $18, $18, $18
        !byte $1c, $1d, $1f, $1f, $1f, $1f, $1f, $1f, $1c, $1d, $1e, $1e, $1e, $1e, $1e, $1e
        !byte $1c, $1d, $1e, $1d, $1d, $1d, $1d, $1d, $1c, $1d, $1e, $1c, $1c, $1c, $1c, $1c
        !byte $1c, $1d, $1e, $1b, $1b, $1b, $1b, $1b, $1c, $1d, $1e, $1a, $1a, $1a, $1a, $1a
        !byte $1c, $1d, $1e, $19, $19, $19, $19, $19, $1c, $1d, $1e, $18, $18, $18, $18, $18
        !byte $1c, $1d, $1e, $1f, $1f, $1f, $1f, $1f, $1c, $1d, $1e, $1f, $1e, $1e, $1e, $1e
        !byte $1c, $1d, $1e, $1f, $1d, $1d, $1d, $1d, $1c, $1d, $1e, $1f, $1c, $1c, $1c, $1c
        !byte $1c, $1d, $1e, $1f, $1b, $1b, $1b, $1b, $1c, $1d, $1e, $1f, $1a, $1a, $1a, $1a
        !byte $1c, $1d, $1e, $1f, $19, $19, $19, $19, $1c, $1d, $1e, $1f, $18, $18, $18, $18
        !byte $1c, $1d, $1e, $1f, $18, $1f, $1f, $1f, $1c, $1d, $1e, $1f, $18, $1e, $1e, $1e
        !byte $1c, $1d, $1e, $1f, $18, $1d, $1d, $1d, $1c, $1d, $1e, $1f, $18, $1c, $1c, $1c
        !byte $1c, $1d, $1e, $1f, $18, $1b, $1b, $1b, $1c, $1d, $1e, $1f, $18, $1a, $1a, $1a
        !byte $1c, $1d, $1e, $1f, $18, $19, $19, $19, $1c, $1d, $1e, $1f, $18, $19, $18, $18
        !byte $1c, $1d, $1e, $1f, $18, $19, $1f, $1f, $1c, $1d, $1e, $1f, $18, $19, $1e, $1e
        !byte $1c, $1d, $1e, $1f, $18, $19, $1d, $1d, $1c, $1d, $1e, $1f, $18, $19, $1c, $1c
        !byte $1c, $1d, $1e, $1f, $18, $19, $1b, $1b, $1c, $1d, $1e, $1f, $18, $19, $1a, $1a
        !byte $1c, $1d, $1e, $1f, $18, $19, $1a, $19, $1c, $1d, $1e, $1f, $18, $19, $1a, $18
        !byte $1c, $1d, $1e, $1f, $18, $19, $1a, $1f, $1c, $1d, $1e, $1f, $18, $19, $1a, $1e
        !byte $1c, $1d, $1e, $1f, $18, $19, $1a, $1d, $1c, $1d, $1e, $1f, $18, $19, $1a, $1c
        !byte $1c, $1d, $1e, $1f, $18, $19, $1a, $1b, $1b, $1b, $1b, $1b, $1b, $1b, $1b, $1b
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
        !byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
tab_c800:
        !byte $00, $02, $05, $08, $0b, $0e
        !byte $11, $14, $16, $19, $1b, $1e, $20, $23, $25, $27, $29, $2b, $2d, $2e, $30, $31
        !byte $33, $34, $35, $36, $36, $37, $37, $37, $37, $37, $37, $37, $36, $36, $35, $34
        !byte $33, $31, $30, $2e, $2d, $2b, $29, $27, $25, $23, $20, $1e, $1c, $19, $16, $14
        !byte $11, $0e, $0b, $08, $05, $02, $00, $ff
