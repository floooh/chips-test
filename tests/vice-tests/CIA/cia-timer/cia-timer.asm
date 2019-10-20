  processor 6502

;-------------------------------------------------------------------------------

  .org $801
basic:
  .word 0$      ; link to next line
  .word 1995    ; line number
  .byte $9E     ; SYS token

; SYS digits

  .if (* + 8) / 10000
  .byte $30 + (* + 8) / 10000
  .endif
  .if (* + 7) / 1000
  .byte $30 + (* + 7) % 10000 / 1000
  .endif
  .if (* + 6) / 100
  .byte $30 + (* + 6) % 1000 / 100
  .endif
  .if (* + 5) / 10
  .byte $30 + (* + 5) % 100 / 10
  .endif
  .byte $30 + (* + 4) % 10

0$:
  .byte 0,0,0   ; end of BASIC program

;-------------------------------------------------------------------------------

start:
  lda #<greet_msg
  ldy #>greet_msg
  jsr $ab1e
  jsr clrscr

restart:
  lda $d011
  bpl *-3

  lda #$00
  sta $fa
nexttest:
  ; "clear" CIAs
  lda #$00
  sta $dc0e
  sta $dc0f
  sta $dd0e
  sta $dd0f
  lda $dc0d
  lda $dd0d
  ldx $fa
  lda icr,x
  sta useirc+1
  lda cr,x
  sta usecr+1
  lda tlow,x
  sta usetlow+1
  lda cianr,x
  sta usecia11+2
  sta usecia12+2
  sta usecia13+2
  sta usecia14+2
  sta usecr+2
  sta usetlow+2
  eor #$01
  sta usecia21+2
  sta usecia22+2
  sta usecia23+2
  sta usecia24+2
  txa
  asl
  tax
  lda out+1,x
  tay
  lda out,x
  sta output0+1
  sty output0+2
  clc
  adc #40
  sta output1+1
  bcc nr1
  iny
nr1:
  sty output1+2
  clc
  adc #40
  sta output2+1
  bcc nr2
  iny
nr2:
  sty output2+2
  clc
  adc #40
  sta output3a+1
  sta output3b+1
  bcc nr3
  iny
nr3:
  sty output3a+2
  sty output3b+2
  clc
  adc #40
  sta output4a+1
  sta output4b+1
  bcc nr4
  iny
nr4:
  sty output4a+2
  sty output4b+2

  ; setup irq
  sei
  lda #$35
  sta $01
  lda #<irqhandler
  sta $fffe
  lda #>irqhandler
  sta $ffff
  lda #<irqhandler
  sta $fffa
  lda #>irqhandler
  sta $fffb
  lda #$7f
  sta $dc0d
  sta $dd0d
useirc:
  lda #$80
usecia11:
  sta $dc0d
  
  inc $d019
  lda $dc0d
  lda $dd0d
  cli

  ; do test
  jsr test

  ; next text or restart
  inc $fa
  lda $fa
  cmp #$08
  bne nrst

  ; "clear" CIAs
  lda #$00
  sta $dc0e
  sta $dc0f
  sta $dd0e
  sta $dd0f
  lda $dc0d
  lda $dd0d

  lda #$1b
  sta $d011
  
  jsr checkdata

  jmp *

nrst:
  jmp nexttest


end:
  jmp end

;-------------------------------------------------------------------------------

test:
  ; switch off the screen while the test is running, this is needed so it can
  ; work correctly also on NTSC (where the border is much smaller)
  lda #$0b
  sta $d011

  lda $d011
  bmi test
  lda $d011
  bpl test

  ldx #$10
  lda $dc0d
  lda $dd0d
  lda #$00
  sta $dc04
  sta $dc06
  sta $dd04
  sta $dd06
  lda #$01
  sta $dc05
  sta $dc07
  sta $dd05
  sta $dd07
usecia21:
  inc $dd04
  lda #$11
usecr:
  sta $dc0f
usecia22:
  sta $dd0e

lp0:

  lda #$20
output3a:
  sta $0478,x
output4a:
  sta $04a0,x
  ldy #$1a
lp1:
  dey
  bne lp1

  lda #$80
  sec
usecia23:
  sbc $dd04
  jsr delay
