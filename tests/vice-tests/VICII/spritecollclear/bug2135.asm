

    *= $0801
    !byte $0b,$08,$01,$00,$9e ; Line 1 SYS2061
    !convtab pet
    !tx "2061"                ; Address for sys start in text
    !byte $00,$00,$00

start:
    sei

    ldx #0
-
    lda #$20
    sta $0400,x
    lda #$01
    sta $d800,x
    inx
    bne -

    ; setup a sprite
    lda #$ff
    ldx #63
-
    sta 10240,x
    dex
    bpl -
    lda #160
    sta 2040
    lda #100
    ldx #3
-
    sta 53248,x
    dex
    bpl -
;30 poke53273,255:i=peek(53278)
    lda #255
    sta 53273
    lda 53278
;40 poke53269,3
    lda #3
    sta 53269
;50 fori=0to200:next
    jsr wait
;60 poke53269,0
    lda #0
    sta 53269
;70 print"should be 4: ",peek(53273)and4
    lda 53273
    sta $0400
    and #4
    sta $0400+40
;80 print"should be 3: ",peek(53278)and3
    lda 53278
    sta $0401
    and #3
    sta $0401+40
;90 print"should still be 4: ",peek(53273)and4
    lda 53273
    sta $0402
    and #4
    sta $0402+40

    ldx #2
-
    lda $0400+40,x
    cmp reference,x
    bne fail
    dex
    bpl -

    lda #5
    sta $d020
    lda #$00
    sta $d7ff
    jmp *
fail:
    lda #10
    sta $d020
    lda #$ff
    sta $d7ff
    jmp *

wait:
-   lda $d011
    bpl -
-   lda $d011
    bmi -
-   lda $d011
    bpl -
-   lda $d011
    bmi -
    rts

reference:
    !byte 4, 3, 4
