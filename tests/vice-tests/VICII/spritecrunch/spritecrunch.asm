.var spr1X		= $d000
.var spr1Y		= $d001
.var spr2X		= $d002
.var spr2Y		= $d003
.var spr3X		= $d004
.var spr3Y		= $d005
.var spr4X		= $d006
.var spr4Y		= $d007
.var spr5X		= $d008
.var spr5Y		= $d009
.var spr6X		= $d00a
.var spr6Y		= $d00b
.var spr7X		= $d00c
.var spr7Y		= $d00d
.var spr8X		= $d00e
.var spr8Y		= $d00f

.var code		= $0810

.var startLine		= $38 + 38 - 8

.var scrptr = $02

//.var DEBUG = $01
//.var DELAY = $3c
//.var OFFSET = $00
.var DEBUG = [ cmdLineVars.get ("debug") .asNumber () ]
.var DELAY = [ cmdLineVars.get ("delay") .asNumber () ]
.var OFFSET = [ cmdLineVars.get ("offset") .asNumber () ]

.var DBGCOLOR0 = cmdLineVars.get("debug") == "0" ? $dbff : $d020
.var DBGCOLOR1 = cmdLineVars.get("debug") == "0" ? $dbff : $d021

.pseudocommand nop x {
	:ensureImmediateArgument(x)
	.for (var i=0; i<x.getValue(); i++) nop
}

.pseudocommand pause cycles
{
	:ensureImmediateArgument(cycles)
	.var x = floor(cycles.getValue())
	.if (x<2) .error "Cant make a pause on " + x + " cycles"

	// Take care of odd cyclecount	
	.if ([x&1]==1) {
		bit $00
		.eval x=x-3
	}	
	
	// Take care of the rest
	.if (x>0)
		:nop #x/2
}

.macro ensureImmediateArgument(arg) {
	.if (arg.getType()!=AT_IMMEDIATE)	.error "The argument must be immediate!" 
}

//=======================================================================================

:BasicUpstart2(run)

.pc = code "code"

run:
        sei

        lda #$34
        sta $01		// copy to the last video bank

        lda #$35
        sta $01

        jsr setup

        lda #$7f
        sta $dc0d
        sta $dd0d

        bit $dc0d

        lda #$1b
        sta $d011

        lda #$55
        sta $3fff

        jsr vblank

        lda #62		// set timer interval
        sta $dc04
        lda #$00
        sta $dc05
        lda #$01
        cmp $d012
        bne *-3

        jsr delay_0
        bne *+5
        cmp $00
        nop
        jsr delay_1
        bne *+4
        cmp $00
        jsr delay_1-1
        beq *+2

        lda #$11		// run timer exactly here
        sta $dc0e
        lda #<First_IRQ
        sta $fffe
        lda #>First_IRQ
        sta $ffff
        lda #1
        sta $d01a
        sta $d019

        cli

lp:
        jsr chkkeys
        jmp lp

//---------------------------------------------

delay_0:
        ldx #$a2		// delay on Timer A CIA
        ldx #$a2
        ldx #$a6
delay_1:
        nop
        ldx #$07
        dex
        bne *-1
        lda $d012
        cmp $d012
        rts

//---------------------------------------------

        .align $0100

Stable_IRQ:      
        pha
        txa
        pha
        tya
        pha
        sec
        lda #$32 + 1
        sbc $dc04
        jsr adjdelay

        // stable raster here... cycle 46

switch:		
        // start line
        inc DBGCOLOR0

        // adjustable delay, max 2 rasterlines (63*2=126 cycles)
        lda #126 - 1
        sec
        sbc xStartOffset
        jsr adjdelay

        // perform the sprite crunch trick
        dec DBGCOLOR1

        // the trick happens exactly here

        lda #$00
        sta $d017

delay2:
        lda #$3c
        jsr adjdelay

        lda #$ff
        sta $d017

        inc DBGCOLOR1

        //-------------------------------------------

        inc topIRQDone

        lda #126 - 1
        sec
        sbc xStopOffset
        sta delay2 + 1

        // reset the raster
        lda #0
        sta $d012

        lda #<First_IRQ
        sta $fffe
        lda #>First_IRQ
        sta $ffff

        dec DBGCOLOR0

        dec framecounter
        bne skp
        lda #0
        sta $d7ff
skp:

        inc $d019

        pla
        tay
        pla
        tax
        pla
        rti
        
framecounter: .byte 5

