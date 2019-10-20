
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

printbits_h:
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

printbits_v: 

    sta hlpa+1
    stx hlp2a+1
    sty hlp2a+2

    ldy #0

bitlpa:
hlpa:
    lda #0
    lsr hlpa+1
    bcc skp1a
    lda #'*'
    jmp skp2a
skp1a:
    lda #'.'
skp2a:
hlp2a:
    sta $dead
    lda hlp2a+1
    clc
    adc #40
    sta hlp2a+1
    lda hlp2a+2
    adc #0
    sta hlp2a+2
    iny
    cpy #8
    bne bitlpa

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

    !for i, 8 {
    lda # !(1 << (i-1)) & $ff
    sta $dc00 ; port a data
    jsr delay
    lda $dc01 ; port b
    ldx #<($0400+(40*2)+0+((i-1)*40))
    ldy #>($0400+(40*2)+0+((i-1)*40))
    jsr printbits_h
    }

    lda #$ff
    sta $dc00 ; port a data
    sta $dc02 ; port a ddr

    ; setup CIA1, port A -> port b (inactive bits input)
    lda #%11111111
    sta $dc02 ; port a ddr (all output)
    lda #%00000000
    sta $dc03 ; port b ddr (all input)

    !for i, 8 {
    lda #%11111111
    sta $dc02 ; port a ddr (all output)
    lda # !(1 << (i-1)) & $ff
    sta $dc00 ; port a data
    lda #(1 << (i-1)) & $ff
    sta $dc02 ; port a ddr (active output)
    jsr delay
    lda $dc01 ; port b
    ldx #<($0400+(40*2)+10+((i-1)*40))
    ldy #>($0400+(40*2)+10+((i-1)*40))
    jsr printbits_h
    }

    lda #$ff
    sta $dc00 ; port a data
    sta $dc02 ; port a ddr

    
    ; setup CIA1, port B -> port A

    lda #%00000000
    sta $dc02 ; port a ddr (all input)
    lda #%11111111
    sta $dc03 ; port b ddr (all output)

    !for i, 8 {
    lda # !(1 << (i-1)) & $ff
    sta $dc01 ; port b data
    jsr delay
    lda $dc00 ; port a
    ldx #<($0400+(40*2)+22+(7-(i-1)))
    ldy #>($0400+(40*2)+22+(7-(i-1)))
    jsr printbits_v
    }

    lda #$ff
    sta $dc03 ; port b ddr
    sta $dc01 ; port b data
    
    ; setup CIA1, port B -> port A (inactive input)

    lda #%00000000
    sta $dc02 ; port a ddr (all input)
    lda #%11111111
    sta $dc03 ; port b ddr (all output)

    !for i, 8 {
    lda #%11111111
    sta $dc03 ; port b ddr (all output)
    lda # !(1 << (i-1)) & $ff
    sta $dc01 ; port b data
    lda #(1 << (i-1)) & $ff
    sta $dc03 ; port b ddr (all output)
    jsr delay
    lda $dc00 ; port a
    ldx #<($0400+(40*2)+32+(7-(i-1)))
    ldy #>($0400+(40*2)+32+(7-(i-1)))
    jsr printbits_v
    }

    lda #$ff
    sta $dc03 ; port b ddr
    sta $dc01 ; port b data

    ; setup CIA1, port B -> port b

    lda #%00000000
    sta $dc02 ; port a ddr (all input)
    lda #%00000000
    sta $dc03 ; port b ddr (all input)

    !for i, 8 {
    lda # (1 << (i-1)) & $ff
    sta $dc03 ; port b ddr (1 bit output)
    lda # !(1 << (i-1)) & $ff
    sta $dc01 ; port b data
    jsr delay
    lda $dc01 ; port b
    ldx #<($0400+(40*13)+0+(7-(i-1)))
    ldy #>($0400+(40*13)+0+(7-(i-1)))
    jsr printbits_v
    }

    lda #$ff
    sta $dc03 ; port a ddr
    sta $dc01 ; port b data