usetlow:
  lda $dc06
  sec
  sbc #$15
output0:
  sta $0400,x
usecia12:
  lda $dc0d
output1:
  sta $0428,x
usecia13:
  lda $dc0d
output2:
  sta $0450,x

  dex
  bne lp0

  rts

;-------------------------------------------------------------------------------

  .org $0f00
irqhandler:
  pha
usecia14:
  lda $dc0d
output3b:
  sta $0478,x
usecia24:
  lda $dd04
  clc
  adc #$10
output4b:
  sta $04a0,x
  pla
intexit:
  rti  

;-------------------------------------------------------------------------------

clrscr:
  ldx #$00
  stx $d020
  stx $d021
clrlp:
  lda #$01
  sta $d800,x
  sta $d900,x
  sta $da00,x
  sta $db00,x
  inx
  bne clrlp
  rts

;-------------------------------------------------------------------------------

  .org $1000
delay:              ;delay 80-accu cycles, 0<=accu<=64
    lsr             ;2 cycles akku=akku/2 carry=1 if accu was odd, 0 otherwise
    bcc waste1cycle ;2/3 cycles, depending on lowest bit, same operation for both
waste1cycle:
    sta smod+1      ;4 cycles selfmodifies the argument of branch
    clc             ;2 cycles
;now we have burned 10/11 cycles.. and jumping into a nopfield 
smod:
    bcc *+10
  .byte $EA,$EA,$EA,$EA,$EA,$EA,$EA,$EA
  .byte $EA,$EA,$EA,$EA,$EA,$EA,$EA,$EA
  .byte $EA,$EA,$EA,$EA,$EA,$EA,$EA,$EA
  .byte $EA,$EA,$EA,$EA,$EA,$EA,$EA,$EA
    rts             ;6 cycles

;-------------------------------------------------------------------------------

icr:
  .byte $80,$81,$80,$82,$80,$81,$80,$82

cr:
  .byte $0e,$0e,$0f,$0f,$0e,$0e,$0f,$0f

tlow:
  .byte $04,$04,$06,$06,$04,$04,$06,$06

cianr:
  .byte $dc,$dc,$dc,$dc,$dd,$dd,$dd,$dd  
  
out:
  .word $0450,$0518,$0464,$052c,$0608,$06d0,$061c,$06e4

greet_msg:
    dc.b 147,"CIA-TIMER R03 / REFERENCE: "
    if DUMP = 0
    dc.b "OLD"
    endif
    if DUMP = 1
    dc.b "NEW"
    endif
    dc.b " CIAS", 13,13,0

;-------------------------------------------------------------------------------

checkdata:
    lda #5
    sta bordercol

    ldy #0
lp

    ldx #2
    lda $0428,y
    cmp data_compare+$000,y
    bne skp1
    ldx #5
skp1
    txa
    sta $d828,y
    cmp #2
    bne skp1a
    sta bordercol
    lda data_compare+$000,y
    sta $0428,y
skp1a:

    ldx #2
    lda $0528,y
    cmp data_compare+$100,y
    bne skp2
    ldx #5
skp2
    txa
    sta $d928,y
    cmp #2
    bne skp2a
    sta bordercol
    lda data_compare+$100,y
    sta $0528,y
skp2a:

    ldx #2
    lda $0628,y
    cmp data_compare+$200,y
    bne skp3
    ldx #5
skp3
    txa
    sta $da28,y
    cmp #2
    bne skp3a
    sta bordercol
    lda data_compare+$200,y
    sta $0628,y
skp3a:

    iny
    bne lp

lpa
    ldx #2
    lda $0728,y
    cmp data_compare+$300,y
    bne skp4
    ldx #5
skp4
    txa
    sta $db28,y
    cmp #2
    bne skp4a
    sta bordercol
    lda data_compare+$300,y
    sta $0728,y
skp4a:

    iny
    cpy #$c0
    bne lpa

bordercol = * + 1
    lda #0
    sta $d020

    ldx #0 ; success
    cmp #5
    beq nofail
    ldx #$ff ; failure
nofail:
    stx $d7ff

    rts

data_compare:
    if DUMP = 0
    incbin "dump-oldcia.bin"
    endif
    if DUMP = 1
    incbin "dump-newcia.bin"
    endif
