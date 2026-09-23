; test written by andreas boose, extracted from D011TEST.D64

    * = $0801
    !byte $0b,$08,$01,$00,$9e
    !convtab pet
    !tx "2061"
    !byte $00,$00,$00
    jmp start

dauer = $ac
v3fff  = $bd
scr   = $0

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
	  ldx #$f8
l6       txa
	  sta $0700,x
	  inx
	  bne l6
	  lda #v3fff
	  sta $3fff
	  lda #$0f
	  sta $d020
	  lda $d012
	  bne *-3
	  lda #0
	  sta dauer
	  lda #$1b
	  sta $d011
	  lda #30
	  sta $d012
	  lda $d019
	  sta $d019
	  lda #$81
	  sta $d01a
l3       jsr $ffe4
	  cmp #29
	  bne l4
	  inc dauer
	  jmp l3
l4       cmp #157
	  bne l5
	  dec dauer
	  jmp l3
l5       cmp #95
	  bne l8
	  inc $d021
l8       cmp #" "
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

irq2     lda #<irq3
	  sta $0314
	  dec $d019
	  inc $d020
	  bit $ff
	  lda $d012 ;4
	  cmp $d012 ;4
	  bne *+2   ;2
		    ;
	  lda #204  ;3
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

	  bit $ffff
	  bit $ffff
	  bit $ffff
	  bit $ffff
	  bit $ffff
	  lda #$1b  +scr ;2
	  sta $d011 ;4
;-=#          lda $d012                     #=-
;-=#          sta $dd01                     #=-
	  dec $d020

	  ldx #12
	  dex
	  bne *-1
	  lda #$0b
	  sta $d011
	  ldx #50
	  dex
	  bne *-1
	  bit $ffff

	  inc $d020
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
l12      bit $ff   ;3
	  dex       ;2
	  bpl l12   ;2
	  dec $d020

	  lda #$1b
	  sta $d011

	  lda dauer
	  ldx $dc01
	  cpx #223
	  bne l7
	  lda $d012
l7       sta $c800
	  sta $dd01
	  lda #$70
	  sta $d012
	  jmp $ea81

irq3     lda #<irq1
	  sta $0314
	  dec $d019
	  lda #$0b  +scr
	  sta $d011
	  lda #44
	  sta $d012
	  lda #$11
	  sta $3fff
	  ldx #0
	  dex
	  bne *-1
	  lda #v3fff
	  sta $3fff
	  jmp $ea31

