         *= $0801

         .byte $4c,$16,$08,$00,$97,$32
         .byte $2c,$30,$3a,$9e,$32,$30
         .byte $37,$30,$00,$00,$00,$a9
         .byte $01,$85,$02

         jsr print
         .byte 147,14,13
         .text "Commodore 64 "
         .text "Emulator Test Suite"
         .byte 13
         .text "Public Domain, "
         .text "no Copyright"
         .byte 13,13
         .null "basic commands"

e        .macro
         jsr print
         .text " - block "
         .text "`1"
         .text " failed"
         .byte 13,0
         rts
         .endm

         cli
         cld
l00      lda #0
l01      cmp #0
         beq l02
         #e "01"
l02      inc l00+1
         inc l01+1
         bne l00

         lda #0
         beq l03
         #e "02"

l03      lda #0
         bpl l04
         #e "03"

l04      lda #$80
         bmi l05
         #e "04"

l05      lda #0
         sec
         asl a
         bcs l06
         cmp #0
         bne l06
         lda #1
         asl a
         bcs l06
         cmp #2
         bne l06
         asl a
         bcs l06
         cmp #4
         bne l06
         asl a
         bcs l06
         cmp #8
         bne l06
         asl a
         bcs l06
         cmp #16
         bne l06
         asl a
         bcs l06
         cmp #32
         bne l06
         asl a
         bcs l06
         cmp #64
         bne l06
         asl a
         bcs l06
         cmp #128
         bne l06
         asl a
         bcc l06
         beq l07
l06      #e "05"

l07      clc
         bcc l08
         #e "06"

l08      sec
         bcs l09
         #e "07"

l09      lda #0
         sec
         lsr a
         bcs l10
         cmp #0
         bne l10
         lda #128
         lsr a
         bcs l10
         cmp #64
         bne l10
         lsr a
         bcs l10
         cmp #32
         bne l10
         lsr a
         bcs l10
         cmp #16
         bne l10
         lsr a
         bcs l10
         cmp #8
         bne l10
         lsr a
         bcs l10
         cmp #4
         bne l10
         lsr a
         bcs l10
         cmp #2
         bne l10
         lsr a
         bcs l10
         cmp #1
         bne l10
         lsr a
         bcc l10
         beq l11
l10      #e "08"

         lda #1
         sta l11+1
l11      lda #1
         bne l12
         #e "09"
l12      inc l11+1
         bne l11

         lda #0
         sta l14+1
l13      inc l14+1
         clc
         adc #1
l14      cmp #1
         beq l15
         #e "10"
l15      ldx l14+1
         bne l13

         lda #0
         sta l17+1
l16      inc l17+1
         sec
         adc #0
l17      cmp #1
         beq l18
         #e "11"
l18      ldx l17+1
         bne l16

         lda #255
         clc
         adc #1
         bne l19
         bmi l19
         bcs l20
l19      #e "12"

l20      lda #0
         sta 172
         lda 172
l21      cmp #0
         beq l22
         #e "13"
l22      inc l20+1
         inc l21+1
         bne l20

         lda #78
         tax
         lda #0
         txa
         cmp #78
         beq l23
         #e "14"

l23      lda #0
         ora #$01
         cmp #$01
         bne l24
         ora #$02
         cmp #$03
         bne l24
         ora #$04
         cmp #$07
         bne l24
         ora #$08
         cmp #$0f
         bne l24
         ora #$10
         cmp #$1f
         bne l24
         ora #$20
         cmp #$3f
         bne l24
         ora #$40
         cmp #$7f
         bne l24
         ora #$80
         cmp #$ff
         beq l25
l24      #e "15"

l25      lda #$ff
         and #$fe
         cmp #$fe
         bne l26
         and #$fd
         cmp #$fc
         bne l26
         and #$fb
         cmp #$f8
         bne l26
         and #$f7
         cmp #$f0
         bne l26
         and #$ef
         cmp #$e0
         bne l26
         and #$df
         cmp #$c0
         bne l26
         and #$bf
         cmp #$80
         bne l26
         and #$7f
         cmp #$00
         beq l27
