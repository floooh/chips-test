;
; Small program to demonstrate a bug on the U64
;

temp			= $fb

VIC_Base		= $D000

VIC_Sprite0_X		= VIC_Base + 0
VIC_Sprite0_Y		= VIC_Base + 1
VIC_Sprite0_Color	= VIC_Base + 39
VIC_Sprite1_X		= VIC_Base + 2
VIC_Sprite1_Y		= VIC_Base + 3
VIC_Sprite1_Color	= VIC_Base + 40
VIC_Sprite2_X		= VIC_Base + 4
VIC_Sprite2_Y		= VIC_Base + 5
VIC_Sprite2_Color	= VIC_Base + 41
VIC_Sprite3_X		= VIC_Base + 6
VIC_Sprite3_Y		= VIC_Base + 7
VIC_Sprite3_Color	= VIC_Base + 42
VIC_Sprite4_X		= VIC_Base + 8
VIC_Sprite4_Y		= VIC_Base + 9
VIC_Sprite4_Color	= VIC_Base + 43
VIC_Sprite5_X		= VIC_Base + 10
VIC_Sprite5_Y		= VIC_Base + 11
VIC_Sprite5_Color	= VIC_Base + 44
VIC_Sprite6_X		= VIC_Base + 12
VIC_Sprite6_Y		= VIC_Base + 13
VIC_Sprite6_Color	= VIC_Base + 45
VIC_Sprite7_X		= VIC_Base + 14
VIC_Sprite7_Y		= VIC_Base + 15
VIC_Sprite7_Color	= VIC_Base + 46
VIC_Sprites_XMSB	= VIC_Base + 16
VIC_Sprites_XExp	= VIC_Base + 29
VIC_Sprites_YExp	= VIC_Base + 23
VIC_Sprites_Priority	= VIC_Base + 27
VIC_Sprites_MultiColor	= VIC_Base + 28
VIC_Sprites_Enable	= VIC_Base + 21
VIC_Control1		= VIC_Base + 17
VIC_RasterLine		= VIC_Base + 18
VIC_MemoryPointer	= VIC_Base + 24
VIC_Color_Border	= VIC_Base + 32
VIC_Color_Background0	= VIC_Base + 33
VIC_Interrupt_Enable	= VIC_Base + 26
VIC_Interrupt_Ack	= VIC_Base + 25



		!cpu	6502
		*=	$0801
;		!to	"bug.prg", cbm


Basic_Header:	!byte	$0b,$08				;link to next line
		!byte	$40,$03				;line-number
		!byte	$9e,$32,$30,$36,$31		;"SYS 2061"
		!byte	0				;end of line
		!byte	0,0				;null-link, no more lines following

Entry_Point:	lda	#$0e				;SET COLOR FOR BACKGROUND AND BORDER
		sta	VIC_Color_Border
		lda	#$06
		sta	VIC_Color_Background0

		lda	VIC_MemoryPointer		;SET BITMAP BASE ADRESS TO $2000
		and	#%11110111
		ora	#%00001000
		sta	VIC_MemoryPointer

		lda	VIC_Control1			;SET SCREEN MODE
		and	#%00111000
		ora	#%00111000			;set bitmap mode & display enable
		ora	#%00000111			;shift canvas down one pixel (to see idle lines)
		sta	VIC_Control1

		ldx	#$20				;CLEAR BITMAP
		txa
		sta	temp+1
		lda	#$00
		sta	temp
		tay
;		lda	#$cc
loop1:		sta	(temp),y
		iny
		bne	loop1
		inc	temp+1
		dex
		bne	loop1

		lda	#$f6				;CLEAR SCREEN-RAM (USED AS COLOR INFORMATION)
		ldx	#$00
loop2:		sta	$0400,x
		sta	$0500,x
		sta	$0600,x
		sta	$0700,x
		inx
		bne	loop2

		lda	#$ff				;PLOT SMALL IMAGE ON UPPER LEFT POSITION
		sta	$2000
		lda	#$81
		sta	$2001
		lda	#$81
		sta	$2002
		lda	#$81
		sta	$2003
		lda	#$81
		sta	$2004
		lda	#$81
		sta	$2005
		lda	#$81
		sta	$2006
		lda	#$ff
		sta	$2007

		lda	#$cc				;SET VALUE FOR VIC-DATA-SEQUENCER IN IDLE MODE
		sta	$3FFF

		lda	#$00				;INIT UPPER 4 SPRITES
		sta	VIC_Sprites_XMSB
		sta	VIC_Sprites_XExp
		sta	VIC_Sprites_YExp
		sta	VIC_Sprites_MultiColor
		sta	VIC_Sprites_Priority
		lda	#$0f				;pointer
		sta	$07f8
		sta	$07f9
		sta	$07fa
		sta	$07fb
		sta	$07fc
		sta	$07fd
		sta	$07fe
		sta	$07ff
		ldx	#$3f				;image
		lda	#$ff