loop2:
    ; Port A outputs (active) low, Port B outputs high.
    ; scanning with port a, inactive bits are high and input

    lda #%11111111
    sta $dc02 ; port a ddr (all output)
    sta $dc03 ; port b ddr (all output)

    sta $dc01 ; port b
    
    !for i, 8 {
    lda #$ff
    sta $dc02 ; port a ddr (all output)
    lda # !(1 << (i-1)) & $ff
    sta $dc00 ; port a data
    lda # (1 << (i-1)) & $ff
    sta $dc02 ; port a ddr (inactive input)
    jsr delay
    lda $dc01 ; port b
    ldx #<($0400+(40*13)+10+((i-1)*40))
    ldy #>($0400+(40*13)+10+((i-1)*40))
    jsr printbits_h
    }

    lda #$ff
    sta $dc00 ; port a data
    sta $dc01 ; port b data

;    jmp loop2
    
    ; setup CIA1, port A -> port a
    lda #%00000000
    sta $dc02 ; port a ddr (all input)
    lda #%00000000
    sta $dc03 ; port b ddr (all input)

    !for i, 8 {
    lda # (1 << (i-1)) & $ff
    sta $dc02 ; port a ddr (1 bit output)
    lda # !(1 << (i-1)) & $ff
    sta $dc00 ; port a data
    jsr delay
    lda $dc00 ; port a
    ldx #<($0400+(40*13)+22+((i-1)*40))
    ldy #>($0400+(40*13)+22+((i-1)*40))
    jsr printbits_h
    }

    lda #$ff
    sta $dc02 ; port a ddr
    sta $dc00 ; port a data
    
    ; Port A outputs (active) low, Port B outputs high.
    ; scanning with port b, inactive bits are low

    lda #%11111111
    sta $dc02 ; port a ddr (all output)
    sta $dc03 ; port b ddr (all output)

    lda #0
    sta $dc00 ; port A
    
    !for i, 8 {
;    lda #%11111111
;    sta $dc03 ; port b ddr (all output)
    lda # !(1 << (i-1)) & $ff
    sta $dc01 ; port b data
;    lda # (1 << (i-1)) & $ff
;    sta $dc03 ; port b ddr (inactive input)
    jsr delay
    lda $dc00 ; port a
    ldx #<($0400+(40*13)+32+(7-(i-1)))
    ldy #>($0400+(40*13)+32+(7-(i-1)))
    jsr printbits_v
    }

    lda #$ff
    sta $dc00 ; port a data
    sta $dc01 ; port b data
    
    jmp loop

;-----------------------------------------------------------------------------


hextab:
    !scr "0123456789abcdef"

screen:
         ;0123456789012345678901234567890123456789
    !scr "pa->pb    pa->pb      pb->pa    pb->pa  "
    !scr "                                        "
    !scr "xxxxxxxx  xxxxxxxx    xxxxxxxx  xxxxxxxx"
    !scr "                                        "
    !scr "                                        "
    !scr "                                        "
    !scr "                                        "
    !scr "                                        "
    !scr "                                        "
    !scr "xxxxxxxx  xxxxxxxx    xxxxxxxx  xxxxxxxx"
    !scr "                                        "
    !scr "pb->pb    pa->pb      pa->pa    pb->pa  "
    !scr "                                        "
    !scr "xxxxxxxx  xxxxxxxx    xxxxxxxx  xxxxxxxx"
    !scr "                                        "
    !scr "                                        "
    !scr "                                        "
    !scr "                                        "
    !scr "                                        "
    !scr "                                        "
    !scr "xxxxxxxx  xxxxxxxx    xxxxxxxx  xxxxxxxx"
    !scr "                                        "
    !scr "                                        "
    !scr "                                        "
    !scr "for details look at readme.txt          "
