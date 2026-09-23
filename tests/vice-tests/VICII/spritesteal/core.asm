; spritesteal
; -----------
; 2010 Hannu Nuotio
; Raster IRQ code from: http://codebase64.org/doku.php?id=base:double_irq

; For measuring amount of stolen cycles by sprites.

!ct scr

; --- Consts

color_back = $00
color_fore = $0e
color_normal = color_fore
color_border_done = $0f
color_ok = $05
color_error = $02

raster = 6              ; start of raster interrupt
testline_first = 10     ; first sprite test line
testline_last = 8       ; last sprite test line (-1)
testline_steals = 9     ; testline with stealing on sprites
; initial timer value
timer_init = $0355 + (CYCLES - 63)
delay_init = 64         ; initial delay value

cinv = $fffe
cnmi = $fffa

screen = $0400


; --- Variables

; - zero page

tmpptr = $f4            ; temp pointer
flag_measure = $f7      ; flag: does the IRQ do the measurements?
                        ; (reset when measurements have been done)
found_error = $f8       ; flag: error found on measurement
strptr = $fa            ; pointer to string
scrptr = $fc            ; pointer to screen
curtest_delay = $fe     ; delay for current test (63..0)
curtest_num = $ff       ; offset to result buffer (0..63)


; --- Code

* = $0801
basic:
; BASIC stub: "1 SYS 2061"
!by $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00

start:
    jsr setup
    jsr install
    jmp main


; -- IRQ handlers

nmi:
    rti

irq1:
    sta reseta1 ;Preserve A,X and Y
    stx resetx1 ;Registers
    sty resety1 ;VIA self modifying
                ;code
                ;(Faster than the
                ;STACK is!)

    lda #<irq2  ;Set IRQ Vector
    ldx #>irq2  ;to point to the
                ;next part of the
    sta $fffe   ;Stable IRQ
    stx $ffff   ;ON NEXT LINE!
    inc $d012
    asl $d019   ;Ack RASTER IRQ
    tsx         ;We want the IRQ
    cli         ;To return to our
    nop         ;endless loop
    nop         ;NOT THE END OF
    nop         ;THIS IRQ!
    nop
    nop         ;Execute nop's
    nop         ;until next RASTER
    nop         ;IRQ Triggers
    nop
    nop         ;2 cycles per
    nop         ;instruction so
    nop         ;we will be within
    nop         ;1 cycle of RASTER
    nop         ;Register change
    nop
irq2:
    txs         ;Restore STACK
                ;Pointer
    ldx #$08    ;Wait exactly 1
    dex         ;lines worth of
    bne *-1     ;cycles for compare, minus compare
!if CYCLES = 64 {
    nop
    nop
} else {
    bit $ea
}
!if CYCLES = 65 {
    nop
}

    ldx $d012
    cpx $d012
    beq irq_test   ;If no waste 1 more cycle

; start action here
irq_test:
    ; check if we're measuring this time
    lda flag_measure
    beq endirq2

    ; - do measurement
    ; set up timer
    lda #<timer_init
    sta $dc04   ; timer A low
    lda #>timer_init
    sta $dc05   ; timer A high
    ; count CPU cycles, force load, one shot, start
    lda #%00010001
    sta $dc0e

    ; do delay
    lda curtest_delay
    jsr delay

    ; waste a few cycles (first measure read hits cycle 51 of line 9)
    ldy #10
-   dey
    bne -
    bit $ea
!if CYCLES = 64 {
    bit $ea
} else {
    nop
}
!if CYCLES = 65 {
    nop
}

actual_measure:
    ; read timer A low
    lda $dc04

    ; store result
    ldy curtest_num
    sta result,y

    ; next test
    dec curtest_delay
    iny
    sty curtest_num
    cpy #64
    bne endirq2

    ; measurements done, signal main loop
    lda #0
    sta flag_measure
    sta curtest_num
    lda #delay_init
    sta curtest_delay

    ; handler finished
endirq2:

    lda #raster
    sta $d012

    lda #<irq1  ;Set IRQ to point
    ldx #>irq1  ;to subsequent IRQ
    sta $fffe
    stx $ffff
    asl $d019   ;Ack RASTER IRQ

    lda #$00    ;Reload A,X,and Y
reseta1  = *-1  ;registers
    ldx #$00
resetx1  = *-1
    ldy #$00
resety1  = *-1
    rti         ;Return from IRQ


