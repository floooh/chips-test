
;-----------------------------------------------------------------------------

    !ct scr

    * = $0801

    ; BASIC stub: "1 SYS 2061"
    !by $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00

;-----------------------------------------------------------------------------

start:

    sei
;    lda #$35
;    sta $01

    lda #6
    sta $d020
    sta $d021

    jmp main

;-----------------------------------------------------------------------------

printscreen:
    ldx #0
lp:
    lda screen,x
    sta $0400,x
    lda screen+$0100,x
    sta $0500,x
    lda screen+$0200,x
    sta $0600,x
    lda screen+$02e8,x
    sta $06e8,x
    lda #1
    sta $d800,x
    sta $d900,x
    sta $da00,x
    sta $dae8,x
    inx
    bne lp
    rts

printbits:
    sta hlp+1
    stx hlp2+1
    sty hlp2+2

    ldy #0

bitlp:
hlp:
    lda #0
    asl hlp+1
    bcc skp1
    lda #'*'
    jmp skp2
skp1:
    lda #'.'
skp2:
hlp2:
    sta $dead,y
    iny
    cpy #8
    bne bitlp

    rts

ptr = $02

printhex:
    stx ptr
    sty ptr+1
    ldy #1
    pha
    and #$0f
    tax
    lda hextab,x
    sta (ptr),y
    dey
    pla
    lsr
    lsr
    lsr
    lsr
    tax
    lda hextab,x
    sta (ptr),y
    rts

delay:
    ldx #0
    dex
    bne *-1
    rts

;-----------------------------------------------------------------------------

main:

    jsr printscreen

loop:

    ; setup CIA1, port A -> port b
    lda #%11111111
    sta $dc02 ; port a ddr (all output)
    lda #%00000000
    sta $dc03 ; port b ddr (all input)
    lda #$00
    sta $dc00 ; port a data

    jsr delay

    lda $dc00 ; port a
    sta result+0
    pha
    ldx #<($0400+(40*3)+10)
    ldy #>($0400+(40*3)+10)
    jsr printhex
    pla
    ldx #<($0400+(40*3)+20)
    ldy #>($0400+(40*3)+20)
    jsr printbits

    lda $dc01 ; port b
    sta result+1
    pha
    ldx #<($0400+(40*3)+14)
    ldy #>($0400+(40*3)+14)
    jsr printhex
    pla
    ldx #<($0400+(40*3)+30)
    ldy #>($0400+(40*3)+30)
    jsr printbits

    ; setup CIA1, port B -> port A

    lda #%00000000
    sta $dc02 ; port a ddr (all input)
    lda #%11111111
    sta $dc03 ; port b ddr (all output)
    lda #$00
    sta $dc01 ; port b data

    jsr delay

    lda $dc00 ; port a
    sta result+2
    pha
    ldx #<($0400+(40*4)+10)
    ldy #>($0400+(40*4)+10)
    jsr printhex
    pla
    ldx #<($0400+(40*4)+20)
    ldy #>($0400+(40*4)+20)
    jsr printbits

    lda $dc01 ; port b
    sta result+3
    pha
    ldx #<($0400+(40*4)+14)
    ldy #>($0400+(40*4)+14)
    jsr printhex
    pla
    ldx #<($0400+(40*4)+30)
    ldy #>($0400+(40*4)+30)
    jsr printbits

    ; setup CIA1, both in

    lda #%00000000
    sta $dc02 ; port a ddr (all input)
    lda #%00000000
    sta $dc03 ; port b ddr (all input)

    jsr delay

    lda $dc00 ; port a
    sta result+4
    pha
    ldx #<($0400+(40*5)+10)
    ldy #>($0400+(40*5)+10)
    jsr printhex
    pla
    ldx #<($0400+(40*5)+20)
    ldy #>($0400+(40*5)+20)
    jsr printbits

    lda $dc01 ; port b
    sta result+5
    pha
    ldx #<($0400+(40*5)+14)
    ldy #>($0400+(40*5)+14)
    jsr printhex
    pla
    ldx #<($0400+(40*5)+30)
    ldy #>($0400+(40*5)+30)
    jsr printbits

    ; setup CIA1, both out ($00,$00)

    lda #%11111111
    sta $dc02 ; port a ddr (all output)
    lda #%11111111
    sta $dc03 ; port b ddr (all output)
    lda #$00
    sta $dc00 ; port a data
    sta $dc01 ; port b data

    jsr delay

    lda $dc00 ; port a
    sta result+6
    pha
    ldx #<($0400+(40*6)+10)
    ldy #>($0400+(40*6)+10)
    jsr printhex
    pla
    ldx #<($0400+(40*6)+20)
    ldy #>($0400+(40*6)+20)
    jsr printbits

    lda $dc01 ; port b
    sta result+7
    pha
    ldx #<($0400+(40*6)+14)
    ldy #>($0400+(40*6)+14)
    jsr printhex
    pla
    ldx #<($0400+(40*6)+30)
    ldy #>($0400+(40*6)+30)
    jsr printbits

    ; setup CIA1, both out (A->B)

    lda #%11111111
    sta $dc02 ; port a ddr (all output)
    lda #%11111111
    sta $dc03 ; port b ddr (all output)
    lda #$00
    sta $dc00 ; port a data
    lda #$ff
    sta $dc01 ; port b data

    jsr delay

    lda $dc00 ; port a
    sta result+8
    pha
    ldx #<($0400+(40*7)+10)
    ldy #>($0400+(40*7)+10)
    jsr printhex
    pla
    ldx #<($0400+(40*7)+20)
    ldy #>($0400+(40*7)+20)
    jsr printbits

    lda $dc01 ; port b
    sta result+9
    pha
    ldx #<($0400+(40*7)+14)
    ldy #>($0400+(40*7)+14)
    jsr printhex
    pla
    ldx #<($0400+(40*7)+30)
    ldy #>($0400+(40*7)+30)
    jsr printbits

    ; setup CIA1, both out (B->A)

    lda #%11111111
    sta $dc02 ; port a ddr (all output)
    lda #%11111111
    sta $dc03 ; port b ddr (all output)
    lda #$ff
    sta $dc00 ; port a data
    lda #$00
    sta $dc01 ; port b data

    jsr delay

    lda $dc00 ; port a
    sta result+10
    pha
    ldx #<($0400+(40*8)+10)
    ldy #>($0400+(40*8)+10)
    jsr printhex
    pla
    ldx #<($0400+(40*8)+20)
    ldy #>($0400+(40*8)+20)
    jsr printbits

    lda $dc01 ; port b
    sta result+11
    pha
    ldx #<($0400+(40*8)+14)
    ldy #>($0400+(40*8)+14)
    jsr printhex
    pla
    ldx #<($0400+(40*8)+30)
    ldy #>($0400+(40*8)+30)
    jsr printbits

    ; setup CIA1, both out ($ff,$ff)

    lda #%11111111
    sta $dc02 ; port a ddr (all output)
    lda #%11111111
    sta $dc03 ; port b ddr (all output)
    lda #$ff
    sta $dc00 ; port a data
    sta $dc01 ; port b data

    jsr delay

    lda $dc00 ; port a
    sta result+12
    pha
    ldx #<($0400+(40*9)+10)
    ldy #>($0400+(40*9)+10)
    jsr printhex
    pla
    ldx #<($0400+(40*9)+20)
    ldy #>($0400+(40*9)+20)
    jsr printbits

    lda $dc01 ; port b
    sta result+13
    pha
    ldx #<($0400+(40*9)+14)
    ldy #>($0400+(40*9)+14)
    jsr printhex
    pla
    ldx #<($0400+(40*9)+30)
    ldy #>($0400+(40*9)+30)
    jsr printbits

    ldy #5
    ldx #0
