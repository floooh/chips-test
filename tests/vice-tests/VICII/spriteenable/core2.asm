; spriteenable
; ------------
; based on testprogs/VICII/sprite0move/sprite0move.asm

; Tests sprite enable bevahiour.


; --- Macros

; SpriteLine - for easy definition of sprites
; from "ddrv.a" by Marco Baye
!macro SpriteLine .v {
!by .v>>16, (.v>>8)&255, .v&255
}


; --- Consts

raster = 47          ; start of raster interrupt
spritey = 50         ; sprite 0 Y

cinv = $0314
cnmi = $0318
screen = $0400
sprite0ptr = $7f8    ; pointer location

start_sprite0_x = 88
start_sprite1_x = 138
start_sprite2_x = 188
start_sprite3_x = 238
start_sprite4_x = 0
start_sprite5_x = 0
start_sprite6_x = 0
start_sprite7_x = 0


; --- Variables

; - zero page

temp = $ff              ; temp zero page variable
strptr = $39            ; zp pointer to string
tmpptr = $56            ; temporary zp pointer 2
scrptr = $fb            ; zp pointer to screen
zpzero = $fe            ; zero

; --- Code

*=$0801
basic:
; BASIC stub: "1 SYS 2061"
!by $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00

start:
    ; restore border color
    lda #0
    sta $d020

    jsr setup
    jsr install
    jmp mainloop

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

; -- IRQ handlers

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
test_start:
    ; enable sprite 3
    lda #$08
    sta $d015

    ldx #4
-   dex
    bne -
    bit $ea
!if CYCLES = 63 {
    nop
} else {
    bit $ea
}

    ; modify sprite enable on cycle 55 (56 on NTSC)
    dec $d015

    ; show timing
    ldx #2
-   dex
    bne -
    nop
    nop
    nop
!if CYCLES = 64 {
    bit $ea
} else {
    nop
}
!if CYCLES = 65 {
    bit $ea     ; +1 cycle for BA low starting later
}
    inc $d021
    dec $d021

    ldx #$7
-   dex
    bne -
    nop
!if CYCLES = 64 {
    bit $ea
} else {
    nop
}
!if CYCLES = 65 {
    nop
}
    inc $d021
    dec $d021

    dec framecount
    bne +
    lda #0
    sta $d7ff
+

    jmp $ea81     ; return to the auxiliary raster interrupt

framecount: !byte 5

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


; --  Main loop

mainloop:
    jmp mainloop


; -- Subroutines

; - setup
;
setup:
    lda #0
    sta $3fff

    lda #0
    sta zpzero

    ; set screen color
    ldx #0
col_lp:
    lda #$20
    sta $0400,x
    sta $0500,x
    sta $0600,x
    sta $06e8,x
    lda 646
    sta $d800,x
    sta $d900,x
    sta $da00,x
    sta $dae8,x
    inx
    bne col_lp

    ; set key repeat on all keys.
    lda #255
    sta 650

    ; print some text
    lda #>screen
    sta scrptr+1
    lda #<screen
    sta scrptr
    lda #>message
    sta strptr+1
    lda #<message
    sta strptr
    jsr print_text

    ; set sprite stuff
    ldy #7
-   jsr set_sprite_x
    jsr set_sprite_ptr
    dey
    bpl -

    ; all 8 sprites at the same line
    lda #spritey
    sta $d001
    sta $d003
    sta $d005
    sta $d007
    sta $d009
    sta $d00b
    sta $d00d
    sta $d00f

    ; misc stuff
    lda #$0f
    sta $d027   ; sprite 0 color
    lda #0
    sta $d01b   ; priority
    sta $d01c   ; MCM
    sta $d01d   ; x expand
    sta $d017   ; y expand
    lda #$ff
    sta $d015   ; enable
    rts


; - set_sprite_x
;  y = sprite number
;
set_sprite_x:
    tya
    asl
    tax
    lda sprite0x,y
    sta $d000,x
    lda sprite0xh
    sta $d010
    rts


; - set_sprite_ptr
;
set_sprite_ptr:
    lda spriteptr,y
    sta sprite0ptr,y
    rts

; - print_text
; parameters:
;  scrptr -> screen location to print to
;  strptr -> string to print
;
print_text:
    ldy #0
-   lda (strptr),y
    beq +++         ; end if zero
    sta (scrptr),y  ; print char
    iny
    bne -           ; loop if not zero
    inc strptr+1
    inc scrptr+1
    bne -
+++
    clc
    tya             ; update scrptr to next char
    adc scrptr
    sta scrptr
    bcc +
    inc scrptr+1
+
    rts             ; return



; --- Data

; selected sprite
spritenum: !by 0

; sprite X coordinates
sprite0x:
!by <start_sprite0_x
!by <start_sprite1_x
!by <start_sprite2_x
!by <start_sprite3_x
!by <start_sprite4_x
!by <start_sprite5_x
!by <start_sprite6_x
!by <start_sprite7_x

; X msbs
sprite0xh: !by >start_sprite0_x

