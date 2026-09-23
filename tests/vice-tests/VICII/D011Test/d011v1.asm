; test written by andreas boose, extracted from D011TEST.D64

    * = $0801
    !byte $0b,$08,$01,$00,$9e
    !convtab pet
    !tx "2061"
    !byte $00,$00,$00
    jmp start

ver    = $fb
tabpnt = $fc

    !align 255,0,0
start:
	  lda #$7f
	  sta $dc0d
	  lda #<irq3
	  sta $0314
	  lda #>irq3
	  sta $0315
	  lda #0
	  sta $ae
	  lda #4
	  sta $af
	  lda #7
	  sta $d021
	  jsr $e544
	  lda #6
	  sta $d021
	  ldx #0
l6       txa
	  ldy #39
	  sta ($ae),y
	  dey
	  bpl *-3
	  lda $ae
	  clc
	  adc #40
	  sta $ae
	  bcc *+4
	  inc $af
	  inx
	  cpx #25
	  bne l6
	  lda $d012
	  bne *-3
	  lda #0
	  sta ver
	  sta tabpnt
	  lda #$c8
	  sta tabpnt+1
	  lda #$e8
	  sta $f7
	  lda #0
	  sta $f8
	  sta $d020
	  lda #$1b
	  sta $d011
	  lda #$ff
	  sta $d012
	  lda $d019
	  sta $d019
	  lda #$81
	  sta $d01a
l3       jsr $ffe4
	  cmp #145
	  bne l10
	  ldx ver
	  inx
	  cpx #57
	  bne *+3
	  dex
	  stx ver
	  jmp l3
l10      cmp #17
	  bne l11
	  ldx ver
	  dex
	  bpl *+3
	  inx
	  stx ver
	  jmp l3
l11      cmp #" "
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
	  rts

;.fill $c100-*,0
    !align 255, 0, 0

irq1     lda #<irq2
	  sta $0314
	  lda #6
	  lda $dd00
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

irq2     lda #<irq3
	  sta $0314
	  dec $d019
	  bit $ff
	  inc $d020
	  lda $d012
	  cmp $d012
	  bne *+2

	  lda $f7
	  lsr $f7+1
	  ror
	  bcs *+2
	  lsr $f7+1
	  ror
	  bcs *+2
	  bcs *+2
	  lsr $f7+1
	  ror
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  lsr $f7+1
	  ror
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  tax
l2       bit $ffff
	  bit $ffff
	  bit $ff
	  dex
	  bpl l2

td0      lda #$1b
	  sta $d011

	  ldx #10
	  dex
	  bne *-1
	  bit $ff
	  bit $ff

td1      lda #$1b
	  sta $d011

	  ldx #10
	  dex
	  bne *-1
	  bit $ff
	  bit $ff

td2      lda #$1e
	  sta $d011

	  ldx #10
	  dex
	  bne *-1
	  bit $ff
	  bit $ff

td3      lda #$1f
	  sta $d011

	  ldx #10
	  dex
	  bne *-1
	  bit $ff
	  bit $ff

td4      lda #$18
	  sta $d011

	  ldx #10
	  dex
	  bne *-1
	  bit $ff
	  bit $ff

td5      lda #$19
	  sta $d011

	  ldx #10
	  dex
	  bne *-1
	  bit $ff
	  bit $ff

td6      lda #$1a
	  sta $d011

	  ldx #10
	  dex
	  bne *-1
	  bit $ff
	  bit $ff

td7      lda #$1b
	  sta $d011

	  dec $d020
	  lda #$ff
	  sta $d012
	  lda #>irq3
	  sta $0315
	  jmp $ea81

irq3     lda #<irq1
	  sta $0314
	  lda #>irq1
	  sta $0315
	  dec $d019
	  lda #$1b
	  sta $d011
	  lda #43
	  sta $d012
	  ldy #0
	  sty $fa
	  lda (tabpnt),y
	  asl
	  rol $fa
	  asl
	  rol $fa
	  asl
	  rol $fa
	  clc
	  adc #<tab
	  sta $f9
	  lda $fa
	  adc #>tab
	  sta $fa
	  lda ($f9),y
	  sta td0+1
	  iny
	  lda ($f9),y
	  sta td1+1
	  iny
	  lda ($f9),y
	  sta td2+1
	  iny
	  lda ($f9),y
	  sta td3+1
	  iny
	  lda ($f9),y
	  sta td4+1
	  iny
	  lda ($f9),y
	  sta td5+1
	  iny
	  lda ($f9),y
	  sta td6+1
	  iny
	  lda ($f9),y
	  sta td7+1
	  inc tabpnt
	  ldy #0
	  lda (tabpnt),y
	  cmp #$ff
	  bne l40
	  lda #$00
	  sta tabpnt
l40      jmp $ea31

