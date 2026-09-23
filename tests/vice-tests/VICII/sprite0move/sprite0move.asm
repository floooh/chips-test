; sprite0move
; -----------
; based on testprogs/VICII/gfxfetch/gfxfetch.asm

; Opens side border, displays test sprite (#0), lets the user move it.

; NOTE! Only PAL tested.


; --- Macros

; SpriteLine - for easy definition of sprites
; from "ddrv.a" by Marco Baye
!macro SpriteLine .v {
!by .v>>16, (.v>>8)&255, .v&255
}


; --- Consts

; Select the video timing (processor clock cycles per raster line)
;CYCLES = 65     ; 6567R8 and above, NTSC-M
;CYCLES = 64    ; 6567R5 6A, NTSC-M
;CYCLES = 63    ; 6569 (all revisions), PAL-B

raster = 46          ; start of raster interrupt
spritey = 51         ; sprite 0 Y

cinv = $0314
cnmi = $0318
screen = $0400
sprite0ptr = $7f8    ; pointer location

start_sprite0_x = 320
start_sprite1_x = 280
start_sprite2_x = 230
start_sprite3_x = 180
start_sprite4_x = 130
start_sprite5_x = 80
start_sprite6_x = 30
start_sprite7_x = 0

GETIN = $ffe4           ; ret: a = 0: no keys pressed, otherwise a = ASCII code

key_sprite_l = $41
key_sprite_r = $44
key_sprite_ll = $53
key_sprite_rr = $57
key_sprite_gfx_next = $52
key_sprite_gfx_prev = $46
key_sprite_next = $54
key_sprite_prev = $47
key_sprite_next_color = $59
key_sprite_prev_color = $48
key_change_3fff = $58
key_toggle_xexp = $51
key_toggle_mc = $45
key_sprite_next_mc_0 = $55
key_sprite_prev_mc_0 = $4a
key_sprite_next_mc_1 = $49
key_sprite_prev_mc_1 = $4b

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
testloop_pre:
    ; delay badline by messing with yscroll
    lda $d012
    sec
    sbc #1
    and #$07
    sta testloop_newyscroll_1
    lda $d011
    and #$f8
testloop_newyscroll_1 = * + 1
    ora #$12
    sta $d011

    ; prepare $d015 for opening the border
    lda #$0f
    sta $d016

    ; open the border two lines before the sprite fetches
    lda zpzero
    sta $d016

    ; prepare $d015 for opening the border
    lda #$0f
    sta $d016

    ldx #9
-   dex
    bne -
    nop
    nop

    ; open the border on the line before the sprite fetches
    lda zpzero
    sta $d016

    ; prepare $d015 for opening the border
    lda #$0f
    sta $d016

    ; loops
    ldy #21

    ldx #9
-   dex
    bne -
    beq *+2

testloop:
    ; open side border
    inc $d016
    dec $d016

    ; delay badline by messing with yscroll
    lda $d012
    sec
    sbc #1
    and #$07
    sta testloop_newyscroll
    lda $d011
    and #$f8
testloop_newyscroll = * + 1
    ora #$12
    sta $d011
    nop
    dey
    bne testloop

    nop
    ; open side border once more
    inc $d016
    dec $d016

    ; restore d016
    lda #$08
    sta $d016

    ; check sprite-sprite collisions
    lda $d01e
    beq +
    ; collision happened -> change border
    ldx #2
    stx $d020
+   tax
    ; display collision register
    lda #<text_location_sprite_coll
    sta tmpptr
    lda #>text_location_sprite_coll
    sta tmpptr+1
    txa
    jsr printhex

    lda #$80
postloop:
    cmp $d012
    bne postloop
endirq:
    ; restore border color
    lda #0
    sta $d020

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


; --  Main loop

mainloop:
    jsr print_sprite_num
    jsr print_sprite_loc

mainloop_wait:
    lda #$ff
-   cmp $d012
    bne -
    jsr GETIN
    beq mainloop_wait

!if 0 {
    ; print pressed key
    pha
    tax
    lda #<text_location_sprite_x + 5
    sta tmpptr
    lda #>text_location_sprite_x
    sta tmpptr+1
    txa
    jsr printhex
    pla
}

++  cmp #key_sprite_gfx_next
    bne ++
    ; next sprite gfx
    lda #sprite_last_ptr
    cmp spriteptr
    beq mainloop_wait
    inc spriteptr
    ldy spritenum
    jsr set_sprite_ptr
    jmp mainloop_wait

++  cmp #key_sprite_gfx_prev
    bne ++
    ; prev sprite gfx
    lda #sprite_first_ptr
    cmp spriteptr
    beq mainloop_wait
    dec spriteptr
    ldy spritenum
    jsr set_sprite_ptr
    jmp mainloop_wait

++  cmp #key_sprite_next
    bne ++
    ; next sprite
    inc spritenum
    lda #8
    cmp spritenum
    bne mainloop
    dec spritenum
    jmp mainloop_wait

++  cmp #key_sprite_prev
    bne ++
    ; prev sprite
    dec spritenum
    bpl mainloop
    inc spritenum
    jmp mainloop_wait

++  cmp #key_sprite_next_color
    bne ++
    ; next sprite color
    ldx spritenum
    inc $d027,x
    jmp mainloop_wait

++  cmp #key_sprite_prev_color
    bne ++
    ; prev sprite color
    ldx spritenum
    dec $d027,x
    jmp mainloop_wait

++  cmp #key_sprite_next_mc_0
    bne ++
    ; next sprite mc 0
    inc $d025
    jmp mainloop_wait

++  cmp #key_sprite_prev_mc_0
    bne ++
    ; prev sprite mc 0
    dec $d025
    jmp mainloop_wait

++  cmp #key_sprite_next_mc_1
    bne ++
    ; next sprite mc 1
    inc $d026
    jmp mainloop_wait

++  cmp #key_sprite_prev_mc_1
    bne ++
    ; prev sprite mc 1
    dec $d026
    jmp mainloop_wait

++  cmp #key_toggle_xexp
    bne ++
    ; toggle x expand
    ldx #$1d
    jsr toggle_bit
    jmp mainloop_wait

++  cmp #key_toggle_mc
    bne ++
    ; toggle multicolor
    ldx #$1c
    jsr toggle_bit
    jmp mainloop_wait

++  cmp #key_sprite_l
    bne ++
    ; spritex--
    lda #-1
    ldy spritenum
    jsr move_sprite
    jmp mainloop

++  cmp #key_sprite_ll
    bne ++
    ; spritex -= 16
    lda #-$10
    ldy spritenum
    jsr move_sprite
    jmp mainloop

++  cmp #key_sprite_r
    bne ++
    ; spritex++
    lda #$01
    ldy spritenum
    jsr move_sprite
    jmp mainloop

++  cmp #key_sprite_rr
    bne ++
    ; spritex += 16
    lda #$10
    ldy spritenum
    jsr move_sprite
    jmp mainloop

++  cmp #key_change_3fff
    bne ++
    ; update 3fff
    ldy spritenum
    lda sprite0x,y
    sta $3fff
    jmp mainloop

++  jmp mainloop_wait


; -- Subroutines

; - setup
;
setup:
    lda #0
    sta $3fff

    lda #0
    sta zpzero

    ; set screen color
    lda 646
    ldx #0
col_lp:
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


; - toggle_bit
;  x = register addr LSB
;
toggle_bit:
    ldy spritenum
    lda bitmask,y
    eor $d000,x
    sta $d000,x
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
    lda spriteptr
    sta sprite0ptr,y
    rts

; - move_sprite
; parameter:
;  a = amount (msb = direction)
;  y = sprite number
;
move_sprite:
    ldx #$90    ; opcode for bcc
    sta move_sprite_amount
    asl         ; test msb
    bcc +
    ldx #$b0    ; opcode for bcs
+   stx move_sprite_type
    lda sprite0x,y
    clc
move_sprite_amount = * + 1
    adc #$12
    sta sprite0x,y
move_sprite_type:
    bcc +
    lda sprite0xh
    eor bitmask,y
    sta sprite0xh
+   jsr set_sprite_x
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


; - print_sprite_loc
;
print_sprite_loc:
    lda #<text_location_sprite_x
    sta tmpptr
    lda #>text_location_sprite_x
    sta tmpptr+1
    lda sprite0xh
    ldy spritenum
    cpy #0
    beq +
-   lsr
    dey
    bne -
+   and #$01
    jsr printhex
    ldy spritenum
    lda sprite0x,y
    jsr printhex
    rts

; - print_sprite_num
;
print_sprite_num:
    lda #<text_location_sprite_n
    sta tmpptr
    lda #>text_location_sprite_n
    sta tmpptr+1
    ldx spritenum
    lda hex_lut,x
    ldy #0
    sta (tmpptr),y
    rts

; - printhex
; parameters:
;  tmpptr:h -> screen location to print to
;  a = value to print
; returns:
;  tmpptr:h += 2
;
printhex:
    pha
    ; mask lower
    and #$0f
    ; lookup
    tax
    lda hex_lut,x
    ; print
    ldy #1
    sta (tmpptr),y
    ; lsr x4
    pla
    lsr
    lsr
    lsr
    lsr
    ; lookup
    tax
    lda hex_lut,x
    ; print
    dey
    sta (tmpptr),y
    ; tmpptr+=2
    inc tmpptr
    bne +
    inc tmpptr+1
+   inc tmpptr
    bne +
    inc tmpptr+1
+   rts



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
!by sprite_first_ptr
!by sprite_first_ptr
!by sprite_first_ptr
!by sprite_first_ptr
!by sprite_first_ptr
!by sprite_first_ptr
!by sprite_first_ptr

!ct scr

; hex lookup table
hex_lut: !tx "0123456789abcdef"

; sprite bits
bitmask: !by $01, $02, $04, $08, $10, $20, $40, $80

; help text
message:
;    |---------0---------0---------0--------|
!tx "                                        "
!tx "sprites on border testprog              "
!tx "                                        "
!tx "controls:                               "
!tx " a/d  - move 1 pixel left/right         "
!tx " s/w  - move 16 pixels left/right       "
!tx " r/f  - next/previous sprite gfx        "
!tx " t/g  - next/previous sprite            "
!tx " y/h  - next/previous sprite color      "
!tx " u/j  - next/previous multicolor 0      "
!tx " i/k  - next/previous multicolor 1      "
!tx " x    - copy sprite x lbs to $3fff      "
!tx " q    - toggle x expand                 "
!tx " e    - toggle multicolor               "
!tx "                                        "
!tx "sprite: "
text_location_sprite_n = * - message + screen
!tx         " , location: "
text_location_sprite_x = * - message + screen
!tx                      "    , collision: "
text_location_sprite_coll = * - message + screen
!tx                                       "  "
!tx "                                        "
!tx "                                        "

!by 0

; sprites
* = $2000
spritedata:
sprite_first_ptr = * / 64
;            765432107654321076543210
+SpriteLine %#....................... ;1
+SpriteLine %.#...................... ;2
+SpriteLine %..#..................... ;3
+SpriteLine %...#.................... ;4
+SpriteLine %....#................... ;5
+SpriteLine %.....#.................. ;6
+SpriteLine %......#................. ;7
+SpriteLine %.......#................ ;8
+SpriteLine %........#............... ;9
+SpriteLine %.........#.............. ;10
+SpriteLine %..........#............. ;11
+SpriteLine %...........#............ ;12
+SpriteLine %............#........... ;13
+SpriteLine %.............#.......... ;14
+SpriteLine %..............#......... ;15
+SpriteLine %...............#........ ;16
+SpriteLine %................#....... ;17
+SpriteLine %.................#...... ;18
+SpriteLine %..................#..... ;19
+SpriteLine %...................#.... ;20
+SpriteLine %....................#... ;21
!by 0

;            765432107654321076543210
+SpriteLine %#....................... ;1
+SpriteLine %#....................... ;2
+SpriteLine %.#...................... ;3
+SpriteLine %.#...................... ;4
+SpriteLine %##...................... ;5
+SpriteLine %##...................... ;6
+SpriteLine %#....................... ;7
+SpriteLine %#....................... ;8
+SpriteLine %.#...................... ;9
+SpriteLine %.#...................... ;10
+SpriteLine %##...................... ;11
+SpriteLine %##...................... ;12
+SpriteLine %........................ ;13
+SpriteLine %........................ ;14
+SpriteLine %#.#.#.#.#.#.#.#.#.#.#.#. ;15
+SpriteLine %........................ ;16
+SpriteLine %.#.#.#.#.#.#.#.#.#.#.#.# ;17
+SpriteLine %........................ ;18
+SpriteLine %######################## ;19
+SpriteLine %........................ ;20
+SpriteLine %#..####..####..###..#..# ;21
!by 0


;            765432107654321076543210
+SpriteLine %#.......#.......#....... ;1
+SpriteLine %#.......#.......#....... ;2
+SpriteLine %#.......#.......#....... ;3
+SpriteLine %#.......#.......#....... ;4
+SpriteLine %#.......#.......#....... ;5
+SpriteLine %#.......#.......#....... ;6
+SpriteLine %#.......#.......#....... ;7
+SpriteLine %#.......#.......#....... ;8
+SpriteLine %#.......#.......#....... ;9
+SpriteLine %#.......#.......#....... ;10
+SpriteLine %#.......#.......#....... ;11
+SpriteLine %#.......#.......#....... ;12
+SpriteLine %#.......#.......#....... ;13
+SpriteLine %#.......#.......#....... ;14
+SpriteLine %#.......#.......#....... ;15
+SpriteLine %#.......#.......#....... ;16
+SpriteLine %#.......#.......#....... ;17
+SpriteLine %#.......#.......#....... ;18
+SpriteLine %#.......#.......#....... ;19
+SpriteLine %#.......#.......#....... ;20
+SpriteLine %#.......#.......#....... ;21
!by 0

;            765432107654321076543210
+SpriteLine %######################## ;1
+SpriteLine %#....................... ;2
+SpriteLine %#....................... ;3
+SpriteLine %#....................... ;4
+SpriteLine %#....................... ;5
+SpriteLine %#....................... ;6
+SpriteLine %#....................... ;7
+SpriteLine %#....................... ;8
+SpriteLine %#....................... ;9
+SpriteLine %#....................... ;10
+SpriteLine %#....................... ;11
+SpriteLine %#....................... ;12
+SpriteLine %#....................... ;13
+SpriteLine %#....................... ;14
+SpriteLine %#....................... ;15
+SpriteLine %#....................... ;16
+SpriteLine %#....................... ;17
+SpriteLine %#....................... ;18
+SpriteLine %#....................... ;19
+SpriteLine %#....................... ;20
+SpriteLine %######################## ;21
!by 0


sprite_last_ptr = * / 64
;            765432107654321076543210
+SpriteLine %#....................... ;1
+SpriteLine %#....................... ;2
+SpriteLine %#....................... ;3
+SpriteLine %#....................... ;4
+SpriteLine %#....................... ;5
+SpriteLine %#....................... ;6
+SpriteLine %#....................... ;7
+SpriteLine %#....................... ;8
+SpriteLine %#....................... ;9
+SpriteLine %#....................... ;10
+SpriteLine %#....................... ;11
+SpriteLine %#....................... ;12
+SpriteLine %#....................... ;13
+SpriteLine %#....................... ;14
+SpriteLine %#....................... ;15
+SpriteLine %#....................... ;16
+SpriteLine %#....................... ;17
+SpriteLine %#....................... ;18
+SpriteLine %#....................... ;19
+SpriteLine %#....................... ;20
+SpriteLine %#....................... ;21
!by 0