; selected sprite pointers
spriteptr:
!by sprite_first_ptr
!by sprite_first_ptr+1
!by sprite_first_ptr+2
!by sprite_first_ptr+3
!by sprite_first_ptr+4
!by sprite_first_ptr+5
!by sprite_first_ptr+6
!by sprite_first_ptr+7

!ct scr

; hex lookup table
hex_lut: !tx "0123456789abcdef"

; sprite bits
bitmask: !by $01, $02, $04, $08, $10, $20, $40, $80

; help text
message:
;    |---------0---------0---------0--------|
!tx " x    y                                 "
!tx "                                        "
!tx "                                        "
!tx "                                        "
!tx "sprite enable testprog 2 - "
!if CYCLES = 63 {
!tx                            "pal          "
}
!if CYCLES = 65 {
!tx                            "ntsc         "
}
!if CYCLES = 64 {
!tx                            "oldntsc      "
}
!tx "                                        "
!tx "dec $d015 (->$07) on check dma cycles.  "
!tx "                                        "
!tx "you should see sprites 0-2.             "
!tx "sprite 0 should have $ff as first byte. "
!tx "stable line from x to y.                "

!by 0

; sprites
!align 63,0
spritedata:
sprite_first_ptr = * / 64
;            765432107654321076543210
+SpriteLine %........################ ;1
+SpriteLine %#......................# ;2
+SpriteLine %#......................# ;3
+SpriteLine %#......##########......# ;4
+SpriteLine %#.....#..........#.....# ;5
+SpriteLine %#.....#..........#.....# ;6
+SpriteLine %#.....#..........#.....# ;7
+SpriteLine %#.....#..........#.....# ;8
+SpriteLine %#.....#..........#.....# ;9
+SpriteLine %#.....#..........#.....# ;10
+SpriteLine %#.....#..........#.....# ;11
+SpriteLine %#.....#..........#.....# ;12
+SpriteLine %#.....#..........#.....# ;13
+SpriteLine %#.....#..........#.....# ;14
+SpriteLine %#.....#..........#.....# ;15
+SpriteLine %#.....#..........#.....# ;16
+SpriteLine %#.....#..........#.....# ;17
+SpriteLine %#......##########......# ;18
+SpriteLine %#......................# ;19
+SpriteLine %#......................# ;20
+SpriteLine %######################## ;21
!by 0

;            765432107654321076543210
+SpriteLine %######################## ;1
+SpriteLine %#......................# ;2
+SpriteLine %#......................# ;3
+SpriteLine %#.........#............# ;4
+SpriteLine %#.......###............# ;5
+SpriteLine %#.........#............# ;6
+SpriteLine %#.........#............# ;7
+SpriteLine %#.........#............# ;8
+SpriteLine %#.........#............# ;9
+SpriteLine %#.........#............# ;10
+SpriteLine %#.........#............# ;11
+SpriteLine %#.........#............# ;12
+SpriteLine %#.........#............# ;13
+SpriteLine %#.........#............# ;14
+SpriteLine %#.........#............# ;15
+SpriteLine %#.........#............# ;16
+SpriteLine %#.........#............# ;17
+SpriteLine %#.........#............# ;18
+SpriteLine %#......................# ;19
+SpriteLine %#......................# ;20
+SpriteLine %######################## ;21
!by 0

;            765432107654321076543210
+SpriteLine %######################## ;1
+SpriteLine %#......................# ;2
+SpriteLine %#......................# ;3
+SpriteLine %#...##############.....# ;4
+SpriteLine %#................##....# ;5
+SpriteLine %#.................#....# ;6
+SpriteLine %#.................#....# ;7
+SpriteLine %#.................#....# ;8
+SpriteLine %#................##....# ;9
+SpriteLine %#....#############.....# ;10
+SpriteLine %#...##.................# ;11
+SpriteLine %#...#..................# ;12
+SpriteLine %#...#..................# ;13
+SpriteLine %#...#..................# ;14
+SpriteLine %#...#..................# ;15
+SpriteLine %#...#..................# ;16
+SpriteLine %#...#..................# ;17
+SpriteLine %#...###############....# ;18
+SpriteLine %#......................# ;19
+SpriteLine %#......................# ;20
+SpriteLine %######################## ;21
!by 0

sprite_last_ptr = * / 64
;            765432107654321076543210
+SpriteLine %######################## ;1
+SpriteLine %#......................# ;2
+SpriteLine %#......................# ;3
+SpriteLine %#..#################...# ;4
+SpriteLine %#..................#...# ;5
+SpriteLine %#..................#...# ;6
+SpriteLine %#..................#...# ;7
+SpriteLine %#..................#...# ;8
+SpriteLine %#..................#...# ;9
+SpriteLine %#..#################...# ;10
+SpriteLine %#..................#...# ;11
+SpriteLine %#..................#...# ;12
+SpriteLine %#..................#...# ;13
+SpriteLine %#..................#...# ;14
+SpriteLine %#..................#...# ;15
+SpriteLine %#..................#...# ;16
+SpriteLine %#..................#...# ;17
+SpriteLine %#..#################...# ;18
+SpriteLine %#......................# ;19
+SpriteLine %#......................# ;20
+SpriteLine %######################## ;21
!by 0

