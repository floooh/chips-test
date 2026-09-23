
;-----------------------------------------------------------------------------

    !ct scr

    * = $0801

    ; BASIC stub: "1 SYS 2061"
    !by $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00

lptypeidx=$fa
lptypes=3
    
;-----------------------------------------------------------------------------

start:

    sei
;    lda #$35
;    sta $01

    lda #7
    sta $d020
    lda #1
    sta $d021

    jmp main

    * = $0980
sprite:
          ;12345678  90123456  78901234
    !byte %00000000,%00111110,%00000000 ;1
    !byte %00000000,%00001000,%00000000 ;2
    !byte %00000000,%00001000,%00000000 ;3
    !byte %00000000,%00001000,%00000000 ;4
    !byte %00000000,%00001000,%00000000 ;5
    !byte %00000000,%00001000,%00000000 ;6
    !byte %00000000,%00001000,%00000000 ;7
    !byte %00000000,%00001000,%00000000 ;8
    !byte %00100000,%00001000,%00000010 ;9
    !byte %00100000,%00001000,%00000010 ;10
    !byte %00111111,%11111111,%11111110 ;11
    !byte %00100000,%00001000,%00000010
    !byte %00100000,%00001000,%00000010
    !byte %00000000,%00001000,%00000000
    !byte %00000000,%00001000,%00000000
    !byte %00000000,%00001000,%00000000
    !byte %00000000,%00001000,%00000000
    !byte %00000000,%00001000,%00000000
    !byte %00000000,%00001000,%00000000
    !byte %00000000,%00001000,%00000000
    !byte %00000000,%00111110,%00000000
    
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
    lda #3
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
    lda #'.'
    jmp skp2
skp1:
    lda #'*'
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
    
;-----------------------------------------------------------------------------

main:

    lda #0
    sta lptypeidx

    jsr printscreen

    lda #%10000001
    sta $3fff
    ; First make sure the normal screen is shown.

    bit $d011
    bpl *-3
    bit $d011
    bmi *-3
    lda #$1b
    sta $d011
    
loop:

    lda $d013
    sta adc1val
    
    ldx #<($0400+(40*4)+3)
    ldy #>($0400+(40*4)+3)
    jsr printhex

    lda adc1val
    asl
    ldx #<($0400+(40*5)+3)
    ldy #>($0400+(40*5)+3)
    jsr printhex

    ldy #1
    lda adc1val
    bmi +
    ldy #0
+
    tya
    ldx #<($0400+(40*5)+1)
    ldy #>($0400+(40*5)+1)
    jsr printhex
    
    lda $d014
    sta adc2val
    
    ldx #<($0400+(40*4)+9)
    ldy #>($0400+(40*4)+9)
    jsr printhex

    lda $d419 ; adc 1
    ldx #<($0400+(40*8)+1)
    ldy #>($0400+(40*8)+1)
    jsr printhex
    
    lda $d41a ; adc 2
    ldx #<($0400+(40*8)+7)
    ldy #>($0400+(40*8)+7)
    jsr printhex

    ; setup CIA1 to read joystick #1
    lda #%00000000
    sta $dc02 ; port a ddr (all input)
    sta $dc03 ; port b ddr (all input)

    ; joy port 1
    lda $dc01 ; port b data
    ldx #<($0400+(40*4)+14)
    ldy #>($0400+(40*4)+14)
    jsr printbits

    dec keydelay
    
keydelay=*+1
    lda #1
    beq +
    jmp nokey
+

    lda #20
    sta keydelay
    
    ; setup CIA1 to read keyboard, port A -> port b
    lda #%11111111
    sta $dc02 ; port a ddr (all output)
    lda #%00000000
    sta $dc03 ; port b ddr (all input)
    lda #%01111111
    sta $dc00 ; port a data

    
    lda $dc01 ; port b data
    cmp #%11011111  ; C=
    bne +
    inc lptypeidx
    ldx lptypeidx
    cpx #lptypes
    bcc +
    ldx #0
    stx lptypeidx
+

    lda #%01111111
    sta $dc00 ; port a data

    lda $dc01 ; port b data
    cmp #%01111111  ; run/stop
    bne +

    ldx #lptypes
    stx lptypeidx
    
    lda adc1val
    sec
    sbc #$7c
    asl
    sta compensationx,x
    lda adc2val
    sec
    sbc #$5a
    sta compensationy,x
