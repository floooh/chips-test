; test written by andreas boose, extracted from D011TEST.D64

    * = $0801
    !byte $0b,$08,$01,$00,$9e
    !convtab pet
    !tx "2061"
    !byte $00,$00,$00
    jmp start

dauer = $ac
v3fff  = $bd
scr   = 8
line  = 47

    !align 255,0,0
start:
	  lda #$7f
	  sta $dc0d
	  lda #<irq1
	  sta $0314
	  lda #>irq1
	  sta $0315
	  lda #$ff
	  sta $dd03
	  lda #$0f
	  sta $d020
	  lda #6
	  sta $dd00
	  ldx #0
	  lda #$00
	  ldy #$40
l12      sta $4000,x
	  inx
	  bne l12
	  inc l12+2
	  dey
	  bne l12
	  lda #$40
	  sta l12+2
	  lda #$33
	  sta $01
	  ldx #$10
	  ldy #0
l16      lda $d000,y
l26      sta $5000,y
	  iny
	  bne l16
	  inc l16+2
	  inc l26+2
	  dex
	  bne l16
	  lda #$d0
	  sta l16+2
	  lda #$50
	  sta l26+2
	  lda #$37
	  sta $01
	  lda #$00
	  sta $ae
	  lda #$44
	  sta $af
	  lda #0
	  sta $f7
	  sta $f8
	  ldx #0
l1234    txa
	  sta $d800,x
	  sta $d900,x
	  sta $da00,x
	  sta $db00,x
	  inx
	  bne l1234
	  lda $d012
	  bne *-3
	  lda #0
	  sta dauer
	  lda #$18
	  sta $d011
	  lda #30
	  sta $d012
	  lda $d019
	  sta $d019
	  lda #$81
	  sta $d01a
	  inc dauer
fff      jmp fff

;.fill $c200-*,0
    !align 255, 0, 0

irq1     lda #<irq2
	  sta $0314
	  lda #00
	  bit $ffff
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

irq2     lda #<irq1
	  sta $0314
	  dec $d019
	  inc $d020
	  bit $ff
	  lda $d012 ;4
	  cmp $d012 ;4
	  bne *+2   ;2
		    ;
	  lda dauer ;3
	  lsr       ;2
	  bcs *+2   ;2
	  lsr       ;2
	  bcs *+2   ;2
	  bcs *+2   ;2
	  lsr       ;2
	  bcs *+2   ;2
	  bcs *+2   ;2
	  bcs *+2   ;2
	  bcs *+2   ;2
	  tax       ;2
l2       bit $ff   ;3
	  dex       ;2
	  bpl l2    ;2

	  lda #$0b-3
	  sta $d011

	  lda $d012
	  bpl *-3

	  lda #$1b-2
	  sta $d011

	  dec $d020
	  lda #line
	  sta $d012
	  jmp $ea31


