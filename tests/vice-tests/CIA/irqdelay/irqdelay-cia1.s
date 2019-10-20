
    .export Start

Start:
    ldx #$27
slp:
    lda #$20
    sta $0400,x
    lda #1
    sta $d800,x
    dex
    bpl slp

    sei

    lda #$35
    sta $01

mainloop:

    lda $d011
    bpl *-3
    lda $d011
    bmi *-3

    inc $d020
    dec $d020

    ; test CIA1 (IRQ)

    lda #$ff
    sta $dc04
    sta $dc05

    lda #%10010000 ; stop timer
    sta $dc0e

    ; Set IRQ vector
    lda #<continue2
    sta $fffe
    lda #>continue2
    sta $ffff

    lda #%10001000
    sta $dc0e
    lda $dc0d

    lda #$81
    sta $dc0d
    cli

    ; Set timer to $100 cycles
    lda #TESTVAL
    sta $dc04
    lda #0
    sta $dc05

.ifdef ONESHOT
    lda #%10011001 ; Fire a 1-shot timer
.else
    lda #%10010001 ; normal timer
.endif
    sta $dc0e

    lda $dc0d
    lda $dc0d
    .repeat 40,count
    lda $dc04
    sta $0400+count
    .endrepeat

    lda #5
.if (TESTVAL = 4)
    .if (CIA = 1)
    ; we should not come here
    lda #2
    .endif
.endif
    sta $d020

    jmp continue1

continue2:
    sei
    lda $dc0d
    pla
    pla
    pla

    lda #5
    sta $d020

continue1:
    lda #$7f
    sta $dc0d

    inc $0400+(24*40)+39

.if (TESTVAL = 255) | (TESTVAL = 4)
    ldx #39
tstlp1:
    ldy #5
    lda refdata,x
    sta $0400+40,x
    cmp $0400,x
    beq tstsk1
    ldy #2
    sty $d020
tstsk1:
    tya
    sta $d800,x
    dex
    bpl tstlp1
.endif

    lda $d020
    and #$0f
    ldx #0 ; success
    cmp #2
    bne nofail
    ldx #$ff ; failure
nofail:
    stx $d7ff

    jmp mainloop


refdata:
.if (TESTVAL = 4)
    .ifdef ONESHOT
        .if (CIA = 1)
        .byte $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .else
        .byte $04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04,$04
        .endif
    .else
        .if (CIA = 1)
        .byte $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .else
        .byte $04,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .endif
    .endif
.endif
.if (TESTVAL = 255)
;    .if (CIA = 1)
;    .byte $f6,$ee,$e6,$de,$d6,$ce,$c6,$be,$b6,$ae,$a6,$9e,$96,$8e,$86,$7e,$76,$6e,$66,$5e,$56,$4e,$46,$3e,$36,$2e,$26,$1e,$16,$0e,$06,$20,$20,$20,$20,$20,$20,$20,$20,$20
;    .else
    .byte $f6,$ee,$e6,$de,$d6,$ce,$c6,$be,$b6,$ae,$a6,$9e,$96,$8e,$86,$7e,$76,$6e,$66,$5e,$56,$4e,$46,$3e,$36,$2e,$26,$1e,$16,$0e,$06,$20,$20,$20,$20,$20,$20,$20,$20,$20
;    .endif
.endif
