
    !to "lpcart.bin",plain

    *=$e000

    !word start
    !word start

    !byte $C3, $C2, $CD, $38, $30

start:

    sei
;    lda #$35
;    sta $01

    lda #21
    sta $d018

    lda #1
    sta $d020
    lda #2
    sta $d021

    ldx #$ff
    txs

    ldx #0
.lphh:
    lda rastersync_lp,x
    sta $0100,x
    inx
    cpx #$80
    bne .lphh

    lda #$11
    sta $dc0e
    lda #$08
    sta $dc0f


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
.lp1:
    lda #$20
    sta $0400,x
    sta $0500,x
    sta $0600,x
    sta $06e8,x
    lda #$0
    sta $d800,x
    sta $d900,x
    sta $da00,x
    sta $dae8,x
    inx
    bne .lp1

    ldx #0
.lp:
    lda #$a0
    sta $0400,x
    sta $0428,x
    lda cols,x
    sta $d800,x
    sta $d828,x
    inx
    cpx #40
    bne .lp

jitter:
    nop         ; 2
    bit $ea     ; 3
    bit $eaea   ; 4

    lda ($ff),y ; 5+1
    lda ($ff,x) ; 6

    lsr $06e8,x ; 7

    inx
    iny

    jmp jitter

cols:
    !byte 1,1,1,1,1,1
    !byte 2,2,2,2,2,2
    !byte 3,3,3,3,3,3
    !byte 4,4,4,4,4,4
    !byte 5,5,5,5,5,5
    !byte 6,6,6,6,6,6
    !byte 7,7,7,7,7,7


  * = $e600

irq:
    pha
    txa
    pha
    tya
    pha

;    jsr rastersync_lp
    jsr $0100

    ldx #6
.lp1a:
    dex
    bne .lp1a
    nop
    nop

    lda #0
    ldx #14
.lpa:
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
    bne .lpa

    inc $d019

    pla
    tay
    pla
    tax
    pla
    rti

    lda $d013
    and #$0f
    tax
    lda hextab,x
    sta $0401
    
    lda $d013
    lsr
    lsr
    lsr
    lsr
    and #$0f
    tax
    lda hextab,x
    sta $0400
    
    lda $d014
    and #$0f
    tax
    lda hextab,x
    sta $0404
    
    lda $d014
    lsr
    lsr
    lsr
    lsr
    and #$0f
    tax
    lda hextab,x
    sta $0403
    

    pla
    tay
    pla
    tax
    pla
    rti

hextab:
    !scr "0123456789abcdef"

  * = $e800

; the lightpen is usually connected to pin 6 of joystick port 1, which is the
; same as used for fire on a regular joystick (and space on the keybpard)
; this line is then directly connected to both the cia (bit 4 of cia1 port B)
; and the lightpen input of the vic, which means that the lightpen line of the
; vic can be artificially written to by toggling said cia port bit.

rastersync_lp:
         ; acknowledge vic irq
         lda $d019
         sta $d019

         ldx #$ff
         ldy #$00
         ; prepare cia ports
         stx $dc00     ; port A = $ff (inactive)
         sty $dc02     ; ddr A = $00  (all input)
         stx $dc03     ; ddr B = $ff  (all output)
         stx $dc01     ; port B = $ff (inactive)
         ; now trigger the lp latch
         sty $dc01     ; port B = $00 (active)
         stx $dc01     ; port B = $ff (inactive)
         lda $d013     ; get x-position (pixels, divided by two)
         ; restore cia setup
         stx $dc02     ; ddr A = $ff  (all output)
         sty $dc03     ; ddr B = $00  (all input)
         stx $dc01     ; port B = $ff (inactive)
         ldx #$7f
         stx $dc00     ; port A = $7f

         ; divide x-pos by 4 to get x-position in cycles (0..62)
         lsr
         lsr
         ; delay 62 - n cycles
         lsr
         sta (timeout+1)-$e700
         bcc timing   ; 2, +1 extra cycle if even
timing:  clv
timeout: bvc timeout
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
         nop

         rts

  *=$fffa
  !word irq
  !word start
  !word irq