tab
!byte $1b,$1b,$1b,$1b,$1b,$1b,$1b,$1b ;-=#0#=-
!byte $1a,$1a,$1a,$1a,$1a,$1a,$1a,$1a
!byte $19,$19,$19,$19,$19,$19,$19,$19
!byte $18,$18,$18,$18,$18,$18,$18,$18
!byte $1f,$1f,$1f,$1f,$1f,$1f,$1f,$1f
!byte $1e,$1e,$1e,$1e,$1e,$1e,$1e,$1e
!byte $1d,$1d,$1d,$1d,$1d,$1d,$1d,$1d
!byte $1c,$1c,$1c,$1c,$1c,$1c,$1c,$1c

!byte $1c,$1b,$1b,$1b,$1b,$1b,$1b,$1b ;-=#1#=-
!byte $1c,$1a,$1a,$1a,$1a,$1a,$1a,$1a
!byte $1c,$19,$19,$19,$19,$19,$19,$19
!byte $1c,$18,$18,$18,$18,$18,$18,$18
!byte $1c,$1f,$1f,$1f,$1f,$1f,$1f,$1f
!byte $1c,$1e,$1e,$1e,$1e,$1e,$1e,$1e
!byte $1c,$1d,$1d,$1d,$1d,$1d,$1d,$1d

!byte $1c,$1d,$1c,$1c,$1c,$1c,$1c,$1c ;-=#2#=-
!byte $1c,$1d,$1b,$1b,$1b,$1b,$1b,$1b
!byte $1c,$1d,$1a,$1a,$1a,$1a,$1a,$1a
!byte $1c,$1d,$19,$19,$19,$19,$19,$19
!byte $1c,$1d,$18,$18,$18,$18,$18,$18
!byte $1c,$1d,$1f,$1f,$1f,$1f,$1f,$1f
!byte $1c,$1d,$1e,$1e,$1e,$1e,$1e,$1e

!byte $1c,$1d,$1e,$1d,$1d,$1d,$1d,$1d ;-=#3#=-
!byte $1c,$1d,$1e,$1c,$1c,$1c,$1c,$1c
!byte $1c,$1d,$1e,$1b,$1b,$1b,$1b,$1b
!byte $1c,$1d,$1e,$1a,$1a,$1a,$1a,$1a
!byte $1c,$1d,$1e,$19,$19,$19,$19,$19
!byte $1c,$1d,$1e,$18,$18,$18,$18,$18
!byte $1c,$1d,$1e,$1f,$1f,$1f,$1f,$1f

!byte $1c,$1d,$1e,$1f,$1e,$1e,$1e,$1e ;-=#4#=-
!byte $1c,$1d,$1e,$1f,$1d,$1d,$1d,$1d
!byte $1c,$1d,$1e,$1f,$1c,$1c,$1c,$1c
!byte $1c,$1d,$1e,$1f,$1b,$1b,$1b,$1b
!byte $1c,$1d,$1e,$1f,$1a,$1a,$1a,$1a
!byte $1c,$1d,$1e,$1f,$19,$19,$19,$19
!byte $1c,$1d,$1e,$1f,$18,$18,$18,$18

!byte $1c,$1d,$1e,$1f,$18,$1f,$1f,$1f ;-=#5#=-
!byte $1c,$1d,$1e,$1f,$18,$1e,$1e,$1e
!byte $1c,$1d,$1e,$1f,$18,$1d,$1d,$1d
!byte $1c,$1d,$1e,$1f,$18,$1c,$1c,$1c
!byte $1c,$1d,$1e,$1f,$18,$1b,$1b,$1b
!byte $1c,$1d,$1e,$1f,$18,$1a,$1a,$1a
!byte $1c,$1d,$1e,$1f,$18,$19,$19,$19

!byte $1c,$1d,$1e,$1f,$18,$19,$18,$18 ;-=#6#=-
!byte $1c,$1d,$1e,$1f,$18,$19,$1f,$1f
!byte $1c,$1d,$1e,$1f,$18,$19,$1e,$1e
!byte $1c,$1d,$1e,$1f,$18,$19,$1d,$1d
!byte $1c,$1d,$1e,$1f,$18,$19,$1c,$1c
!byte $1c,$1d,$1e,$1f,$18,$19,$1b,$1b
!byte $1c,$1d,$1e,$1f,$18,$19,$1a,$1a

!byte $1c,$1d,$1e,$1f,$18,$19,$1a,$19 ;-=#7#=-
!byte $1c,$1d,$1e,$1f,$18,$19,$1a,$18
!byte $1c,$1d,$1e,$1f,$18,$19,$1a,$1f
!byte $1c,$1d,$1e,$1f,$18,$19,$1a,$1e
!byte $1c,$1d,$1e,$1f,$18,$19,$1a,$1d
!byte $1c,$1d,$1e,$1f,$18,$19,$1a,$1c
!byte $1c,$1d,$1e,$1f,$18,$19,$1a,$1b

!byte $1b,$1b,$1b,$1b,$1b,$1b,$1b,$1b