loop3:		sta	$03c0,x
		dex
		bpl	loop3
		lda	#50				;position (with value of #50, sprites will be displayed in raster line #51)
		sta	VIC_Sprite0_Y
		sta	VIC_Sprite1_Y
		sta	VIC_Sprite2_Y
		sta	VIC_Sprite3_Y
		sta	VIC_Sprite4_Y
		sta	VIC_Sprite5_Y
		sta	VIC_Sprite6_Y
		sta	VIC_Sprite7_Y
		lda	#40
		sta	VIC_Sprite0_X
		lda	#70
		sta	VIC_Sprite1_X
		lda	#100
		sta	VIC_Sprite2_X
		lda	#130
		sta	VIC_Sprite3_X
		lda	#160
		sta	VIC_Sprite4_X
		lda	#190
		sta	VIC_Sprite5_X
		lda	#220
		sta	VIC_Sprite6_X
		lda	#250
		sta	VIC_Sprite7_X
		lda	#$0e				;color
		sta	VIC_Sprite0_Color
		sta	VIC_Sprite1_Color
		sta	VIC_Sprite2_Color
		sta	VIC_Sprite3_Color
		sta	VIC_Sprite4_Color
		sta	VIC_Sprite5_Color
		sta	VIC_Sprite6_Color
		sta	VIC_Sprite7_Color
		lda	#%00000000			;sprites will be switched on during irq
		sta	VIC_Sprites_Enable

		sei					;SET RASTER INTERRUPT
		lda	#$35				;roms ausblenden
		sta	$01
		lda	#<VIC_Interrupt1
		sta	$fffe
		lda	#>VIC_Interrupt1
		sta	$ffff

		lda	#%00000001
		sta	VIC_Interrupt_Enable
		lda	#48
		sta	VIC_RasterLine

		lda	#%01111111			;timer interrupts off
		sta	$dc0d
		lda	$dc0d

		lda	#%00000001
		sta	VIC_Interrupt_Ack

		cli

EndlessLoop:	jmp	EndlessLoop			;ENDLESS LOOP



VIC_Interrupt1:	tsx					;FIRST IRQ ROUTINE (LINE 48)
		lda	#49
		sta	VIC_RasterLine
		lda	#%00000000
		sta	VIC_Sprites_Enable
		lda	#<VIC_Interrupt2
		sta	$fffe
		lda	#>VIC_Interrupt2
		sta	$ffff
		lda	#%00000001
		sta	VIC_Interrupt_Ack
		cli
		nop					;somewhere here an interrupt occures...
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
		nop
		brk


VIC_Interrupt2:	ldy	#48				;SECOND IRQ ROUTINE (LINE 49)
		sty	VIC_RasterLine
		lda	#<VIC_Interrupt1
		sta	$fffe
		lda	#>VIC_Interrupt1
		sta	$ffff
		lda	#%00000001
		sta	VIC_Interrupt_Ack
		lda	$ff
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
		ldy	#50
		cpy	VIC_RasterLine			;already in line 50?
		beq	Stable_Raster

Stable_Raster:	txs					;stable raster in line 50
		lda	#%11111111
		sta	VIC_Sprites_Enable
		lda	#$0b
		ldy	#$0c
		sta	VIC_Color_Border
		sty	VIC_Color_Border

		ldx	$ff				;wasting line 50
		lda	#$ff
		ldy	#$00
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
		
		
							;now in line 51	(sequencer is still idle)
		sty	VIC_Sprites_YExp
		nop
		nop
		nop
		nop
		nop
		sta	VIC_Sprites_YExp
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		tya
		ldx	#$3b
		stx	VIC_Control1
		
							;now in line 52 (sequencer outputs first bitmap-line)
		sta	VIC_Sprites_YExp,Y
		nop
		nop
		nop
		nop
		nop
		nop
		lda	$ff
		lda	#$ff
		sta	VIC_Sprites_YExp
		nop
		stx	$ff
		nop
		nop
		nop
		nop
		nop
		nop
		ldx	#$00

							;now in line 53
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
		nop
		nop
		nop
		nop
		tya
		ldx	#$3c
		stx	VIC_Control1

							;now in line 54
		lda	$ff
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		ldx	$30
		lda	#$00
		nop
		nop
		nop
		stx	$ff
		nop
		nop
		nop
		nop
		nop
		nop
		ldx	#$00

framedelay=*+1
		lda #2
		beq +
        dec framedelay
		rti

+		
		stx $d7ff
		
		rti
		