l1
    lda result,x
    cmp resultspc,x
    beq nofail1
    ldy #2
nofail1
    inx
    cpx #2*7
    bne l1

    tya
    ldx #0
l1a
    sta $d800+(40*11),x
    inx
    cpx #40
    bne l1a

    ldy #5
    ldx #0
l2
    lda result,x
    cmp resultshiftlock,x
    beq nofail2
    ldy #2
nofail2
    inx
    cpx #2*7
    bne l2

    tya
    ldx #0
l2a
    sta $d800+(40*13),x
    inx
    cpx #40
    bne l2a

    ldy #5
    ldx #0
l3
    lda result,x
    cmp resultleftshift,x
    beq nofail3
    ldy #2
nofail3
    inx
    cpx #2*7
    bne l3

    tya
    ldx #0
l3a
    sta $d800+(40*15),x
    inx
    cpx #40
    bne l3a

    jmp loop

;-----------------------------------------------------------------------------

result:
    !byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0

resultspc:
    !byte $00,$ef,$7f,$00,$ff,$ff,$00,$00,$00,$ff,$7f,$00,$ff,$ff
resultshiftlock:
    !byte $00,$7f,$fd,$00,$ff,$ff,$00,$00,$00,$7f,$fd,$00,$ff,$ff
resultleftshift:
    !byte $00,$7f,$fd,$00,$ff,$ff,$00,$00,$00,$ff,$fd,$00,$ff,$ff

hextab:
    !scr "0123456789abcdef"

screen:
         ;0123456789012345678901234567890123456789
    !scr "cia keyboard ports test                 " ;0
    !scr "                                        "
    !scr "          pa  pb    pa        pb        "
    !scr "pa->pb      ->                          "
    !scr "pb->pa      <-                          "
    !scr "both in                                 "
    !scr "both out                                "
    !scr "a lo b hi   ->                          "
    !scr "b hi a lo   <-                          "
    !scr "                                        "
    !scr "                                        "
    !scr "press space                             "
    !scr "                                        "
    !scr "press shift lock                        "
    !scr "                                        "
    !scr "press left shift                        "
    !scr "                                        "
    !scr "                                        "
    !scr "                                        "
    !scr "                                        "
    !scr "                                        "
    !scr "                                        "
    !scr "                                        "
    !scr "for details look at readme.txt          "
    !scr "                                        "
