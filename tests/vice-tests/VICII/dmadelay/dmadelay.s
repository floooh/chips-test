
;NTSC=1

!if (DEBUG = 0) {
DEBUGCOLOR0=$d020
DEBUGCOLOR1=$d021
} else {
DEBUGCOLOR0=$dbff
DEBUGCOLOR1=$dbff
}

scrptr = $02

*= $0801
!byte $0b,$08,$01,$00,$9e               ; Line 1 SYS2061
!convtab pet
!tx "2061"                                              ; Address for sys start in text
!byte $00,$00,$00

!zn
.theLine = $2c
        ; Stop interrupts, clear decimal mode and backup the previous stack entries
        sei
        cld
        ; Grab everything on the stack
        ldx #$ff
        txs
        ; Set the user requested ROM state
        ldy #$35
        sty $01
        ; Clear all CIA to known state, interrupts off.
        lda #$7f
        sta $dc0d
        sta $dd0d
        lda #0
        sta $d01a
        sta $dc0e
        sta $dc0f
        sta $dd0e
        sta $dd0f
        ; Ack any interrupts that might have happened from the CIAs
        lda $dc0d
        lda $dd0d
        ; Ack any interrupts that have happened from the VIC2
        lda #$ff
        sta $d019

        ; Setup kernal and user mode IRQ vectors to point to a blank routine
        lda #<.initIRQ
        sta $fffe
        lda #>.initIRQ
        sta $ffff

        lda #<.initNMI
        sta $fffa
        lda #>.initNMI
        sta $fffb

        ; Turn off various bits in the VIC2 and SID chips
        ; Screen, sprites and volume are disabled.
        lda #0
        sta $d021
        sta $d020
        sta $d011
        sta $d015

        ; Setup raster IRQ
        lda #<IrqTopOfScreen
        sta $fffe
        lda #>IrqTopOfScreen
        sta $ffff
        lda #1
        sta $d01a
        lda #.theLine
        sta $d012
        lda #$1b
        sta $d011

        lda #2
        sta $d020

        ; Ack any interrupts that might have happened from the CIAs
        lda $dc0d
        lda $dd0d
        ; Ack any interrupts that have happened from the VIC2
        lda #$ff
        sta $d019

        cli

;-------------------------------------------------------------------------------
        ldx #0
.lp1
        lda #$20
        sta $0400,x
        sta $0500,x
        sta $0600,x
        sta $0700,x
        lda #15
        sta $d800,x
        sta $d900,x
        sta $da00,x
        sta $db00,x
        inx
        bne .lp1

        ldx #0
.lp
        txa
        and #$0f
        tay
        lda hextab,y
        sta $0400,x
        sta $0400+(10*40),x
        sta $0400+(15*40),x
        txa
        sta $d800,x
        sta $d800+(10*40),x
        sta $d800+(15*40),x
        lda screendata,x
        sta $0400+(3*40),x
        lda screendata2,x
        sta $0400+(5*40),x
        lda #$01
        sta $d800+(3*40),x
        inx
        cpx #80
        bne .lp
        
.mainLine
        ; Wait for the top IRQ to be triggered
        lda .topIRQDone
        beq .mainLine

        lda #0
        sta .topIRQDone

        lda #%11111101
        sta $dc00
        ldy $dc01

        tya
        and #%00000100
        bne .skA

        ldx .xPosOffset
        dex
        cpx #$00 - 1
        beq .o1
        stx .xPosOffset
.o1
        jmp .skEND
.skA

        tya
        and #%00100000
        bne .skS
        ldx .xPosOffset
        inx
        cpx #126
        beq .o2
        stx .xPosOffset
.o2
        jmp .skEND
.skS

        lda #%01111111
        sta $dc00
        ldy $dc01

        tya
        and #%00000001
        bne .sk1
        ldx .xStartOffset
        dex
        cpx #$00 - 1
        beq .o12
        stx .xStartOffset
.o12
        jmp .skEND
.sk1

        tya
        and #%00001000
        bne .sk2
        ldx .xStartOffset
        inx
        cpx #126
        beq .o22
        stx .xStartOffset
.o22
        jmp .skEND
.sk2

.skEND
        lda #>($0400+(3*40))
        sta scrptr+1
        lda #<($0400+(3*40))
        sta scrptr+0

        lda .xStartOffset
        jsr hexout
        
        lda #>($0400+(4*40))
        sta scrptr+1
        lda #<($0400+(4*40))
        sta scrptr+0

        lda .xPosOffset
        jsr hexout

        lda #%00000000
        sta $dc00
waitk:
        ldy $dc01
        cpy #$ff
        bne waitk
        
        jmp .mainLine

.topIRQDone !by 0

.xStartOffset !by DEFXSTART
.xPosOffset   !by DEFXOFFS

