; gfxfetch
; --------
; 2009, 2010 Hannu Nuotio, Antti Lankila
; based on testprogs/VICII/videomode/rmwtest.asm

; Side border is opened by the test to reduce potential of any
; VIC-CPU timing skew.

; --- Consts

; position and length on the modified characters (for the reference)
testpos = 12
testlen = 4

cinv = $314
cnmi = $318
raster = 53     ; start of raster interrupt
m = $fb         ; zero page variable
screen = $400

; --- Code

*=$0801
basic:
; BASIC stub: "1 SYS 2061"
!by $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00

start:
  jsr setupchars
  jsr install
- jmp -

; -- Subroutines

install:        ; install the raster routine
  jsr restore   ; Disable the Restore key (disable NMI interrupts)
checkirq:
  lda cinv      ; check the original IRQ vector
  ldx cinv+1    ; (to avoid multiple installation)
  cmp #<irq1
  bne irqinit
  cpx #>irq1
  beq skipinit
irqinit:
  sei
  sta oldirq    ; store the old IRQ vector
  stx oldirq+1
  lda #<irq1
  ldx #>irq1
  sta cinv      ; set the new interrupt vector
  stx cinv+1
skipinit:
  lda #$1b
  sta $d011     ; set the raster interrupt location
  lda #raster
  sta $d012
  lda #$7f
  sta $dc0d     ; disable timer interrupts
  sta $dd0d
  ldx #1
  stx $d01a     ; enable raster interrupt
  lda $dc0d     ; acknowledge CIA interrupts
  lsr $d019     ; and video interrupts
  cli
  rts

deinstall:
  sei           ; disable interrupts
  lda #$1b
  sta $d011     ; restore text screen mode
  lda #$81
  sta $dc0d     ; enable Timer A interrupts on CIA 1
  lda #0
  sta $d01a     ; disable video interrupts
  lda oldirq
  sta cinv      ; restore old IRQ vector
  lda oldirq+1
  sta cinv+1
  bit $dd0d     ; re-enable NMI interrupts
  cli
  rts

; Auxiliary raster interrupt (for syncronization)
irq1:
; irq (event)   ; > 7 + at least 2 cycles of last instruction (9 to 16 total)
; pha           ; 3
; txa           ; 2
; pha           ; 3
; tya           ; 2
; pha           ; 3
; tsx           ; 2
; lda $0104,x   ; 4
; and #xx       ; 2
; beq           ; 3
; jmp ($314)    ; 5
                ; ---
                ; 38 to 45 cycles delay at this stage
  lda #<irq2
  sta cinv
  lda #>irq2
  sta cinv+1
  nop           ; waste at least 12 cycles
  nop           ; (up to 64 cycles delay allowed here)
  nop
  nop
  nop
  nop
  inc $d012     ; At this stage, $d012 has already been incremented by one.
  lda #1
  sta $d019     ; acknowledge the first raster interrupt
  cli           ; enable interrupts (the second interrupt can now occur)
  ldy #9
  dey
  bne *-1       ; delay
  nop           ; The second interrupt will occur while executing these
  nop           ; two-cycle instructions.
  nop
  nop
  nop
oldirq = * + 1  ; Placeholder for self-modifying code
  jmp *         ; Return to the original interrupt

; Main raster interrupt
irq2:
; irq (event)   ; 7 + 2 or 3 cycles of last instruction (9 or 10 total)
; pha           ; 3
; txa           ; 2
; pha           ; 3
; tya           ; 2
; pha           ; 3
; tsx           ; 2
; lda $0104,x   ; 4
; and #xx       ; 2
; beq           ; 3
; jmp (cinv)    ; 5
                ; ---
                ; 38 or 39 cycles delay at this stage
  lda #<irq1
  sta cinv
  lda #>irq1
  sta cinv+1
  ldx $d012
  nop
!if CYCLES != 63 {
!if CYCLES != 64 {
  nop           ; 6567R8, 65 cycles/line
  bit $24
} else {
  nop           ; 6567R56A, 64 cycles/line
  nop
}
} else {
  bit $24       ; 6569, 63 cycles/line
}
  cpx $d012     ; The comparison cycle is executed CYCLES or CYCLES+1 cycles
                ; after the interrupt has occurred.
  beq *+2       ; Delay by one cycle if $d012 hadn't changed.
                ; Now exactly CYCLES+3 cycles have passed since the interrupt.
  dex
  dex
  stx $d012     ; restore original raster interrupt position
  ldx #1
  stx $d019     ; acknowledge the raster interrupt


; start action here

  ldx #38
preloop:
  dex
  bne preloop
!if CYCLES = 65 {
  nop
  nop
  nop
  nop
}
!if CYCLES = 64 {
  nop
  nop
}

