
cinv = $314
cnmi = $318
raster = 30     ; start of raster interrupt

*=$801
basic: !by $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00

start:
  jmp install

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
w1:
  inc $0400
  jmp w1

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
w2:
  inc $0401
  jmp w2

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
  jsr runciatest
  jsr setupnexttest
  jmp $ea81     ; return to the auxiliary raster interrupt

restore:        ; disable the Restore key
  lda cnmi
  sta $c000
  lda cnmi+1
  sta $c001
  lda #<nmi     ; Set the NMI vector
  sta cnmi
  lda #>nmi
  sta cnmi+1
  rts

runciatest:
  ; CIA reset
  ;lda #0
  ;sta $dc0e
  ;sta $dd0e
  ;sta $dd0f

  ldx #$08
  ldy #$82
  sty $dd0d
  ldy #$ff
  sty $dc04
  ldy #$00
  sty $dc05
  ldy #$d5
  sty $dc0e
loada:
  ldy #$13
  sty $dd04
  ldy #$00
  sty $dd05
forceloada:
  ldy #$d5
  sty $dd0e

loadb:
  ldy #$07
  sty $dd06
  ldy #$00
  sty $dd07
forceloadb:
  ldy #$d9
  sty $dd0f

; sample TBL into $2fff,x (pointer incremented by 8 for each test)
readtb:
  lda $dd06
storesta:
  sta $2fff,x
  dex
  bne readtb

; wait until NMI has certainly occured & done its thing
waitfornmi:
  lda $d012
  cmp #$50
  bne waitfornmi

; compare the results of TBL sampling
  ldx #$08
storelda:
  lda $2fff,x
storecmp:
  cmp $3fff,x
  bne cmptestfailedjump
  dex
  bne storelda
  rts

cmptestfailedjump:
  jmp cmptestfailed

setupnexttest:
storeb:
  ; update the check pointers of CIA values
  clc
  lda storesta+1
  adc #$08
  sta storesta+1
  sta storelda+1
  sta storecmp+1
  bcc nexta
  inc storesta+2
  inc storelda+2
  inc storecmp+2
nexta:
  ; try with force load & without
  lda forceloada+1
  eor #$10
  sta forceloada+1
  and #$10
  beq setupend

  ; try all choices from 0x13 to 0x4
  ldx loada+1
  dex
  stx loada+1
  cpx #$03
  bne setupend
  ldx #$13
  stx loada+1

  ; try with force load & without
  lda forceloadb+1
  eor #$10
  sta forceloadb+1
  and #$10
  beq setupend

  ; decrement load until complete
  ldx loadb+1
  dex
  stx loadb+1
  cpx #$ff
  bne setupend

  ; end test: turn border green, restore IRQ/NMI
  lda #5
  sta $d020
  ; signal success
  lda #0
  sta $d7ff

  lda $c000
  sta cnmi
  lda $c001
  sta cnmi+1
  jmp deinstall

setupend:
  rts

nmi:
  pha
  lda $dc04
nmistore:
  cmp $5000
  bne nmitestfailed
  inc nmistore+1
  bne acknmi
  inc nmistore+2
acknmi:
  bit $dd0d
  pla
  rti

; debug printing section
screenptr: !by 0

printbyte:
  stx storex+1
  ldx screenptr
  sta $400,x
  inc screenptr
storex:
  ldx #$0
  rts

printhex:
  pha
  lsr
  lsr
  lsr
  lsr
  jsr printhexbyte
  pla
  and #$f
printhexbyte:
  cmp #$a
  bcc handleletter
  sbc #9
  jmp printbyte
handleletter:
  adc #$30
  jmp printbyte

nmitestfailed:
  jsr printhex
  lda #' '
  jsr printbyte
  lda #14
  jsr printbyte
  lda #13
  jsr printbyte
  lda #9
  jsr printbyte
  lda #' '
  jsr printbyte
  lda nmistore+2
  jsr printhex
  lda nmistore+1
  jsr printhex
  jmp freeze

freeze:
  inc $d020
  ; signal failure
  lda #$ff
  sta $d7ff
  jmp freeze

cmptestfailed:
  txa
  jsr printhex

  lda #' '
  jsr printbyte
  lda #3
  jsr printbyte
  lda #13
  jsr printbyte
  lda #16
  jsr printbyte
  lda #' '
  jsr printbyte

  lda storesta+2
  jsr printhex
  lda storesta+1
  jsr printhex

  jmp freeze