; - delay
;
!align 63, 0
delay:              ;delay 80-accu cycles, 0<=accu<=64
    lsr             ;2 cycles akku=akku/2 carry=1 if accu was odd, 0 otherwise
    bcc waste1cycle ;2/3 cycles, depending on lowest bit, same operation for both
waste1cycle:
    sta smod+1      ;4 cycles selfmodifies the argument of branch
    clc             ;2 cycles
;now we have burned 10/11 cycles.. and jumping into a nopfield
smod:
    bcc *+10
!by $ea,$ea,$ea,$ea,$ea,$ea,$ea,$ea
!by $ea,$ea,$ea,$ea,$ea,$ea,$ea,$ea
!by $ea,$ea,$ea,$ea,$ea,$ea,$ea,$ea
!by $ea,$ea,$ea,$ea,$ea,$ea,$ea,$ea
    rts             ;6 cycles


; -- Main

main:
    ; reset counters for first test
    lda #0
    sta flag_measure
    sta curtest_num
    lda #delay_init
    sta curtest_delay
    lda #testline_first
    sta curtest_line

testloop:
    ; normal border color
    lda #color_normal
    sta $d020

    ; setup test
    jsr testsetup

    ; do subtests
    jsr do_subtests

    ; next test
    jsr testnext

    ; check if last test
    lda #testline_last
    cmp curtest_line
    bne testloop

    ; tests over
    lda #color_border_done
    sta $d020

    lda #$00 ; ok
    sta $d7ff

    ; print finished message
    lda #<statusloc
    sta scrptr
    lda #>statusloc
    sta scrptr+1
    lda #<message_finished
    sta strptr
    lda #>message_finished
    sta strptr+1
    jsr print_text

    ; wait for key
-   jsr checkkey
    beq -

    jmp main


; -- Subroutines

; - do_subtests
;
do_subtests:
    lda #0
    sta curtest_enable
    jsr calc_expected
    jsr do_test

    lda #$01
    sta curtest_enable
-   jsr calc_expected
    jsr do_test
    asl curtest_enable
    bne -

    lda #$ff
    sta curtest_enable
    jsr calc_expected
    jsr do_test

    rts

; - calc_expected
;
calc_expected:
    ; calculate expected
    lda #$00

    ldx curtest_line
    cpx #testline_steals
    bne +
    ; at the line with sprite DMA
    lda curtest_enable
+   sta calc_expected_mask

    ; initial counter value + 1
    lda #0
    sta tmpptr
    ; test counter
    ldx #0
    ; table index-1
    ldy #$ff

    ; next cycle
-   iny
    ; timer counts down
    dec tmpptr
    ; check if sprite DMA steals this cycle
    lda sprite_steal_table,y
calc_expected_mask = * + 1
    and #$12
    bne -
    ; get simulated timer value
    lda tmpptr
    sta expected,x
    ; next test
    inx
    cpx #63
    bne -
    rts

; - do_test
;
do_test:
    ; point to expected location
    lda #<expectedloc
    sta scrptr
    lda #>expectedloc
    sta scrptr+1

    ; print expected values
    ldy #0
    ldx #color_normal
-   lda expected,y
    jsr print_hex
    jsr print_space
    jsr print_space
    iny
    cpy #63
    bne -

    ; print enable
    lda #<testspreloc
    sta scrptr
    lda #>testspreloc
    sta scrptr+1
    lda curtest_enable
    ; setup sprite enable
    sta $d015
    ldx #color_normal
    jsr print_hex

    ; print measuring...
    lda #<statusloc
    sta scrptr
    lda #>statusloc
    sta scrptr+1
    lda #<message_measuring
    sta strptr
    lda #>message_measuring
    sta strptr+1
    jsr print_text

    ; signal IRQ to measure
    lda #1
    sta flag_measure

    ; wait for measuring to finish
-   lda flag_measure
    bne -

    ; check results, print if differing
    jsr testerror
    rts

; - testerror
;
testerror:
    lda #0
    sta found_error

    ; point to result location
    lda #<resultloc
    sta scrptr
    lda #>resultloc
    sta scrptr+1

    ; print measured values
    ldy #0
-   lda result,y
    cmp expected,y
    beq +

    ; error found
    sta testerror_diff
    inc found_error
    ldx #color_error
    jsr print_hex
    lda expected,y
    sec
testerror_diff = * + 1
    sbc #0
    ldx #color_normal
    jsr print_hex
    bne ++

+   ldx #color_ok
    jsr print_hex
    jsr print_space
    jsr print_space

++  iny
    cpy #63
    bne -

    lda found_error
    bne +
    rts

    ; print error message
