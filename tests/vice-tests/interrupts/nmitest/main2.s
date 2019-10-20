
; little nmi/irq test - check restore key timing
; (w) groepaz/hitmen

	!initmem $00
	!cpu 6502
	!to "nmitest2.prg", cbm

; basic stup		
	*= $0801

!byte $0c,$08,$0b,$00,$9e
!byte $32,$33,$30,$34
!byte $00,$00,$00,$00

; irq and nmi setup (relies on some stuff set up by the system after reset)
start

	*=$0900

	sei

	lda #$01
	sta $d01a
	lda #$7f
	sta $dc0d
	lda #$1b
	sta $d011
	lda #$40
	sta $d012

	; IRQ soft vector
        LDA #>IRQ
	sta $0315
        LDA #<IRQ
	sta $0314

	; NMI soft vector
        LDA #>NMI
	sta $0319
        LDA #<NMI
	sta $0318

	; cia 2 timer A for NMI
	lda #>((8*63)-1)
	sta $dd05
	lda #<((8*63)-1)
	sta $dd04

        LDA #%10000001    ; enable CIA-2 timer A nmi
        STA $DD0D
		
        LDA #%00010001    ; timer A start
        STA $DD0E

	; ack pending irq/nmi
        lda $DD0D
        lda $Dc0D
	cli

loop:
	inc $0428
		
	jmp loop
			
;----------------------------------------------------------------------------------------------
		
IRQ:
	LDA $DC0D
	inc $d019

	; will be executed when raster irq triggers

	inc $d020
	ldx $d012
	inx
	inx
wit:
	cpx $d012
	bne wit
        dec $d020
	
	PLA
	TAY
	PLA
	TAX
	PLA
	RTI	

;----------------------------------------------------------------------------------------------

NMI:
	sta nmia+1

	lda $dd0d
	bmi norestore

	; will be executed when restore was pressed
	inc $0400

	lda $d012
	sta $d012
	lda $d011
	sta $d011

	jmp nmiend

	; will be executed when timer nmi triggers
norestore:

;	inc $d020

nmiend:

nmia	lda #0
	rti

