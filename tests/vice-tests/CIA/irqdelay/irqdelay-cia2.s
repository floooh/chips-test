
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
    ; test CIA2 (NMI)

    lda #$ff
    sta $dd04
    sta $dd05

    lda #%10010000 ; stop timer
    sta $dd0e

    ; Set NMI vector
    lda #<continue1
    sta $fffa
    lda #>continue1
    sta $fffb

    ldx #$7f
    lda #$81
    sta $dd0d

    ; Set timer to $100 cycles
    lda #TESTVAL
    sta $dd04
    lda #0
    sta $dd05

.ifdef ONESHOT
    lda #%10011001 ; Fire a 1-shot timer
.else
    lda #%10010001 ; normal timer
.endif
    sta $dd0e

    lda $dd0d
    lda $dd0d
    .repeat 40,count
    lda $dd04
    sta $0400+count
    .endrepeat

    ; we should not come here
    lda #2
    sta $d020

    jmp continue2

continue1:
    stx $dd0d
    lda $dd0d
    pla
    pla
    pla

    lda #5
    sta $d020

continue2:
    lda #$7f
    sta $dd0d

    inc $0450
    
.if (TESTVAL = 255)
    ldx #$1e
tstlp1:
    ldy #2
    lda $0400,x
    cmp refdata,x
    bne tstsk1
    ldy #5
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

.if (TESTVAL = 255)
refdata:
    .byte $f6,$ee,$e6,$de,$d6,$ce,$c6,$be,$b6,$ae,$a6,$9e,$96,$8e,$86,$7e,$76,$6e,$66,$5e,$56,$4e,$46,$3e,$36,$2e,$26,$1e,$16,$0e,$06
.endif
