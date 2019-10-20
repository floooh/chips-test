
; little nmi/irq test - demonstrates the nmi bug in vice
; (w) groepaz/hitmen 12/07/2008
;
; program does the following:
;
; - set up a timer irq which shows colorbars using $d021
; - set up a timer nmi which shows colorbars using $d020
; - increments $0400 when restore is pressed
; - increments $0428 in main loop
;
; to see the bug, run program and press restore. the nmi will
; immediatly stop working in vice, no more timer nmis will be
; triggered and repeatedly pressing restore does nothing.

	!initmem $00
	!cpu 6502
	!to "nmitest.prg", cbm

; basic stup		
	*= $0801

!byte $0c,$08,$0b,$00,$9e
!byte $32,$33,$30,$34
!byte $00,$00,$00,$00

; irq and nmi setup (relies on some stuff set up by the system after reset)
start

	*=$0900

	sei

	; cia 1 timer A for IRQ
	lda #>((8*63)-1)
	sta $dc05
	lda #<((8*63)-1)
	sta $dc04
			
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

	; will be executed when timer irq triggers
			
	inc $d021
		
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
		
	jmp nmiend

	; will be executed when timer nmi triggers
norestore:

	inc $d020

nmiend:

nmia	lda #0
	rti