+   lda #<statusloc
    sta scrptr
    lda #>statusloc
    sta scrptr+1
    lda #<message_error
    sta strptr
    lda #>message_error
    sta strptr+1
    jsr print_text

    ; change border color
    lda #color_error
    sta $d020

    lda #$ff ; failure
    sta $d7ff

    ; wait for key
-   jsr checkkey
    beq -

    ; restore border color
    lda #color_normal
    sta $d020

    rts


; - testsetup
;
testsetup:
    ; set sprite y
    lda curtest_line
    sta $d001
    sta $d003
    sta $d005
    sta $d007
    sta $d009
    sta $d00b
    sta $d00d
    sta $d00f

    ; print line number
    lda #<testlineloc
    sta scrptr
    lda #>testlineloc
    sta scrptr+1
    lda curtest_line
    ldx #color_normal
    jsr print_hex

    rts


; - testnext
;
testnext:
    dec curtest_line
    rts


; - install
;
install:        ; install the raster routine
    sei         ;Disable IRQs
    lda #$7f    ;Disable CIA IRQs
    sta $dc0d
    sta $dd0d

    lda #$35    ;Bank out kernal and basic
    sta $01     ;$e000-$ffff

    lda #<irq1  ;Install RASTER IRQ
    ldx #>irq1  ;into Hardware
    sta $fffe   ;Interrupt Vector
    stx $ffff

    lda #$01    ;Enable RASTER IRQs
    sta $d01a
    lda #raster ;IRQ on line #raster
    sta $d012
    lda #$1b    ;High bit (lines 256-311)
    sta $d011

    lda #<nmi
    ldx #>nmi
    sta $fffa
    stx $fffb

    asl $d019   ;Ack any previous
    bit $dc0d   ;IRQs
    bit $dd0d

    cli         ;Allow IRQ's
    rts

; - deinstall
;
deinstall:      ; install the raster routine
    lda #$37    ;Bank out kernal and basic
    sta $01     ;$e000-$ffff
    asl $d019   ;Ack any previous
    bit $dc0d   ;IRQs
    bit $dd0d

    cli         ;Allow IRQ's
    rts

; - setup
;
setup:
    ; disable measuring
    lda #0
    sta flag_measure

    ; set screen colors
    lda #color_fore
    ldx #0
-   sta $d800,x
    sta $d900,x
    sta $da00,x
    sta $dae8,x
    inx
    bne -

    lda #color_normal
    sta $d020

    lda #color_back
    sta $d021

    ; set sprite y
    lda #$55
    sta $d001
    sta $d003
    sta $d005
    sta $d007
    sta $d009
    sta $d00b
    sta $d00d
    sta $d00f

    ; enable sprites
    lda #$ff
    sta $d015

    ; wait a few frames to get rid of mcbase oddities
    ldy #7
--  lda $d011
    bmi --
-   lda $d011
    bpl -
    dey
    bne --

    ; disable sprites
    lda #0
    sta $d015

    ; print message and results
    lda #<screen
    sta scrptr
    lda #>screen
    sta scrptr+1
    lda #<message
    sta strptr
    lda #>message
    sta strptr+1
    jsr print_text

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
+   rts             ; return


; - print_space
; parameters:
;  scrptr -> screen location to print to
;
print_space:
    tya
    pha
    ldy #0
    lda #' '
    sta (scrptr),y  ; print char
    inc scrptr      ; update scrptr to next char
    bne +
    inc scrptr+1
+   pla
    tay
    rts             ; return

; - print_xx
; parameters:
;  scrptr -> screen location to print to
;
print_xx:
    tya
    pha
    ldy #0
    lda #'x'
    sta (scrptr),y  ; print char
    inc scrptr      ; update scrptr to next char
    bne +
    inc scrptr+1
+   sta (scrptr),y  ; print char
    inc scrptr      ; update scrptr to next char
    bne +
    inc scrptr+1
+   pla
    tay
    rts             ; return


; - print_hex
; parameters:
;  scrptr -> screen location to print to
;  x = color to print
;  a = value to print
; returns:
;  scrptr += 2
;
print_hex:
    sty print_hex_y
    stx print_hex_color
    pha
    ; store a to x
    tax
    ; set up tmpptr
    lda scrptr
    sta tmpptr
    lda scrptr+1
    clc
    adc #>($d800 - screen)
    sta tmpptr+1
    ; restore a
    txa
    ; mask lower
    and #$0f
    ; lookup
    tax
    lda hex_lut,x
    ; print
    ldy #1
    sta (scrptr),y
    lda print_hex_color
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
    sta (scrptr),y
    lda print_hex_color
    sta (tmpptr),y
    ; scrptr+=2
    inc scrptr
    bne +
    inc scrptr+1
