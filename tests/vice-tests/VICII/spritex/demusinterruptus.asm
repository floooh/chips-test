; emu check extracted from "demusinterruptus/crest"

        * = $0801
        !word bend
        !word 10
        !byte $9e
        !text "2061", 0
bend:   !word 0

        SEI
        
        lda #0
        sta $d020
        sta $d021
        
        ldx #0
        lda #$20
-
        sta $0400,x
        sta $0500,x
        sta $0600,x
        sta $0700,x
        inx
        bne -
        
-       bit $d011
        bpl -
-       bit $d011
        bmi -

        LDA #$7F
        STA $DC0D
        LDA $DC0D
        LDA #<irq
        STA $0314
        LDA #>irq
        STA $0315
        LDA #$1B
        STA $D011
        LDA #$53
        STA $D012
        LDA #$01
        STA $D01A
        LDA $D019
        STA $D019
        
        LDX #$3F
-
        LDA #$02
        STA $0380,X
        LDA #$55
        STA $03C0,X
        DEX
        BPL -
        
        LDA #$0E
        STA $07F8
        LDA #$0F
        STA $07F9
        LDA #$4C
        STA $D000
        LDA #$5C
        STA $D001
        LDA #$4C
        STA $D002
        LDA #$5C
        STA $D003
        LDA #$03
        STA $D010
        LDA #$00
        STA $D01C
        LDA #$00
        STA $D017
        LDA #$03
        STA $D015
        CLI
        
        jmp *

        !align 255,0
irq:
        DEC $D019
        INC $D011
        DEC $D011
        LDA #$00
        LDX #$48
-
        DEX
        BPL -
        DEC $D016
        INC $D016
        LDA #$18
        STA $D011
        NOP
        NOP
        LDX #$07
-
        DEX
        BPL -
        DEC $D016
        INC $D016
        LDA #$19
        STA $D011
        NOP
        NOP
        LDX #$07
-
        DEX
        BPL -
        DEC $D016
        INC $D016
        LDA #$1A
        STA $D011
        NOP
        NOP
        LDX #$06
-
        DEX
        BPL -
        DEC $D016
        INC $D016
        LDA #$1B
        STA $D011
        NOP
        NOP
        LDX #$06
-
        DEX
        BPL -
        DEC $D016
        INC $D016
        LDA #$1C
        STA $D011
        NOP
        NOP
        LDX #$06
-
        DEX
        BPL -
        DEC $D016
        INC $D016
        LDA #$1D
        STA $D011
        LDA #$53
        STA $D012
        LDA #$1B
        STA $D011
        LDA $D01E
        STA $0400
        
        ldy #5
        ldx #0
        cmp #3
        beq +
        ldy #10
        ldx #$ff
+
        sty $D020
        stx $d7ff

        JMP $EA31
