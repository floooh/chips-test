; test written by andreas boose, extracted from D011TEST.D64

    * = $0801
    !byte $0b,$08,$01,$00,$9e
    !convtab pet
    !tx "2061"
    !byte $00,$00,$00
    jmp start

dauer = $ac
v3fff  = $bd
scr   = 8

    !align 255,0,0
start:
    lda #$7f
    sta $dc0d
    lda #<irq1
    sta $0314
    lda #>irq1
    sta $0315
    lda #$ff
    sta $dd03
    lda #$0f
    sta $d020
    lda #6
    sta $dd00
    ldx #0
    lda #$00
    ldy #$40
l12:
    sta $4000,x
    inx
    bne l12
    inc l12+2
    dey
    bne l12
    lda #$40
    sta l12+2
    lda #$33
    sta $01
    ldx #$10
    ldy #0
l16:
    lda $d000,y
l26:
    sta $5000,y
    iny
    bne l16
    inc l16+2
    inc l26+2
    dex
    bne l16
    lda #$d0
    sta l16+2
    lda #$50
    sta l26+2
    lda #$37
    sta $01
    jsr clear
    lda #$00
    sta $ae
    lda #$44
    sta $af
    lda #0
    sta $f7
    sta $f8
    sta $c5
    sta $c6
    lda #$80
    sta $028A

    ldx #0
l1234:
    txa
    sta $d800,x
    sta $d900,x
    sta $da00,x
    sta $db00,x
    inx
    bne l1234
    lda $d012
    bne *-3
    lda #0
    sta dauer
    lda #$18
    sta $d011
    lda #30
    sta $d012
    lda $d019
    sta $d019
    lda #$81
    sta $d01a

l3:
    jsr $ffe4

    cmp #29
    beq l23
    cmp #157
    beq l23
    cmp #17
    beq l23
    cmp #145
    bne l18
l23:
    sta $f7
l18:
    cmp #136
    bne l4
    inc dauer
    jmp l3
l4:
    cmp #135
    bne l5
    dec dauer
    jmp l3
l5:
    cmp #95
    bne l8
    inc $d021
    jmp l3
l8:
    cmp #133
    bne l30
    ldy #0
    lda ($ae),y
    clc
    adc #1
    sta ($ae),y
    jmp l3
l30:
    cmp #137
    bne l31
    ldy #0
    lda ($ae),y
    sec
    sbc #1
    sta ($ae),y
    jmp l3
l31:
    cmp #134
    bne l32
    ldy #0
    lda ($ae),y
    sta $f8
    jmp l3
l32:
    cmp #138
    bne l33
    jsr clear
    jmp l3
l33:
    cmp #" "
    bne l3
    lda #0
    sta $d01a
    lda #$31
    sta $0314
    lda #$ea
    sta $0315
    lda #$81
    sta $dc0d
    lda #$1b
    sta $d011
    inc $dd00
    rts

clear:
    ldx #0
    stx $f9
    lda #$44
    sta $fa
l15:
    ldy #39
    txa
l14:
    sta ($f9),y
    dey
    bpl l14
    lda $f9
    clc
    adc #40
    sta $f9
    bcc *+4
    inc $fa
    inx
    cpx #25
    bne l15
    ldy #23
    lda #25
    sta $47e8,y
    dey
    bpl *-4
    rts

;.fill $c200-*,0
    !align 255,0, 0

irq1:
    lda #<irq2
    sta $0314
    lda #00
    bit $ffff
    ldx $d012
    inx
    inx
    stx $d012
    dec $d019
    cli
    ldx #$0a
    dex
    bne *-1
    nop
    nop
    nop
    nop
    jmp $ea81

irq2:
    lda #<irq1
    sta $0314
    dec $d019
    inc $d020
    bit $ff
    lda $d012 ;4
    cmp $d012 ;4
    bne *+2   ;2

    lda dauer ;3
    lsr       ;2
    bcs *+2   ;2
    lsr       ;2
    bcs *+2   ;2
    bcs *+2   ;2
    lsr       ;2
    bcs *+2   ;2
    bcs *+2   ;2
    bcs *+2   ;2
    bcs *+2   ;2
    tax       ;2
l2:
    bit $ff   ;3
    dex       ;2
    bpl l2    ;2

    bit $ffff
    bit $ffff
    bit $ffff
    bit $ffff
    bit $ff

    inc $d011
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    dec $d011
    dec $d020
    lda #$53+7
    sta $d012

    lda $f7
    beq i1
    lda $f8
    ldy #0
    sta ($ae),y
    lda $f7
    cmp #29
    bne l22
    inc $ae
    bne *+4
    inc $af
    jmp l20
l22:
    cmp #157
    bne l25
    lda $ae
    bne *+4
    dec $af
    dec $ae
    jmp l20
l25:
    cmp #17
    bne l24
    lda $ae
    clc
    adc #40
    sta $ae
    bcc *+4
    inc $af
    jmp l20
l24:
    cmp #145
    bne l20
    lda $ae
    sec
    sbc #40
    sta $ae
    bcs *+4
    dec $af
l20:
    lda ($ae),y
    sta $f8
    lda #$ff
    sta ($ae),y
    sty $f7
i1:
    ldx #0
    dex
    bne *-1
    jmp $ea31

