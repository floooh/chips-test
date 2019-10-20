buffer = $2200
buffer2 = $3200

CIA	= $dc00
TIMER	= 0	; timer 0 or timer 1

        *= $0801
        !byte $0c,$08,$0b,$00,$9e
        !byte $34,$30,$39,$36
        !byte $00,$00,$00,$00

        ;-----------------------------------------------------

        *=$1000

	lda #%00000010
	sta CIA+13

	lda #0
	sta CIA+15

	sei

	lda #0
	sta $d011
la
	lda $d012
	cmp #1
	bne la

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
lb
	lda $d012
	cmp #1
	bne lb

	; test2 - read values from IRQ register
	; should read @ $1300 00 [ 02 00 00 02 02 02 02 02 ] [ 02 00 00 ....
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

	lda #$1b
	sta $d011
;	cli

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
        lda buffer,x
        sta $0400,x
        cmp buffer2,x
        beq sk
        lda #2
        sta $d800,x
        sta $d020
sk
        lda buffer+$0100,x
        sta $0500,x
        cmp buffer+$0100,x
        beq sk2
        lda #2
        sta $d900,x
        sta $d020
sk2
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

