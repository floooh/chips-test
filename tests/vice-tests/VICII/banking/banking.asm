        *=$0801
        ; BASIC stub: "1 SYS 2061"
        !byte $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00
start:  jmp main

baseline = $32

main:
        jsr preparescreens

        sei
        lda #$1b
        sta $d011
        lda #$c8
        sta $d016
        lda #$15        ; screen: $0400 char: $1000
        sta $d018

-       lda $d011
        bpl -
-       lda $d011
        bmi -

mainlp:

        lda #0
        sta $d020
        sta $d021

        ldx #8
        jsr rastersync
        inc $d020

        lda #$3f        ; bits 0-1 output
        sta $dd02

        lda #3
        sta $dd00

        ldx #2
        lda #baseline + (2 * 8)
-       cmp $d012
        bne -
        stx $d020

        lda #2
        sta $dd00

        lda #baseline + (4 * 8)
-       cmp $d012
        bne -
        inc $d020

        lda #1
        sta $dd00

        lda #baseline + (6 * 8)
-       cmp $d012
        bne -
        inc $d020

        lda #0
        sta $dd00
        ldy #1
        ldx #$0f
        lda #baseline + (8 * 8)
-       cmp $d012
        bne -
        
        sty $d021
        stx $d020

        ;------------------------------

        lda #0
        sta $dd00

        lda #$3c | $00
        sta $dd02

        lda #baseline + (10 * 8)
-       cmp $d012
        bne -
        inc $d020

        lda #$3c | $01
        sta $dd02

        lda #baseline + (12 * 8)
-       cmp $d012
        bne -
        inc $d020

        lda #$3c | $02
        sta $dd02

        ldx #8
        lda #baseline + (14 * 8)
-       cmp $d012
        bne -
        stx $d020

        lda #$3c | $03
        sta $dd02

        inx
        lda #baseline + (16 * 8)
-       cmp $d012
        bne -
        inc $d021
        nop
        bit $ea
        stx $d020

        ;------------------------------

        lda #3
        sta $dd00

        lda #$3c | $03
        sta $dd02

        lda #baseline + (18 * 8)
-       cmp $d012
        bne -
        inc $d020

        lda #$3c | $02
        sta $dd02

        lda #baseline + (20 * 8)
-       cmp $d012
        bne -
        inc $d020

        lda #$3c | $01
        sta $dd02

        ldx #13
        lda #baseline + (22 * 8)
-       cmp $d012
        bne -
        bit $eaea
        stx $d020

        lda #$3c | $00
        sta $dd02

        ldx #14
        lda #baseline + (24 * 8)
-       cmp $d012
        bne -
        ;nop
        ;bit $ea
        stx $d020

-       lda $d011
        bpl -
-       lda $d011
        bmi -

        ldx #0 ; success
        stx $d7ff

        jmp mainlp

screen0 = $0400
screen1 = $4400
screen2 = $8400
screen3 = $c400

charset1 = $5000
charset3 = $d000

ptr = $02
tmp = $04

preparescreens:
        ldy #>screen0
        lda #<screen0
        ldx #3
        jsr preparescr
        ldy #>screen1
        lda #<screen1
        ldx #2
        jsr preparescr
        ldy #>screen2
        lda #<screen2
        ldx #1
        jsr preparescr
        ldy #>screen3
        lda #<screen3
        ldx #0
        jsr preparescr

        jsr preparechar

        lda #12
        ldx #0
-
        sta $d800,x
        sta $d900,x
        sta $da00,x
        sta $db00,x
        inx
        bne -

        rts

preparescr:
        sta ptr+0
        sty ptr+1
        stx tmp

        ldx #0
--
        ldy #0
        lda tmp
-
        sta (ptr),y
        iny
        cpy #40
        bne -

        clc
        lda ptr
        adc #40
        sta ptr
        lda ptr+1
        adc #0
        sta ptr+1

        inx
        cpx #25
        bne --

        rts

preparechar:
        sei
        lda #$33
        sta $01

        ldx #0
-
        lda $d000+($30*8),x
        sta charset1,x
        inx
        bne -

        lda #$33
        sta $01

        ldx #0
-
        lda charset1,x
        sta charset3,x
        inx
        bne -

        lda #$35
        sta $01
        rts

;--------------------------------------------------
; simple polling rastersync routine

        *=$0d00 ; align to some page so branches do not cross a page boundary and fuck up the timing

rastersync:

lp1:
          cpx $d012
          bne lp1
          jsr cycles
          bit $ea
          nop
          cpx $d012
          beq skip1
          nop
          nop
skip1:    jsr cycles
          bit $ea
          nop
          cpx $d012
          beq skip2
          bit $ea
skip2:    jsr cycles
          nop
          nop
          nop
          cpx $d012
          bne onecycle
onecycle: rts

cycles:
         ldy #$06
lp2:     dey
         bne lp2
         inx
         nop
         nop
         rts