; set lda/sta/stx gfx RAM addr to $2001 for the first modified line
  lda #1
  sta testptr1
  sta testptr2
  sta testptr3

  ldy #7    ; modify lines 1..7
testloop:
testptr1 = * + 1
  lda $2001
  tax
  eor #$ff
testptr2 = * + 1
  sta $2001
testptr3 = * + 1
  stx $2001
  ; next gfx RAM addr for next line
  inc testptr1
  inc testptr2
  ; one inc moved below "open side border" code.

  ; delay 7 cycles
  nop
  nop
  bne *+2

  ; enter 38 column mode
  ldx #$0
  stx $d016
  ; leave 38 column mode
  ldx #$8
  stx $d016

  ; delay 5(+2/+1) cycles.
  bne *+2
!if CYCLES = 65 {
  nop
}
!if CYCLES = 64 {
  bit $24
} else {
  nop
}

  ; continue "next gfx RAM addr for next line"
  inc testptr3

  dey
  bne testloop

  lda #$80
postloop:
  cmp $d012
  bne postloop
endirq:

    dec framecount
    bne +
    lda #0
    sta $d7ff
+

  jmp $ea81     ; return to the auxiliary raster interrupt

restore:        ; disable the Restore key
  lda cnmi
  ldy cnmi+1
  pha
  lda #<nmi     ; Set the NMI vector
  sta cnmi
  lda #>nmi
  sta cnmi+1
  ldx #$81
  stx $dd0d     ; Enable CIA 2 Timer A interrupt
  ldx #0
  stx $dd05
  inx
  stx $dd04     ; Prepare Timer A to count from 1 to 0.
  ldx #$dd
  stx $dd0e     ; Cause an interrupt.
nmi = * + 1
  lda #$40      ; RTI placeholder
  pla
  sta cnmi
  sty cnmi+1    ; restore original NMI vector (although it won't be used)
  rts


setupchars:
; copy (partial) character rom to $2000-
  ldx #0        ; 255 loops
  sei           ; interrups off
  lda $1
  and #$fb
  sta $1        ; character rom on
- lda $d000,x   ; load from char-rom
  sta $2000,x   ; store to ram
  lda $d100,x   ; load from char-rom
  sta $2100,x   ; store to ram
  inx
  bne -
  lda $1
  ora #$04
  sta $1        ; character rom off
  lda $dc0d     ; acknowledge CIA interrupts
  lsr $d019     ; and video interrupts
  cli           ; interrupts on

; font = $2000
  lda $d018
  and #$f1
  ora #$08
  sta $d018

; create reference char
  lda $2000
  sta $2000 + $ff * 8
  lda $2001
  eor #$ff
  sta $2001 + $ff * 8
  lda $2002
  eor #$ff
  sta $2002 + $ff * 8
  lda $2003
  eor #$ff
  sta $2003 + $ff * 8
  lda $2004
  eor #$ff
  sta $2004 + $ff * 8
  lda $2005
  eor #$ff
  sta $2005 + $ff * 8
  lda $2006
  eor #$ff
  sta $2006 + $ff * 8
  lda $2007
  eor #$ff
  sta $2007 + $ff * 8

; setup text
  ldx #4
-- ldy #0
messageptr = * + 1
- lda message,y
screenptr = * + 1
  sta screen,y
  iny
  bne -
  inc messageptr+1
  inc screenptr+1
  dex
  bne --
  rts

framecount: !byte 5
  
; --- Data

!ct scr

!align 255,0
message:
;    |---------0---------0---------0--------|
!tx "graphics fetch testprog v5"
!if CYCLES = 65 {
!tx "n"
}
!if CYCLES = 64 {
!tx "o"
}
!if CYCLES = 63 {
!tx "p"
}
!tx                            ". test line: "
!fi 40, 0
!tx "...and what it should look like:        "
!fi testpos, 0
!fi testlen, $ff
!fi (40 - testlen - testpos), 0
!tx "                                        "
!tx "                                        "
!tx "this testprog tests graphics fetch      "
!tx "timing using in-line changes to the     "
!tx "graphics of the displayed character.    "
!tx "                                        "
!tx "                                        "
!tx "test: first, we set up character ram at "
!tx "$2000. using a stable raster interrupt, "
!tx "the graphics of char ", $22, 0, $22, " are read, xor'd"
!tx "with $ff, stored back to $200x (with x ="
!tx "1..7 depending on raster line) and      "
!tx "restored back to normal.                "
!tx "                                        "
!tx "results: the lines 1..7 (in other words,"
!tx "all except the first line) of four of   "
!tx "the characters are inverted. (", 0, "->", $ff,")     "
!tx "                                        "
!tx "side border is open by the test on the  "
!tx "lines 1..7 to ascertain that cpu and vic"
!tx "are correctly synced.                   "
