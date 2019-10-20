
buffer = $2200
buffer2 = $3200

CIA	= $dd00
TIMER	= 0	; timer 0 or timer 1

        *= $0801
        !byte $0c,$08,$0b,$00,$9e
        !byte $34,$30,$39,$36
        !byte $00,$00,$00,$00

        ;-----------------------------------------------------

        *=$1000

	lda #TIMER+1
	sta CIA+13

	lda #0
	sta CIA+14+TIMER

	sei

	lda #0
	sta $d011

	jsr sync

	; test1 - read timer values from register after starting. 
	; should read @ buffer 0a 09 08 07 06 05 04 03 02 01 0c 0c 0b 0a 09 08 07 ....
	lda #12
	sta CIA+4+(TIMER*2)
	lda #0
	sta CIA+5+(TIMER*2)

	lda #16
	sta CIA+14+TIMER	; cont mode, force load, timer stop

	ldx #0
	inc CIA+14+TIMER	; start timer
l0
	lda CIA+4+(TIMER*2)
	sta buffer,x
	inx
	bne l0

	; done

	jsr sync

	; test2 - read values from IRQ register
	; should read @ $1300 00 [ 01 00 01 01 01 01 01 01 ] [ 01 00 01 ....
	lda #15
	sta CIA+4+(TIMER*2)
	lda #0
	sta CIA+5+(TIMER*2)

	lda #16
	sta CIA+14+TIMER	; cont mode, force load, timer stop
		
	ldx #0
	lda CIA+13	; clr irq flag before timer start
	inc CIA+14+TIMER
l1
	lda CIA+13
	sta buffer+$0100,x
	inx
	bne l1

	; done

	; test3 - check IRQ addresses
	; when used with CIA2 TA, and reading of CIA+13, it
	; should read @ $1400 0c 93 10 04 81 10 07 dc 10 0f fa 10 10
	; when used with CIA2 TA, without reading of CIA+13, it
	; should read @ $1400 06 93 10 04 dc 10 0f

	jsr sync

	lda #15
	sta CIA+4+(TIMER*2)
	lda #0
	sta CIA+0+(TIMER*2)

	lda #16			; cont mode, force load, timer stop
	sta CIA+14+TIMER

	lda CIA+13
	lda #129+TIMER
	sta CIA+13		; set timer IRQ

	lda #<IRQ
	sta $0314
	lda #>IRQ
	sta $0315
	
	lda #<NMI
	sta $0318
	lda #>NMI
	sta $0319

	lda #0
	sta buffer+$0200

	inc CIA+14+TIMER	; start timer

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

	jsr sync

	lda #16			; timer latch differs by one to previous
	sta CIA+4+(TIMER*2)
	lda #16			; force load. cont mode
	sta CIA+14+TIMER

	inc CIA+14+TIMER

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

	jsr sync

	lda #15
	sta CIA+4+(TIMER*2)
	lda #0
	sta CIA+0+(TIMER*2)

	lda #24			; one-shot, force load, timer stop
	sta CIA+14+TIMER

	lda CIA+13
	lda #129+TIMER
	sta CIA+13		; set timer IRQ

	inc CIA+14+TIMER	; start timer

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

	jsr sync

	lda #16			; timer latch differs by one to previous
	sta CIA+4+(TIMER*2)
	lda #24			; force load. one-shot
	sta CIA+14+TIMER

	inc CIA+14+TIMER

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

	; done

	lda #$1b
	sta $d011
	jmp ($fffc)	; reset

sync
	lda $d012
	cmp #1
	bne sync
	bit $d011
	bmi sync
	rts

NMI	
	pha
	lda #0
	sta CIA+14+TIMER	; timer stop
	txa
	pha
	tya
	pha
IRQ
	lda #0
	sta CIA+14+TIMER	; timer stop
	tsx
	ldy buffer+$0200
	iny
	lda $105,x		; return address
	sta buffer+$0200,y
	iny
	lda $106,x
	sta buffer+$0200,y
	iny
	lda CIA+4+(TIMER*2)
	sta buffer+$0200,y
	sty buffer+$0200

	; without this read, there should only be one NMI on CIA2, TA
	lda CIA+13

        ; done

        lda #$1b
        sta $d011
        lda #$03
        sta $dd00
;       cli

        ldx #0
lp1:
        lda #$20
        sta $0400,x
        sta $0500,x
        sta $0600,x
        sta $0700,x
        lda #5
        sta $d800,x
        sta $d900,x
        sta $da00,x
        sta $db00,x
        inx
        bne lp1

        lda #5
        sta $d020

        ldx #0
lp:
        lda buffer+$0000,x
        sta $0400,x
        cmp buffer2+$0000,x
        beq sk1
        lda #2
        sta $d800,x
        sta $d020
sk1
        lda buffer+$0100,x
        sta $0500,x
        cmp buffer2+$0100,x
        beq sk2
        lda #2
        sta $d900,x
        sta $d020
sk2
        lda buffer+$0200,x
        sta $0600,x
        cmp buffer2+$0200,x
        beq sk3
        lda #2
        sta $da00,x
        sta $d020
sk3
        inx
        bne lp

    lda $d020
    and #$0f
    ldx #0 ; success
    cmp #5
    beq nofail
    ldx #$ff ; failure
nofail:
    stx $d7ff

        jmp *


