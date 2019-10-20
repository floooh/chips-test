        *=$0801             ; basic start
        !WORD +
        !WORD 10
        !BYTE $9E
        !TEXT "2061"
        !BYTE 0
+       !WORD 0

        lda #$93
        jsr $ffd2

        lda #$11
        sta $dc0e

        lda #$08
        sta $dc0f

        lda #$00
        sta $dc09
        sta $dc0a
        sta $dc0b
        sta $dc08       ; 10th seconds (starts clock)
lp:
        lda $dc09
        sta $0400
        cmp #$05
        bne lp

        lda #$05
        sta $d020

        lda $d020
        and #$0f
        ldx #0 ; success
        cmp #5
        beq nofail
        ldx #$ff ; failure
nofail:
        stx $d7ff

        jmp *
