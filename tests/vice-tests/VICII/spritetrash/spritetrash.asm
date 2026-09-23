; based on code posted by mathop, see
; https://www.lemon64.com/forum/viewtopic.php?t=81712

; on 6569R1 this will display two 8-character wide bars in the top row
; with the contents of $3fff between them
;
; ..********...**...******
; 000000001111111122222222


irqvec = $0314

spriteloc=$0340
screenloc=$0400

*=$0801

  !word .z
  !word 10
  !byte $9e ; SYS
  !text "2061"
  !byte 0
.z
  !word 0

start:
  sei
  lda #$7f
  sta $dc0d
  lda $dc0d
  lda #1
  sta $d01a
  sta $d019
  lda #24
  sta $d012
  lda #27
  sta $d011

  lda #<isr
  sta irqvec
  lda #>isr
  sta irqvec+1
  lda #0
  sta tempx

  ldx #62
clrsprite
  sta spriteloc, x
  dex
  bpl clrsprite

  lda #128          ; leftmost pixel set
  sta spriteloc

  lda #13
  sta screenloc+$3fe
  sta screenloc+$3ff

  ldx #98
  stx $d00c ; 6 x = 98
  stx $d00d ; 6 y = 98
  inx
  stx $d00e ; 7 x = 99
  stx $d00f ; 7 y = 99

  lda #$c0
  sta $d010
  sta $d015

  lda #$18
  sta $3fff ; idle byte

  bit $d01e ; clear collision
  cli
  rts

isr
  lda #1
  sta $d019

  ldx tempx
;  lda screenloc, x
  lda #'.'
  ldy $d01e             ; sprite/sprite collision
  beq nocol
;  ora #128
  lda #'*'
;  bne update_screen
nocol
;  and #127
;update_screen
  sta screenloc, x
  lda #1
  sta $d800, x

  jsr toggle_sprite_pixel   ; remove leftmost set pixel
  ldx tempx
  inx
  cpx #8*3
  bcc updatex

  ldy #$ff ; failure
  ldx #0
-
  lda screenloc,x
  cmp expected,x
  bne +
  inx
  cpx #8*3
  bne -
  ldy #0 ; success
+
  sty $d7ff
  lda #10 ; red
  cpy #0  ; success
  bne +
  lda #13 ; green
+
  sta $d020

  ldx #0
updatex
  stx tempx
  jsr toggle_sprite_pixel   ; set new leftmost set pixel
  jmp $ea31

toggle_sprite_pixel
  lda tempx
  tax
  lsr
  lsr
  lsr
  tay ; y = tempx / 8
  txa
  and #7
  tax
  lda spriteloc, y
  eor bits, x
  sta spriteloc, y
  rts

tempx
  !byte 0

bits
  !byte 128, 64, 32, 16, 8, 4, 2, 1
expected
  !text "..********...**...******"