First_IRQ:
        pha
        txa
        pha
        tya
        pha
        
        // here we are in raster $00
        sec
        lda #$32 + 1
        sbc $dc04
        jsr adjdelay

        inc DBGCOLOR0

        // first line of the text screen
        //lda #50
        lda #startLine
        sta spr1Y
        sta spr2Y
        sta spr3Y
        sta spr4Y
        sta spr5Y
        sta spr6Y
        sta spr7Y
        sta spr8Y

        dec DBGCOLOR0
        
        lda #startLine - 1
        cmp $d012
        bne * - 3
        jmp switch

        // set the next raster irq to the line before the start line
        lda #startLine - 1
        sta $d012
        lda #<Stable_IRQ
        sta $fffe
        lda #>Stable_IRQ
        sta $ffff

        dec DBGCOLOR0

        inc $d019

        pla
        tay
        pla
        tax
        pla
        rti

// ---------------------------------------------------------

setup:		
        lda #$00
        sta $d012

        lda #$09
        sta $d020
        lda #$00
        sta $d021

        // set sprites

        lda #40
        sta spr1X
        clc
        adc #24
        sta spr2X
        adc #24
        sta spr3X
        adc #24
        sta spr4X
        adc #24
        sta spr5X
        adc #24
        sta spr6X
        adc #24
        sta spr7X
        adc #24
        sta spr8X

        lda #startLine
        sta spr1Y
        sta spr2Y
        sta spr3Y
        sta spr4Y
        sta spr5Y
        sta spr6Y
        sta spr7Y
        sta spr8Y

        // clear screen
        ldx #0
!:
        lda #32
        sta $0400,x
        sta $0500,x
        sta $0600,x
        sta $0700,x
        lda #1
        sta $d800,x
        sta $d900,x
        sta $da00,x
        sta $db00,x
        inx
        bne !-

        ldx #79
!:      
        lda screendata,x
        sta $0428,x
        dex
        bne !-

        lda #sprite/64
        sta $07f8
        sta $07f9
        sta $07fa
        sta $07fb
        sta $07fc
        sta $07fd
        sta $07fe
        sta $07ff

        lda #%11111111
        sta $d015
        lda #0
        sta $d01b

        rts

vblank:		
        lda $d011
        bpl *-3
        lda $d011
        bmi *-3
        rts


.align $100
	
adjdelay:
        // divide by 2 to get the number of nops to skip
        lsr
        sta sm2+1
        // Force branch always
        clv

        // Introduce a 1 cycle extra delay depending on the least significant bit of the x offset
        bcc sm2
sm2:    bvc *

        // The above branches somewhere into these nops depending on the x offset position
        .for (var i=0; i< [128 / 2]; i++) {
        nop
        }
        rts

topIRQDone: .by 0
xStartOffset: .by DELAY
xStopOffset: .by OFFSET

chkkeys:
        // Wait for the top IRQ to be triggered
        lda topIRQDone
        cmp #3
        bne chkkeys

        lda #0
        sta topIRQDone

        lda #%11111101
        sta $dc00
        ldy $dc01

        tya
        and #%00000100
        bne skA

        ldx xStopOffset
        dex
        cpx #$00 - 1
        beq o1
        stx xStopOffset
o1:
        jmp skEND
skA:

        tya
        and #%00100000
        bne skS
        ldx xStopOffset
        inx
        cpx #126
        beq o2
        stx xStopOffset
o2:
        jmp skEND
skS:

        lda #%01111111
        sta $dc00
        ldy $dc01

        tya
        and #%00000001
        bne sk1
        ldx xStartOffset
        dex
        cpx #$00 - 1
        beq o12
        stx xStartOffset
o12:
        jmp skEND
sk1:

        tya
        and #%00001000
        bne sk2
        ldx xStartOffset
        inx
        cpx #126
        beq o22
        stx xStartOffset
o22:
        jmp skEND
sk2:

skEND:
        lda #>[$0428]
        sta scrptr+1
        lda #<[$0428]
        sta scrptr+0

        lda xStartOffset
        jsr hexout

        lda #>[$0450]
        sta scrptr+1
        lda #<[$0450]
        sta scrptr+0

        lda xStopOffset
        jsr hexout

        rts

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
        bcc sk
        inc scrptr+1
sk:
        rts


hextab:
        .by $30, $31, $32, $33, $34, $35, $36, $37, $38, $39
        .by $01, $02, $03, $04, $05, $06

screendata:
             //1234567890123456789012345678901234567890
        .text ".. (1-2)                                "
        .text ".. (a-s)                                "

        .pc = $2000	"sprite"

sprite:	

        .for (var i=0; i<$40; i++) {
            .by i
        }