+   inc scrptr
    bne +
    inc scrptr+1
print_hex_y = * + 1
+   ldy #0
print_hex_color = * + 1
    ldx #0
    rts


; - checkkey
; returns:
;   z - not pressed
;
checkkey:
    ; wait for line 256 (> last DMA line)
-   lda $d011
    bmi -
-   lda $d011
    bpl -
    ; read matrix state
    lda $dc01
    tay
    ; compare to previous
    eor lastkey
    and lastkey
    ; store last state
    sty lastkey
    rts


; --- Data

lastkey:
!by $ff

; hex lookup table
hex_lut:
!scr "0123456789abcdef"

; LUT for first cycles of sprite steal cycles
sprite_steal_table:
!if CYCLES = 63 {
; cycle 51-53
!by 0,0,0
; cycle 54-62
!by $01,$01,$03,$03,$07,$06,$0e,$0c,$1c
; cycle 0-9
!by $18,$38,$30,$70,$60,$e0,$c0,$c0,$80,$80
; cycle 10-50           20                  30                  40                  50
!by 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
; cycle 51-53
!by 0,0,0
; cycle 54-62
!by $01,$01,$03,$03,$07,$06,$0e,$0c,$1c
; cycle 0-9
!by $18,$38,$30,$70,$60,$e0,$c0,$c0,$80,$80
; cycle 10-50           20                  30                  40                  50
!by 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
}
!if CYCLES = 65 {
; cycle 51-54
!by 0,0,0,0
; cycle 55-64
!by $01,$01,$03,$03,$07,$06,$0e,$0c,$1c,$18
; cycle 0-8
!by $38,$30,$70,$60,$e0,$c0,$c0,$80,$80
; cycle 9-50              20                  30                  40                  50
!by 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
; cycle 51-54
!by 0,0,0,0
; cycle 55-64
!by $01,$01,$03,$03,$07,$06,$0e,$0c,$1c,$18
; cycle 0-8
!by $38,$30,$70,$60,$e0,$c0,$c0,$80,$80
; cycle 9-50              20                  30                  40                  50
!by 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
}
!if CYCLES = 64 {
; cycle 51-54
!by 0,0,0,0
; cycle 55-63
!by $01,$01,$03,$03,$07,$06,$0e,$0c,$1c
; cycle 0-9
!by $18,$38,$30,$70,$60,$e0,$c0,$c0,$80,$80
; cycle 10-50           20                  30                  40                  50
!by 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
; cycle 51-54
!by 0,0,0,0
; cycle 55-63
!by $01,$01,$03,$03,$07,$06,$0e,$0c,$1c
; cycle 0-9
!by $18,$38,$30,$70,$60,$e0,$c0,$c0,$80,$80
; cycle 10-50           20                  30                  40                  50
!by 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
}

curtest_line:
!by 0
curtest_enable:
!by 0

; text
message:
;     |---------0---------0---------0--------|
!scr "spritesteal - sprite dma testprog v3"
!if CYCLES = 65 {
!scr "ntsc"
}
!if CYCLES = 64 {
!scr "oldn"
}
!if CYCLES = 63 {
!scr "-pal"
}
!scr "timer reads, first on line 9 cycle 51   "
!scr "                                        "
!scr "test: line/sprite enable = "
testlineloc = * - message + screen
testspreloc = testlineloc + 3
!scr                            "  /          "
!scr "                                        "
!scr "measured(&diff):                        "
resultloc = * - message + screen
!scr "                                        "
!scr "                                        "
!scr "                                        "
!scr "                                        "
!scr "                                        "
!scr "                                        "
!scr "                                        "
!scr "                                        "
!scr "expected:                               "
expectedloc = * - message + screen
!scr "                                        "
!scr "                                        "
!scr "                                        "
!scr "                                        "
!scr "                                        "
!scr "                                        "
!scr "                                        "
!scr "                                        "
statusloc = * - message + screen
!scr "                                        "
message_empty_line
!scr "                                        "
!by 0

message_error:
!scr "difference found! space to continue.    "
!by 0

message_measuring:
!scr "measuring...                            "
!by 0

message_finished:
!scr "all tests finished. space to test again."
!by 0

!align 63,0,0

; measured result
result = *

; expected (simulated) results
expected = * + 64
