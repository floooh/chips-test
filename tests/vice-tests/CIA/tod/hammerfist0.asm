         PROCESSOR 6502
         ORG $801

         hex 0b 08 0a 00  9e 32 30 36  31 00 00 00

        JSR set
        JSR reset
        LDA $dc0d
        LSR
        LSR
        LSR
        BCS fine
        LDY #$01
fine    STY $400

        ldy #10
        lda $400
        cmp #4
        bne fail
        ldy #5
fail:
        sty $d020

        lda $d020
        and #$0f
        ldx #0 ; success
        cmp #5
        beq nofail
        ldx #$ff ; failure
nofail:
        stx $d7ff

        RTS

vax     hex 03 12 30 09

set     LDY #$03
iniz    LDA vax,Y
        STA $dc08,Y
        DEY
        BPL iniz
        rts

reset   LDY #$00
        LDA #$00
cont    STA $dc08,Y
        INY
        CPY #$04
        BCC cont
        rts

