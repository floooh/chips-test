
        *= $0801
        !byte $0c, $08, $00, $00, $9e, $20
        !byte $32, $30, $36, $32
        !byte 0,0,0
        *= $080e
start:
        sei
        ldx #$2f
-
        lda vicregs,x
        sta $d000,x
        dex
        bpl -

        lda #$ff
        ldx #$3f
-
        sta 832,x
        dex
        bpl -

        ldx #0
-
        lda #$20
        sta $0400,x
        sta $0500,x
        sta $0600,x
        sta $0700,x
        lda #$01
        sta $d800,x
        sta $d900,x
        sta $da00,x
        sta $db00,x
        inx
        bne -

        lda #13
        sta $07f8
        sta $07f9
        sta $07fa
        sta $07fb
        sta $07fc
        sta $07fd
        sta $07fe
        sta $07ff

mainlp:
-       lda $d011
        bpl -
-       lda $d011
        bmi -

        ldx sprnum
        lda hextab,x
        sta $0400+4

        txa
        asl
        tax

        lda xpos
        sta $d000,x

        lda xpos+1
        and #1
        beq +
        inx
+
        lda $d010
        and msbandtab,x
        ora msbortab,x
        sta $d010

        lda xpos
        and #$0f
        tax
        lda hextab,x
        sta $0400+2
        lda xpos
        lsr
        lsr
        lsr
        lsr
        tax
        lda hextab,x
        sta $0400+1
        lda xpos+1
        and #$01
        tax
        lda hextab,x
        sta $0400+0

        ; a
        lda #%11111101
        sta $dc00
        lda $dc01
        cmp #%11111011
        bne +

        ldx sprnum
        inx
        txa
        and #7
        sta sprnum

-       lda $dc01
        cmp #%11111011
        beq -

        jmp mainlp
+
        ; q
        lda #%01111111
        sta $dc00
        lda $dc01
        cmp #%10111111
        bne +

        lda xpos
        sec
        sbc #1
        sta xpos
        lda xpos+1
        sbc #0
        sta xpos+1

        jmp mainlp
+
        ; w
        lda #%11111101
        sta $dc00
        lda $dc01
        cmp #%11111101
        bne +

        lda xpos
        clc
        adc #1
        sta xpos
        lda xpos+1
        adc #0
        sta xpos+1
+
        jmp mainlp

xpos:
        !byte $00, $00
        
sprnum:
        !byte 7

msbandtab:
        !byte %11111110
        !byte %11111110
        !byte %11111101
        !byte %11111101
        !byte %11111011
        !byte %11111011
        !byte %11110111
        !byte %11110111
        !byte %11101111
        !byte %11101111
        !byte %11011111
        !byte %11011111
        !byte %10111111
        !byte %10111111
        !byte %01111111
        !byte %01111111

msbortab:
        !byte %00000000
        !byte %00000001
        !byte %00000000
        !byte %00000010
        !byte %00000000
        !byte %00000100
        !byte %00000000
        !byte %00001000
        !byte %00000000
        !byte %00010000
        !byte %00000000
        !byte %00100000
        !byte %00000000
        !byte %01000000
        !byte %00000000
        !byte %10000000

hextab:
        !scr "0123456789abcdef"

vicregs:
        !byte   <(47+(0*48)), 60+(0*10)
        !byte   <(47+(1*48)), 60+(1*10)
        !byte   <(47+(2*48)), 60+(2*10)
        !byte   <(47+(3*48)), 60+(3*10)
        !byte   <(47+(4*48)), 60+(4*10)
        !byte   <(47+(5*48)), 60+(5*10)
        !byte   <(47+(6*48)), 60+(6*10)
        !byte              0, 60+(7*10)
        ; d010
        !byte %01100000
        !byte $1b
        !byte 0
        !byte 0
        !byte 0
        !byte $ff
        !byte $c8
        !byte 0
        ; d018
        !byte $15
        !byte 0
        !byte 0
        !byte 0
        !byte 0
        !byte 255
        !byte 0
        !byte 0
        ; d020
        !byte $0f
        !byte 0
        !byte 0
        !byte 0
        !byte 0
        !byte 0
        !byte 0
        !byte 1, 2, 3, 4, 5, 6, 7, 8
        !byte 0, 0
