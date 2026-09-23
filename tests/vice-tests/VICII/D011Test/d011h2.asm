; test written by andreas boose, extracted from D011TEST.D64

    * = $0801
    !byte $0b,$08,$01,$00,$9e
    !convtab pet
    !tx "2061"
    !byte $00,$00,$00
    jmp start

dauer  = $ac
dauer2 = $ae
tabpnt = $f7

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
	  ldx #$00
	  lda #$07
l20      sta $d800,x
	  sta $d900,x
	  sta $da00,x
	  sta $db00,x
	  inx
	  bne l20
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
	  sta tabpnt
	  lda #$c8
	  sta tabpnt+1
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
l45      ;-=#cmp #133   #=-
	  ;-=#bne l10    #=-
	  ;-=#ldx data1+1#=-
	  ;-=#inx        #=-
	  ;-=#txa        #=-
	  ;-=#and #$07   #=-
	  ;-=#ora #$18   #=-
	  ;-=#sta data1+1#=-
	  ;-=#jmp l3     #=-
l10      ;-=#cmp #137   #=-
	  ;-=#bne l11    #=-
	  ;-=#ldx data1+1#=-
	  ;-=#dex        #=-
	  ;-=#txa        #=-
	  ;-=#and #$07   #=-
	  ;-=#ora #$18   #=-
	  ;-=#sta data1+1#=-
	  ;-=#jmp l3     #=-
l11      ;-=#cmp #134   #=-
	  ;-=#bne l12    #=-
	  ;-=#ldx data2+1#=-
	  ;-=#inx        #=-
	  ;-=#txa        #=-
	  ;-=#and #$07   #=-
	  ;-=#ora #$18   #=-
	  ;-=#sta data2+1#=-
	  ;-=#jmp l3     #=-
l12      ;-=#cmp #138   #=-
	  ;-=#bne l13    #=-
	  ;-=#ldx data2+1#=-
	  ;-=#dex        #=-
	  ;-=#txa        #=-
	  ;-=#and #$07   #=-
	  ;-=#ora #$18   #=-
	  ;-=#sta data2+1#=-
	  ;-=#jmp l46    #=-
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

	  ;-=#ldx #13                       #=-
	  ;-=#dex                           #=-
	  ;-=#bne *-1                       #=-
	  ;-=#bit $ffff                     #=-

	  ldx #10
	  dex
	  bne *-1
	  bit $ff

	  lda dauer
	  and #$07
	  ora #$c8
	  sta $d016

	  lda dauer
	  lsr dauer+1
	  ror
	  lsr
	  lsr

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
	  sty dauer+1
	  lda #>irq3
	  sta $0315
	  jmp $ea81

;.fill $c300-*,0
    !align 255,0,0

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

	  ldx #36
	  dex
	  bne *-1
	  bit $ff

td0      lda #$1a
	  sta $d011

	  ldx #10
	  dex
	  bne *-1
	  bit $ff
	  bit $ff

td1      lda #$1a
	  sta $d011

	  ldx #10
	  dex
	  bne *-1
	  bit $ff
	  bit $ff

td2      lda #$1a
	  sta $d011

	  ldx #10
	  dex
	  bne *-1
	  bit $ff
	  bit $ff

td3      lda #$1a
	  sta $d011

	  ldx #10
	  dex
	  bne *-1
	  bit $ff
	  bit $ff

td4      lda #$1a
	  sta $d011

	  ldx #10
	  dex
	  bne *-1
	  bit $ff
	  bit $ff

td5      lda #$1a
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

td7      lda #$1a
	  sta $d011

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

