; phi1timing
; ----------
; 2010 Hannu Nuotio
; Raster IRQ code from: http://codebase64.org/doku.php?id=base:double_irq

; Determines PHI1 fetch type on each cycle.

!ct scr

; --- Consts

phi1_loc = $dead    ; location to use for phi1 measurements

color_back = $06
color_fore = $0e
color_border = color_fore

raster = 2          ; start of raster interrupt

cinv = $fffe
cnmi = $fffa

screen = $0400


; --- Variables

; - zero page


flag_measure = $fb      ; flag: does the IRQ do the measurements?
                        ; (reset when measurements have been done)
strptr = $fc            ; pointer to string
scrptr = $fe            ; pointer to screen


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
    beq testloop_pre   ;If no waste 1 more cycle

; start action here
testloop_pre:
    inc $d020
    dec $d020

    ; check if we're measuring this time
    lda flag_measure
    beq endirq2

    jsr irq_measure

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


; -- Measuring routine

irq_measure:
    ; pre delay - first read should hit cycle 0
    ldx #5
-   dex
    bne -
    nop
    nop
!if CYCLES = 64 {
    bit $ea
} else {
    nop
}
!if CYCLES = 65 {
    nop
}

    ldy #0
measure_loop:
    ; read what the VICII has just fetched
    lda phi1_loc
    ; ...and store it to the buffer
    sta measure_buffer,y

    ; show a line
    inc $d020
    dec $d020

    ; waste time
    ldx #6
-   dex
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
    ; next cycle
    iny
    cpy #CYCLES
    bne measure_loop

    ; measurements done, signal main loop
    lda #0
    sta flag_measure

    rts


; -- Main

main:
    ; enable ECM
    lda #$4b
    sta $d011

    ; signal IRQ to measure
    lda #1
    sta flag_measure

    ; wait for measuring to finish
-   lda flag_measure
    bne -

    ; analyse measurements
    ldy #0
-   lda measure_buffer,y
    jsr translate_measured
    sta result_buffer,y
    iny
    cpy #CYCLES
    bne -

    ; check measurements
    ldx #5
    ldy #0
-   lda result_buffer,y
    cmp reference,y
    beq +
    ldx #10
+
    iny
    cpy #CYCLES
    bne -
    stx $d020

    lda $d020
    and #$0f
    ldx #0 ; success
    cmp #5
    beq nofail
    ldx #$ff ; failure
nofail:
    stx $d7ff

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

    ; enable screen
    lda #$1b
    sta $d011

    ; wait for key
-   lda $dc01
    eor #$ff
    beq -
    jmp main


; -- Subroutines

; - translate_measured
; parameters:
;  a = measured value
; returns:
;  a = type
;
translate_measured:
    cmp #$aa
    bne +
    lda #'g'    ; graphics
    rts
+   cmp #$55
    bne +
    lda #'r'    ; refresh
    rts
+   cmp #$ff
    bne +
    lda #'i'    ; idle
    rts
+   cmp #$08
    bcs +
    adc #'0'    ; sprite pointer 0..7
    rts
+   lda #'?'    ; unknown
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
    lda #$4b    ;High bit (lines 256-311)
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
deinstall:        ; install the raster routine
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

    lda #color_border
    sta $d020

    lda #color_back
    sta $d021

    ; set up $3fxx
    ldy #0
-   lda #$55 ;tya
    sta $3f00,y
    iny
    bne -

    lda #$ff
    sta $3fff

    ; set up sprite pointers
    ldy #0
-   tya
    sta $07f8,y
    iny
    cpy #8
    bne -

    ; set up $39ff
    lda #$aa
    sta $39ff
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

; text
message:
;     |---------0---------0---------0--------|
!scr "phi1timing v2 "
!if CYCLES = 65 {
!scr "ntsc"
}
!if CYCLES = 64 {
!scr "oldn"
}
!if CYCLES = 63 {
!scr "pal "
}
!scr                   " - phi1 timing check  "
!scr "                                        "
!scr "measured:                               "
result_buffer:
!fi CYCLES, '?'
!fill 40-(CYCLES-40), ' '
!scr "                                        "
!scr "reference:                              "
reference:
!if CYCLES = 63 {
!scr "3i4i5i6i7irrrrrggggggggggggggggggggggggg"
!scr "gggggggggggggggii0i1i2i                 "
}
!if CYCLES = 64 {
!scr "3i4i5i6i7irrrrrggggggggggggggggggggggggg"
!scr "gggggggggggggggiii0i1i2i                "
}
!if CYCLES = 65 {
!scr "i4i5i6i7iirrrrrggggggggggggggggggggggggg"
!scr "gggggggggggggggiii0i1i2i3               "
}
!scr "                                        "
!scr "0-7 = sprite pointer fetch              "
!scr " g  = graphics fetch ($39ff, ecm)       "
!scr " i  = idle fetch ($39ff)                "
!scr " r  = refresh ($3fxx)                   "
!scr " ?  = unknown                           "
!scr "                                        "
!scr "press space to measure again.           "
!scr "                                        "



!align 255, 0, 0
; buffer for measured data
measure_buffer:
!fi 66, 0

; previous measurements
previous_buffer:
!fi 66, 0

; - results:
;  g   = graphics fetch ($39ff, ext mode)
;  0-7 = sprite N pointer fetch
;  i   = idle fetch ($3fff)
;  r   = refresh ($3fXX)
;  ?   = unknown
;
