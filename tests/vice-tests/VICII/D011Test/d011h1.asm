; test written by andreas boose, extracted from D011TEST.D64

    * = $0801
    !byte $0b,$08,$01,$00,$9e
    !convtab pet
    !tx "2061"
    !byte $00,$00,$00
    jmp start
    
dauer  = $ac
dauer2 = $ae
scr=4

    !align 255,0,0
start:
	  lda #$7f
	  sta $dc0d
	  lda #<irq3
	  sta $0314
	  lda #>irq3
	  sta $0315
	  lda #$ff
	  sta $dd03
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
	  sta dauer
	  sta dauer+1
	  sta dauer2
	  sta dauer2+1
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
	  cmp #29
	  bne l4
	  inc dauer
	  bne *+4
	  inc dauer+1
	  jmp l3
l4       cmp #157
	  bne l5
	  lda dauer
	  bne *+4
	  dec dauer+1
	  dec dauer
	  jmp l3
l5       cmp #17
	  bne l44
	  inc dauer2
	  bne *+4
	  inc dauer2+1
	  jmp l3
l44      cmp #145
	  bne l45
	  lda dauer2
	  bne *+4
	  dec dauer2+1
	  dec dauer2
	  jmp l3
l45      cmp #133
	  bne l10
	  ldx data1+1
	  inx
	  txa
	  and #$07
	  ora #$18
	  sta data1+1
	  jmp l3
l10      cmp #137
	  bne l11
	  ldx data1+1
	  dex
	  txa
	  and #$07
	  ora #$18
	  sta data1+1
	  jmp l3
l11      cmp #134
	  bne l12
	  ldx data2+1
	  inx
	  txa
	  and #$07
	  ora #$18
	  sta data2+1
	  jmp l3
l12      cmp #138
	  bne l13
	  ldx data2+1
	  dex
	  txa
	  and #$07
	  ora #$18
	  sta data2+1
	  jmp l46
l13      cmp #" "
	  bne l46
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
l46      jmp l3

;.fill $c200-*,0
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
	  ldy dauer+1
	  inc $d020
	  lda $d012
	  cmp $d012
	  bne *+2

	  ldx #15
	  dex
	  bne *-1

	  lda dauer
	  lsr
	  bcs *+2
	  lsr
	  bcs *+2
	  bcs *+2
	  lsr
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  lsr
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  lsr
	  bcc l50
	  ldx #2
	  bit $ff
	  dex
	  bne *-3
l50      lsr
	  bcc l51
	  ldx #6
	  dex
	  bne *-1
	  nop
l51      lda #$18
	  sta $d011
	  lda #$1b
	  sta $d011
	  lda #53
	  sta $d012
	  dec $d020
	  jmp $ea81

irq3     lda #<irq4
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

irq4     lda #<irq5
	  sta $0314
	  dec $d019
	  ldy dauer+1
	  inc $d020
	  lda $d012
	  cmp $d012
	  bne *+2
	  lda dauer2
	  lsr
	  bcs *+2
	  lsr
	  bcs *+2
	  bcs *+2
	  lsr
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  lsr
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  bcs *+2
	  lsr
	  bcc l48
	  ldx #2
	  bit $ff
	  dex
	  bne *-3
l48      lsr
	  bcc l49
	  ldx #6
	  dex
	  bne *-1
	  nop
l49      lsr
	  bcc l53
	  ldx #12
	  dex
	  bne *-1
	  bit $ffff
l53      lsr
	  bcc l54
	  ldx #25
	  dex
	  bne *-1
	  bit $ff
l54      lsr
	  bcc l55
	  ldx #51
	  dex
	  bne *-1
	  nop
l55
data1    lda #$1a
	  sta $d011

	  ldx #10
	  dex
	  bpl *-1
	  bit $ff
	  bit $ff

data2    lda #$1a
;-=#          sta $d011                     #=-
;-=#                                        #=-
;-=#          ldx #10                       #=-
;-=#          dex                           #=-
;-=#          bpl *-1                       #=-
;-=#          bit $ff                       #=-
;-=#          bit $ff                       #=-
;-=#                                        #=-
;-=#+data2    lda #$1b                      #=-
;-=#          sta $d011                     #=-

	  dec $d020
	  lda dauer2
	  sta $c800
	  lda #$ff
	  sta $d012
	  lda #>irq5
	  sta $0315
	  jmp $ea81

irq5     lda #<irq1
	  sta $0314
	  lda #>irq1
	  sta $0315
	  dec $d019
	  lda #$1b
	  sta $d011
	  lda #43
	  sta $d012
	  jmp $ea31

