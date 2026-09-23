
TESTING = 0

irqline = $20

    .export Start

Start:

    lda #0
    sta $d020
    sta $d021

    jsr initirq

jitter:
    nop         ; 2
    bit $ea     ; 3
    bit $eaea   ; 4

    lda ($ff),y ; 5+1
    lda ($ff,x) ; 6
.if TESTING = 1
    inc $06e8,x ; 7
.endif
    inx
    iny

    jmp jitter

cols:
    .byte 1,1,1,1,1,1
    .byte 2,2,2,2,2,2
    .byte 3,3,3,3,3,3
    .byte 4,4,4,4,4,4
    .byte 5,5,5,5,5,5
    .byte 6,6,6,6,6,6
    .byte 7,7,7,7,7,7

    .import rastersync_lp

tmp = $f0

testlp:

         ldx #$ff
         ldy #$00
         ; prepare cia ports
         stx $dc00     ; port A = $ff (inactive)
         sty $dc02     ; ddr A = $00  (all input)
         stx $dc03     ; ddr B = $ff  (all output)
         stx $dc01     ; port B = $ff (inactive)
         ; now trigger the lp latch
         sty $dc01     ; port B = $00 (active)
         stx $dc01     ; port B = $ff (inactive)
;         lda $d013     ; get x-position (pixels, divided by two)
    lda $d013
    sta tmp
    lda $d014
    sta tmp+1
         ; restore cia setup
         stx $dc02     ; ddr A = $ff  (all output)
         sty $dc03     ; ddr B = $00  (all input)
         stx $dc01     ; port B = $ff (inactive)
         ldx #$7f
         stx $dc00     ; port A = $7f


    lda tmp
    and #$0f
    tax
    lda hextab,x
    sta $0401
    
    lda tmp
    lsr
    lsr
    lsr
    lsr
    and #$0f
    tax
    lda hextab,x
    sta $0400
.if TESTING = 1
    lda tmp
    tax
    sta $0400+(4*40),x
.endif
    lda tmp+1
    and #$0f
    tax
    lda hextab,x
    sta $0404
    
    lda tmp+1
    lsr
    lsr
    lsr
    lsr
    and #$0f
    tax
    lda hextab,x
    sta $0403

    ; wait until at top of frame to avoid grey dot on screen(shot)
    lda $d011
    bpl *-3
    lda $d011
    bmi *-3
    
    ldy #5

    lda tmp
.ifdef NEWVIC
    cmp #1 ; new VIC
    beq showvic
.else
    cmp #2 ; old VIC
    beq showvic
.endif
    ldy #10

    lda #0 ; error

showvic:
    sty $d020

    asl
    asl
    asl
    tax

    ldy #0
slp:
    lda types,x
    and #$3f
    sta $0400+10,y
    inx
    iny
    cpy #8
    bne slp
    
    ; skip check on first frame, as the value will be bogus
@dl:lda #0
    beq @skp1
    
    ldy #0      ; success
    lda $d020
    and #$0f
    cmp #5
    beq @skp
    ldy #$ff    ; failure
@skp:
    sty $d7ff
@skp1:
    inc @dl+1

    rts

    .align 256
        ;*= $2000    ;Assemble to $2000
initirq:
 
         sei         ;Disable IRQ's
         lda #$7f    ;Disable CIA IRQ's
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
         lda #irqline    ;IRQ on line 52
         sta $d012
         lda #$1b    ;High bit (lines 256-311)
         sta $d011
                     ;NOTE double IRQ
                     ;cannot be on or
                     ;around a BAD LINE!
                     ;(Fast Line)
 
         lda #$00
         sta $d015   ;turn off sprites
 
         jsr clrscreen
         jsr clrcolor
         jsr printtext
 
@l1:     lda $d011
         bpl @l1
@l2:     lda $d011
         bmi @l2

         asl $d019   ;Ack any previous
         bit $dc0d   ;IRQ's
         bit $dd0d
 
         cli         ;Allow IRQ's
 
         rts
 
        .repeat 128+7
        .byte 0
        .endrepeat

    .align 256
 
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
         bne *-1     ;cycles for compare
         bit $ea     ;Minus compare
         nop         ;cycles
 
;         nop  ;<--- remove 1 NOP for PAL
 
         lda #irqline + 1    ;RASTER change yet?
         cmp $d012
         beq start   ;If no waste 1 more
                     ;cycle
start:
         nop         ;Some delay
         nop         ;So stable can be
         bit $ea     ;seen
.if TESTING = 1 
         inc $d020   ;6 Here is the proof
         dec $d020   ;6
.else
        bit $eaea ;4
        bit $eaea ;4
        bit $eaea ;4
.endif 
         lda #<irq1  ;Set IRQ to point
         ldx #>irq1  ;to subsequent IRQ
         ldy #irqline    ;at line $68
         sta $fffe
         stx $ffff
         sty $d012
         asl $d019   ;Ack RASTER IRQ

        jsr testlp
 
reseta1  = *+1       ;registers
         lda #$00    ;Reload A,X,and Y
resetx1  = *+1
         ldx #$00
resety1  = *+1
         ldy #$00
 
         rti         ;Return from IRQ
 
clrscreen:
         lda #$20    ;Clear the screen
         ldx #$00
clrscr:   sta $0400,x
         sta $0500,x
         sta $0600,x
         sta $0700,x
         dex
         bne clrscr
         rts
clrcolor:
         lda #$03    ;Clear color memory
         ldx #$00
clrcol:   sta $d800,x
         sta $d900,x
         sta $da00,x
         sta $db00,x
         dex
         bne clrcol
         rts
 
printtext:
         lda #$16    ;C-set = lower case
         sta $d018
 
exit:     rts

types:
    .byte "error   "
    .byte "new/8565"
    .byte "old/6569"

hextab:
    .byte "0123456789",1,2,3,4,5,6,7,8

