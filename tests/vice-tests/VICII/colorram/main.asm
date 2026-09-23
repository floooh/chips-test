

*=$0801
; BASIC stub: "1 SYS 2061"
!byte $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00
    jmp entrypoint

    *=$1000
entrypoint:

    ldx #0
    stx $d020
    stx $d021
-
    txa
    and #$f
    sta $0400,x
    sta $d800,x
    lda #$20
    sta $0500,x
    sta $0600,x
    sta $0700,x
    lda #$1
    sta $d900,x
    sta $da00,x
    sta $db00,x
    dex
    bne -

loop:
    ldx #0
-
    lda $d800,x
    sta $0400+(7*40),x
    and #$f
    sta $0400+(14*40),x
    dex
    bne -

    ldy #5
    ldx #0
-
    txa
    and #$f
    sta temp
    lda $0400+(14*40),x
    cmp temp
    beq sk
    ldy #10
sk:
    dex
    bne -

    sty $d020

    lda $d020
    and #$0f
    ldx #0 ; success
    cmp #5
    beq nofail
    ldx #$ff ; failure
nofail:
    stx $d7ff

    jmp loop

temp: !byte 0