hexout:
        ldy #0
        pha
        lsr
        lsr
        lsr
        lsr
        tax
        lda hextab,x
        sta (scrptr),y
        iny
        pla
        and #$0f
        tax
        lda hextab,x
        sta (scrptr),y
        clc
        lda scrptr
        adc #3
        sta scrptr
        bcc .sk
        inc scrptr+1
.sk
        rts


hextab:
        !by $30, $31, $32, $33, $34, $35, $36, $37, $38, $39
        !by $01, $02, $03, $04, $05, $06

screendata:
             ;1234567890123456789012345678901234567890
        !scr ".. (1-2) delay before d011 init         "
        !scr ".. (a-s) delay before badline forcing   "

;-------------------------------------------------------------------------------

; Remove all possibility that the timings will change due to previous code
!align 255,0
IrqTopOfScreen
        ; Line $2c
        pha
        txa
        pha
        tya
        pha

        inc .topIRQDone

        lda #<.irq2
        ldx #>.irq2

        sta $fffe
        stx $ffff
        inc $d012
        lda #1
        sta $d019                          ; Ack Raster interupt

        ; Begin the raster stabilisation code
        tsx
        cli
        ; These nops never really finish due to the raster IRQ triggering again
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
.irq2
        ; Line $2d
        txs

        ; Delay for a while
        ldx #8
.l1
        dex
        bne .l1
        bit $ea
        nop
        !if NTSC=1 {
        nop
        }

        ; Final cycle wobble check.
        lda #.theLine+1
        cmp $d012
        beq .start
.start
        ; The raster is now stable

        ; Line $2e -------------------------------------------------------------

        ; adjustable delay, max 2 rasterlines (63*2=126 cycles)
        lda #126 - 1
        sec
        sbc .xStartOffset
        ; divide by 2 to get the number of nops to skip
        lsr
        sta .sm2+1
        ; Force branch always
        clv

        ; Introduce a 1 cycle extra delay depending on the least significant bit of the x offset
        bcc .sm2
.sm2
        bvc *
        ; The above branches somewhere into these nops depending on the x offset position
        !for i, 126 / 2 {
        nop
        }

        ; Line $2e..$30 --------------------------------------------------------
        !if ((VSPSEQ = 1) | (VSPSEQ = 2)) {
        ; DEN = 1 and YSCROLL = 1
        lda #$19
        sta $d011
        }
        !if (VSPSEQ = 3) {
        ; DEN = 0 and YSCROLL = 0
        lda #$08
        sta $d011
        }

        ; Calculate a variable offset to delay by branching over nops
        lda #126 - 1
        sec
        sbc .xPosOffset
        ; divide by 2 to get the number of nops to skip
        lsr
        sta .sm1+1
        ; Force branch always
        clv

        ; Introduce a 1 cycle extra delay depending on the least significant bit of the x offset
        bcc .sm1
.sm1
        bvc *
        ; The above branches somewhere into these nops depending on the x offset position
        !for i, 126 / 2 {
        nop
        }

        ; Show the raster position is stable and varies as a function of x offset
        inc DEBUGCOLOR0
        dec DEBUGCOLOR0

        ; Do the HSP by tweaking the $d011 register at the correct time
        !if (VSPSEQ = 1) {
        ; in this sequence DEN is always 1
        lda #$1b
        dec $d011       ; $19 -> $18    %00011000 (badline condition)
        inc $d011       ; $18 -> $19    %00011001
        sta $d011       ; $1b           %00011011
        }
        !if (VSPSEQ = 2) {
        ; in this sequence DEN is always 1
        lda #$1b
        ldx #$10
        ldy #$11
        stx $d011       ; $11 -> $10    %00010000 (badline condition)
        sty $d011       ; $10 -> $11    %00010001
        sta $d011       ; $1b           %00011011
        }

        !if (VSPSEQ = 3) {
        ; set DEN to 1
        ldx #$1b
        ldy #$18
        sty $d011       ; $00 -> $10    %00011000 (badline condition)
        stx $d011       ; $1b           %00011011
        }
        
        ; some color bars. since the above actually forces a badline condition,
        ; the timing is stable here again!
        !for ii, 15 {
        !for i, 7 {
        sty DEBUGCOLOR1
        stx DEBUGCOLOR1
        }
        nop
        nop
        bit $ea
        !if NTSC=1 {
        nop
        }
        }

        ; Restart the IRQ chain
        lda #<IrqTopOfScreen
        ldx #>IrqTopOfScreen
        ldy #.theLine
        sta $fffe
        stx $ffff
        sty $d012
        lda #1
        sta $d019                          ; Ack Raster interupt
        
        dec framecount
        bne +
        lda #0
        sta $d7ff
+
        ; Exit the IRQ
        pla
        tay
        pla
        tax
        pla
.initIRQ
.initNMI
        rti 

framecount: !byte 5
