TESTING = 0

    .export Start

Start:

    sei
    lda #$35
    sta $01
    lda #$c9
    sta $d016

    lda #$01
    sta $d01a
    lda #$7f
    sta $dc0d
    lda #$1b
    sta $d011
    lda #$20
    sta $d012
    lda #>irq
    sta $ffff
    lda #<irq
    sta $fffe

    lda $dc0d
    sta $dc0d
    inc $d019
    cli

    lda #0
    sta $d020
    sta $d021

    ldx #0
@lp1:
    lda #$20
    sta $0400,x
    sta $0500,x
    sta $0600,x
    sta $06e8,x
    lda #$1
    sta $d800,x
    sta $d900,x
    sta $da00,x
    sta $dae8,x
    inx
    bne @lp1

    ldx #0
@lp:
    lda #$a0
    sta $0400,x
    sta $0428,x
    lda cols,x
    sta $d800,x
    sta $d828,x
    inx
    cpx #40
    bne @lp

jitter:
    nop         ; 2
    bit $ea     ; 3
    bit $eaea   ; 4

    lda ($ff),y ; 5+1
    lda ($ff,x) ; 6
.if TESTING = 1
    inc $06e8,x ; 7
.endif
    inx
    iny

    jmp jitter

cols:
    .byte 1,1,1,1,1,1
    .byte 2,2,2,2,2,2
    .byte 3,3,3,3,3,3
    .byte 4,4,4,4,4,4
    .byte 5,5,5,5,5,5
    .byte 6,6,6,6,6,6
    .byte 7,7,7,7,7,7

    .import rastersync_lp

tmp = $f0

irq:
    pha
    txa
    pha
    tya
    pha

    lda $d013
    sta tmp
    lda $d014
    sta tmp+1

    jsr rastersync_lp

    ldx #6
@lp1:
    dex
    bne @lp1
    nop
    nop

    lda #0
    ldx #14
@lp:
    inc $d020
    inc $d020
    inc $d020
    inc $d020
    inc $d020
    inc $d020
    inc $d020
    inc $d020
    inc $d020
    sta $d020
    dex
    bne @lp

    inc $d019
.if TESTING = 1
    lda tmp
    and #$0f
    tax
    lda hextab,x
    sta $0401
    
    lda tmp
    lsr
    lsr
    lsr
    lsr
    and #$0f
    tax
    lda hextab,x
    sta $0400

    lda tmp
    tax
    sta $0400+(4*40),x
    
    lda tmp+1
    and #$0f
    tax
    lda hextab,x
    sta $0404
    
    lda tmp+1
    lsr
    lsr
    lsr
    lsr
    and #$0f
    tax
    lda hextab,x
    sta $0403
.endif
    nop         ; 2
    bit $ea     ; 3
    bit $eaea   ; 4

    lda ($ff),y ; 5+1
    lda ($ff,x) ; 6
.if TESTING = 1
    inc $06e8,x ; 7
.endif
    bne *+2

    inx
    iny

    dec framecount
    bne skp
    lda #0
    sta $d7ff
skp:
    
    pla
    tay
    pla
    tax
    pla
    rti

hextab:
    .byte "0123456789",1,2,3,4,5,6,7,8
framecount:
    .byte 5
