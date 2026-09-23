;ZP-adresses
irqA = 2
irqX = 3
irqY = 4

;Variables
IrqLijn0 = $2f
IrqLijn1 = 255


*=$0801
; 10 SYS2064 
	!by    $0B, $08, $0A, $00, $9E, $32, $30, $36, $34, $00, $00, $00

*=$0810
;set up stable raster interrupt and put something on screen
        sei
		lda #%00100000
		sta $01
		ldx #0
.clm1	stx ClearMemStart
	inx
	inc .clm1+1
	bne .clm1
	inc .clm1+2
	bne .clm1
	
	lda #%00110111
	sta $01

	; Clear everything else :)
	ldx #0
.clm2
	txa
	sta $0002,x
	sta $0102,x
	sta $0202,x
	sta $0302,x
	sta $0700,x
	sta $db00,x
	dex
	bne .clm2

        cld
        lda #0
        sta $d015
        
        ldy #00
l0:     ldx #39
        tya
m0:     sta $0400,x
m1:	sta $d800,x
        dex
        bpl m0
        clc
        lda m0+1
        adc #40
        sta m0+1
        bcc *+5
        inc m0+2

	clc
	lda m1+1
	adc #40
	sta m1+1
	bcc *+5
	inc m1+2

        iny
        cpy #25
        bcc l0

        lda #100
        sta $0404
        lda #1
        sta $d804

        lda #$7f
        sta $042b
        lda #3
        sta $d82b

        lda #86
        sta $042f
        lda #7
        sta $d82f

        ldx #7
        stx $dbeb
        ldx #5
        stx $dbec 
        ldx #13
        stx $dbed

        jsr setTimer  ;stabilize raster

        lda #$7f   
        sta $dc0d  
        sta $dd0d  
        lda $dc0d  
        lda $dd0d  

        lda #$01  
        sta $d01a
        
        lda #IrqLijn0  
        sta $d012 
       
        lda #$1f     
        sta $d011 
        lda #<irq0
        sta $fffe
        lda #>irq0
        sta $ffff 
    
        lda #$35
        sta $01
        lda #$55
        sta $3fff

        lda #$01
        sta $d010

        inc $d019
        cli
        jmp *

;Raster irq every 4th line
irq0:
        sta irqA
        nop
        lda $dc06     
        eor #7
        sta *+4
        bpl *+2         ;Note: make sure there is no page boundary crossing!
        lda #$a9
        lda #$a9
        lda $eaa5   
        
        
        stx irqX
        sty irqY
        ldx $d012

        inc $0200 ;for delay purposes only

        inc $d020 ;show current line in right border
        dec $d020
        
        inc $0200
;badline on next rasterline
        
        inx
        txa
        and #$07
        ora #$18		;$38
        sta $d011
        txa
        clc
        adc #$03		;irq every 4th line
        cmp #$f8
        bcs endscrn
        sta $d012
        inc $d019
        ldy irqY
        ldx irqX
        lda irqA
        rti

endscrn:
;open the border
        lda $d011
        and #$f7
        sta $d011
        
        bit $d011
        bpl *-3

        lda #$1f		;$3f
        sta $d011
        
        dec delay+1
delay:  lda #2
        bne skp
        lda #0
        sta $d7ff
skp:

        lda #IrqLijn0   ;IrqLijn1
        sta $d012
        inc $d019
        ldy irqY
        ldx irqX
        lda irqA
        
        rti

;Stabilize raster
setTimer:


;Via badline detection with timer
        lax $dc04      
        sbx #51
        sta irqA
        cpx $dc04
        bne setTimer    ;Note: make sure there is no page boundary crossings!
;wait till cycle 54 with setting $dc0f
        ldx #8
        dex
        bne *-1         
        bit $ea             
        lda #8
        sta $dc06
        stx $dc07
        lda #$11
        sta $dc0f     ;This instruction should happen on cycle 54
        rts


ClearMemStart

