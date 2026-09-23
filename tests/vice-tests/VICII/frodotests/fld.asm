        *=$0801
        ; BASIC stub: "1 SYS 2061"
        !byte $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00
        jmp entrypoint

        *=$0900
entrypoint:
        LDA #$00
        STA lc04c
        LDA #$FF
        STA $3FFF
        LDA #$32
        STA lc04b
        SEI
        LDA #$7F
        STA $DC0D
        LDA #$01
        STA $D01A
        LDA #<irq
        STA $0314
        LDA #>irq
        STA $0315
        LDA #$00
        STA $D012
        CLI
        RTS

irq:
        LDX lc04b
--
        LDY $D012
-
        CPY $D012
        BEQ -
        DEY
        TYA
        AND #$07
        ORA #$10
        STA $D011
        DEX
        BNE --
        INC $D019
        JSR moveit
        JMP $EA31

lc04b:
        !byte $32
lc04c:
        !byte 0

moveit:
        LDA lc04c
        BNE ++
        INC lc04b
        LDA lc04b
        CMP #$FA
        BNE +
        STA lc04c
+
        RTS
++
        DEC lc04b
        LDA lc04b
        CMP #$32
        BNE +
        LDA #$00
        STA lc04c
+
        RTS

