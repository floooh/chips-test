; test written by andreas boose, extracted from D011TEST.D64

    * = $0801
    !byte $0b,$08,$01,$00,$9e
    !convtab pet
    !tx "2061"
    !byte $00,$00,$00
    jmp start


    !align 255,0,0
start:
    sei
l2:
    lda #$ff
    cmp $d012
    bne *-3

    lda #$00
    sta $dd0e
    lda #$40
    sta $dd04
    lda #$00
    sta $dd05
    lda #$01
    sta $dd0e

    ldx #$0
l1:
    lda $dd04
    sta $0400,x
    inx
    lda $dd05
    sta $0400,x
    inx
    lda #$00
    sta $dd0e
    lda $dd04
    sta $0400,x
    inx
    lda $dd05
    sta $0400,x
    lda #$01
    sta $dd0e
    inx
    cpx #$50
    bne l1

    jmp l2