l26      #e "16"

l27      ldx #0
l28      txa
         asl a
         cmp l0tab,x
         beq l29
         #e "17"
l29      inx
         bne l28

         ldx #0
l30      txa
         clc
         rol a
         cmp l0tab,x
         beq l31
         #e "18"
l31      inx
         bne l30

         ldx #0
l32      txa
         sec
         rol a
         cmp l1tab,x
         beq l33
         #e "19"
l33      inx
         bne l32

         ldx #0
l34      txa
         lsr a
         cmp r0tab,x
         beq l35
         #e "20"
l35      inx
         bne l34

         ldx #0
l36      txa
         clc
         ror a
         cmp r0tab,x
         beq l37
         #e "21"
l37      inx
         bne l36

         ldx #0
l38      txa
         sec
         ror a
         cmp r1tab,x
         beq l39
         #e "22"
l39      inx
         bne l38

         ldy #0
l40      tya
         tax
         inx
         txa
         cmp i0tab,y
         beq l41
         #e "23"
l41      iny
         bne l40

         ldx #0
l42      txa
         tay
         iny
         tya
         cmp i0tab,x
         beq l43
         #e "24"
l43      inx
         bne l42

         ldx #0
l44      txa
         clc
         adc #1
         cmp i0tab,x
         beq l45
         #e "25"
l45      inx
         bne l44

         ldx #0
l46      txa
         sec
         adc #1
         cmp i1tab,x
         beq l47
         #e "26"
l47      inx
         bne l46

         ldx #0
l48      txa
         sec
         sbc #1
         cmp d0tab,x
         beq l49
         #e "27"
l49      inx
         bne l48

         ldx #0
l50      txa
         clc
         sbc #1
         cmp d1tab,x
         beq l51
         #e "28"
l51      inx
         bne l50

         jsr print
         .text " - ok"
         .byte 13,0
         lda 2
         beq load
wait     jsr $ffe4
         beq wait
         jmp $8000

load     jsr print
name     .text "ldab"
namelen  = *-name
         .byte 0
         lda #0
         sta $0a
         sta $b9
         lda #namelen
         sta $b7
         lda #<name
         sta $bb
         lda #>name
         sta $bc
         pla
         pla
         jmp $e16f

l0tab
x        .var 0
l0t      .lbl
         .byte x*2&255
x        .var x+1&255
         .if x
         .goto l0t
         .endif

l1tab
x        .var 0
l1t      .lbl
         .byte x*2&255.1
x        .var x+1&255
         .if x
         .goto l1t
         .endif

r0tab
x        .var 0
r0t      .lbl
         .byte x/2&255
x        .var x+1&255
         .if x
         .goto r0t
         .endif

r1tab
x        .var 0
r1t      .lbl
         .byte x/2&255.128
x        .var x+1&255
         .if x
         .goto r1t
         .endif

i0tab
x        .var 0
i0t      .lbl
         .byte x+1&255
x        .var x+1&255
         .if x
         .goto i0t
         .endif

i1tab
x        .var 0
i1t      .lbl
         .byte x+2&255
x        .var x+1&255
         .if x
         .goto i1t
         .endif

d0tab
x        .var 0
d0t      .lbl
         .byte x-1&255
x        .var x+1&255
         .if x
         .goto d0t
         .endif

d1tab
x        .var 0
d1t      .lbl
         .byte x-2&255
x        .var x+1&255
         .if x
         .goto d1t
         .endif

print    pla
         sta print0+1
         pla
         sta print0+2
         ldx #1
print0   lda !*,x
         beq print1
         jsr $ffd2
         inx
         bne print0
print1   sec
         txa
         adc print0+1
         sta print2+1
         lda #0
         adc print0+2
         sta print2+2
print2   jmp !*


