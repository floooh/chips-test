        !to "via5.prg", cbm

TESTID =          5

tmp=$fc
addr=$fd
add2=$f9

ERRBUF = $1f00
TMP    = $2000          ; measured data on C64 side
DATA   = $3000          ; reference data

TESTLEN =         $20

NUMTESTS =        18 - 6

TESTSLOC = $1800

DTMP=screenmem

        !src "common.asm"


        * = TESTSLOC

;------------------------------------------
; before: --
; in the loop:
;       [Timer A lo | Timer A hi | Timer A latch lo | Timer A latch hi] = loop counter
;       read [Timer A lo | Timer A hi | IRQ Flags]

	!zone {		; A
.test 	ldx #0
.l1	stx viabase+$4       ; Timer A lo
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
	lda viabase+$4       ; Timer A lo
	sta DTMP,x
	inx
	bne .l1
	rts
        * = .test+TESTLEN
        }

	!zone {		; B
.test 	ldx #0
.l1	stx viabase+$4       ; Timer A lo
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
	lda viabase+$5       ; Timer A hi
	sta DTMP,x
	inx
	bne .l1
	rts
        * = .test+TESTLEN
        }

	!zone {		; C
.test 	ldx #0
.l1	stx viabase+$5       ; Timer A hi
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
	lda viabase+$4       ; Timer A lo
	sta DTMP,x
	inx
	bne .l1
	rts
        * = .test+TESTLEN
        }

	!zone {		; D
.test 	ldx #0
.l1	stx viabase+$5       ; Timer A hi
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
	lda viabase+$5       ; Timer A hi
	sta DTMP,x
	inx
	bne .l1
	rts
        * = .test+TESTLEN
        }

	!zone {		; E
.test 	ldx #0
.l1	stx viabase+$4       ; Timer A lo
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
	lda viabase+$d       ; IRQ Flags / ACK
	sta DTMP,x
	inx
	bne .l1
	rts
        * = .test+TESTLEN
        }

	!zone {		; F
.test 	ldx #0
.l1	stx viabase+$5       ; Timer A hi
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
	lda viabase+$d       ; IRQ Flags / ACK
	sta DTMP,x
	inx
	bne .l1
	rts
        * = .test+TESTLEN
        }

;----------------------------------------
        !zone {         ; G
.test   ldx #0
.l1     stx viabase+$6       ; Timer A lo
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda viabase+$6       ; Timer A lo
        sta DTMP,x
        inx
        bne .l1
        rts
        * = .test+TESTLEN
        }

        !zone {         ; H
.test   ldx #0
.l1     stx viabase+$6       ; Timer A lo
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda viabase+$7       ; Timer A hi
        sta DTMP,x
        inx
        bne .l1
        rts
        * = .test+TESTLEN
        }

        !zone {         ; I
.test   ldx #0
.l1     stx viabase+$7       ; Timer A hi
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda viabase+$6       ; Timer A lo
        sta DTMP,x
        inx
        bne .l1
        rts
        * = .test+TESTLEN
        }

        !zone {         ; J
.test   ldx #0
.l1     stx viabase+$7       ; Timer A hi
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda viabase+$7       ; Timer A hi
        sta DTMP,x
        inx
        bne .l1
        rts
        * = .test+TESTLEN
        }

        !zone {         ; K
.test   ldx #0
.l1     stx viabase+$6       ; Timer A lo
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda viabase+$d       ; IRQ Flags / ACK
        sta DTMP,x
        inx
        bne .l1
        rts
        * = .test+TESTLEN
        }

        !zone {         ; L
.test   ldx #0
.l1     stx viabase+$7       ; Timer A hi
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda viabase+$d       ; IRQ Flags / ACK
        sta DTMP,x
        inx
        bne .l1
        rts
        * = .test+TESTLEN
        }

        * = DATA
        !bin "via5ref.bin", NUMTESTS * $0100, 2