+

    lda lptypeidx
    ldx #<($0400+(40*20)+13)
    ldy #>($0400+(40*20)+13)
    jsr printhex

    ldx lptypeidx
    lda compensationx,x
    ldx #<($0400+(40*20)+31)
    ldy #>($0400+(40*20)+31)
    jsr printhex

    ldx lptypeidx
    lda compensationy,x
    ldx #<($0400+(40*20)+36)
    ldy #>($0400+(40*20)+36)
    jsr printhex

    ldx lptypeidx
    lda compensationx,x
    clc
    adc #13 ; crosshair midpoint x
    lsr     ; /2
    sta compx
    
    ldx lptypeidx
    lda compensationy,x
    clc
    adc #11 ; crosshair midpoint y
    sta compy

    lda lptypeidx
    asl
    asl
    asl
    asl
    asl
    tax
    ldy #0 
-
    lda typestr,x
    sta $0400+(40*22)+1,y
    inx
    iny
    cpy #32
    bne -

    lda #%10111111
    sta $dc00 ; port a data

    lda $dc01 ; port b data
    cmp #%10111111  ; arrow up
    bne +
    inc bordermode
+
    ldx #%11111111
    ldy #0
bordermode=*+1
    lda #0
    and #1
    bne +
    ldx #%10000001
    ldy #7
+
    stx $3fff
    sty $d020
    
    
nokey:

    lda #1
    sta $d015
    lda #3
    sta $d027
    lda #sprite / 64
    sta $07f8

    ; at the left border of the text screen the value in the latch is $18,
    ; ie 48 "pixels". so to get the crosshair in place we must substract
    ; 24 + xoffs
    lda adc1val
    sec
compx=*+1
    sbc #24 + 16
    asl
    sta $d000
    adc #0
    sta $d010

    ; the value in the latch is the active rasterline, so to get the crosshair
    ; in place we must substract 1 (because sprite starts 1 line later) + yoffs
    lda adc2val
    sec
compy=*+1
    sbc #10 + 1
    sta $d001

    ; For each frame, set screen-mode to 24 lines at y-position $f9 - $fa..

    lda #$f9
    cmp $d012
    bne *-3
    lda $d011
    and #$f7
    sta $d011

    ; .. and below y-position $fc, set it back to 25 lines.
    bit $d011
    bpl *-3
    ora #8
    sta $d011    
    
    jmp loop

;-----------------------------------------------------------------------------

adc1val:
    !by 0,0
adc2val:
    !by 0,0

hextab:
    !scr "0123456789abcdef"
    
typestr:
         ;12345678901234567890123456789012
    !scr "ideal                           "
    !scr "lightpen (gpz)                  "
    !scr "magnum light phaser             "
    !scr "custom calibrated               "

compensationx:
    !byte 0
    !byte 28
    !byte 72
    !byte 0
compensationy:
    !byte 0
    !byte 0
    !byte 0
    !byte 0
    
screen:
         ;0123456789012345678901234567890123456789
    !scr "                                        "
    !scr " lightpen/gun coordinates test          " ;0
    !scr "                                        " ;1
    !scr " xpos  ypos   port1                     " ;2
    !scr "   00    00   ........     ",100,101," < 7c,5a   " ;3
    !scr " 0000            43210      ",101,"           " ;4
    !scr "                                        " ;5
    !scr " adc1  adc2                             " ;6
    !scr " 00    00                               "
    !scr "                                        "
    !scr " 4  joy fire                mouse left  "
    !scr " 3  joy right paddle 2 fire             "
    !scr " 2  joy left  paddle 1 fire             "
    !scr " 1  joy down                            "
    !scr " 0  joy up                  mouse right "
    !scr "                                        "
    !scr " this will currently work only with     "
    !scr " lightguns. remember coordinates will   "
    !scr " actually be slightly off!              "
    !scr "                                        "
    !scr " cbm > type: 00 compensation x:00 y:00  "
    !scr "                                        "
    !scr " ideal                                  "
    !scr "                                        "
    !scr "                                        "

    
