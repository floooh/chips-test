; test written by andreas boose, extracted from D011TEST.D64

    * = $0801
    !byte $0b,$08,$01,$00,$9e
    !convtab pet
    !tx "2061"
    !byte $00,$00,$00
    jmp start

job      = $ac
value    = $ad
tab      = $f7
cntzy    = $f9
helpbyte = $fb
basevec  = $fc

basepage = $4000
tabbeg   = $8000
zeile    = $9a-6

    !align 255,0,0
start:
	  lda #$7f
	  sta $dc0d
	  sta $dd0d
	  sta $dd05
	  lda #$41
	  sta $dd04
	  lda #<irq1
	  sta $0314
	  lda #>irq1
	  sta $0315
	  lda $d012
	  bne *-3
	  lda #$3b
	  sta $d011
	  lda #zeile
	  sta $d012
	  lda $d019
	  sta $d019
	  lda #$81
	  sta $d01a

	  lda #<tabbeg
	  sta tab
	  lda #>tabbeg
	  sta tab+1
	  lda anftab
	  sta cntzy
	  lda anftab+1
	  sta cntzy+1
l02      jsr settime
	  jsr getadr
	  jsr getd012
	  jsr getdd04
	  inc cntzy
	  bne l01
	  inc cntzy+1
l01      lda cntzy
	  cmp endtab
	  bne l02
	  lda cntzy+1
	  cmp endtab+1
	  bne l02
	  lda #0
	  sta $d01a
	  lda $d019
	  sta $d019
	  lda #<$ea31
	  sta $0314
	  lda #>$ea31
	  sta $0315
	  lda #$81
	  sta $dc0d
	  lda #$1b
	  sta $d011
	  rts

settime  lda cntzy
	  and #7
	  sta helpbyte
	  ldx #7
l04      txa
	  asl
	  tay
	  dec helpbyte
	  bmi l03
	  lda #$ea
	  !byte $2c
l03      lda #$24
	  sta delay1,y
	  dex
	  bne l04
	  lda cntzy+1
	  sta helpbyte
	  lda cntzy
	  ror helpbyte
	  ror
	  ror helpbyte
	  ror
	  ror helpbyte
	  ror
	  sta delay8+1
	  rts

getdd04  lda #<$dd04
	  sta load+1
	  lda #>$dd04
	  sta load+2
	  lda #1
	  sta job
	  lda job
	  bne *-2
	  lda value
	  eor #$ff
	  ldy #0
	  sta (tab),y
	  inc tab
	  bne *+4
	  inc tab+1
	  rts

getd012  lda #<$d012
	  sta load+1
	  lda #>$d012
	  sta load+2
	  lda #1
	  sta job
	  lda job
	  bne *-2
	  lda value
	  ldy #0
	  sta (tab),y
	  inc tab
	  bne *+4
	  inc tab+1
	  rts

getadr   lda #<$de00
	  sta load+1
	  lda #>$de00
	  sta load+2
	  lda #<basepage
	  sta basevec
	  lda #>basepage
	  sta basevec+1
	  ldy #0
	  ldx #$40
l06      lda basevec+1
l05      sta (basevec),y
	  iny
	  bne l05
	  inc basevec+1
	  dex
	  bne l06
	  lda #1
	  sta job
	  lda job
	  bne *-2
	  lda value
	  pha
	  sta basevec+1
	  ldy #0
l07      tya
	  sta (basevec),y
	  iny
	  bne l07
	  lda #1
	  sta job
	  lda job
	  bne *-2
	  lda value
	  sta (tab),y
	  inc tab
	  bne *+4
	  inc tab+1
	  pla
	  sta (tab),y
	  inc tab
	  bne *+4
	  inc tab+1
	  rts

anftab !word 0
endtab !word 1000

;.fill $c200-*,0
    !align 255, 0, 0

irq1     lda #<irq2
	  sta $0314
	  lda #6
	  sta $dd00
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
	  nop
	  jmp $ea81

irq2     lda #<irq1
	  sta $0314
	  dec $d019
	  nop
	  nop
	  nop
	  nop
	  ldy #$11
	  lda $d012
	  cmp $d012
	  bne *+2        ;7
	  sty $dd0e      ;4
	  dec $d020

	  ldx #5+6
xx       bit $ff
	  dex
	  bne xx
	  bit $ff
	  bit $ff

	  ldx #124
z1       bit $ff
	  dex
	  bne z1
	  bit $ff
	  nop
	  nop

	  ldx #124
z2       bit $ff
	  dex
	  bne z2
	  bit $ff
	  nop
	  nop

	  ldx #124
z3       bit $ff
	  dex
	  bne z3
	  bit $ff
	  nop
	  nop

	  ldx #124
z4       bit $ff
	  dex
	  bne z4
	  bit $ff
	  nop
	  nop

	  ldx #124
z5       bit $ff
	  dex
	  bne z5
	  bit $ff
	  nop
	  nop

	  ldx #124
z6       bit $ff
	  dex
	  bne z6
	  bit $ff
	  nop
	  nop

	  ldx #124
z7       bit $ff
	  dex
	  bne z7
	  bit $ff
	  nop
	  nop

	  ldx #124
z8       bit $ff
	  dex
	  bne z8
	  bit $ff
	  nop
	  nop

	  ldx #124
z9       bit $ff
	  dex
	  bne z9
	  bit $ff
	  nop
	  nop

	  ldx #8         ;1
	  dex            ;10*5
	  bne *-1        ;
	  nop
	  nop
delay8   ldx #0         ;2
	  bit $ff        ;3
	  dex            ;2
	  bpl *-3        ;2
delay1   bit $ea        ;8*3 =
	  bit $ea
	  bit $ea
	  bit $ea
	  bit $ea
	  bit $ea
	  bit $ea
	  bit $ea

load     lda $de00      ;4  162
	  sta value

	  sta $0700
	  lda #0
	  sta job
yy       lda #$3b
	  sta $d011
	  lda #zeile
	  sta $d012
	  inc $d020
	  lda #7
	  sta $dd00
	  jmp $ea81

