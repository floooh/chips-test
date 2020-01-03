        !to "via4.prg", cbm

TESTID =          4

tmp=$fc
addr=$fd
add2=$f9

ERRBUF = $1f00
TMP    = $2000          ; measured data on C64 side
DATA   = $3000          ; reference data

TESTLEN =         $20

NUMTESTS =        24 - 8

TESTSLOC = $1800

DTMP=screenmem

        !src "common.asm"


        * = TESTSLOC

;------------------------------------------
; before:
;       [Timer A lo | Timer A hi] = 1
;       Timer A CTRL = [Timed Interrupt when Timer 1 is loaded, no PB7 |
;                       Continuous Interrupts, no PB7 |
;                       Timed Interrupt when Timer 1 is loaded, one-shot on PB7 |
;                       Continuous Interrupts, square-wave on PB7]
; in the loop:
;       read [Timer A lo | Timer A hi | IRQ Flags]
;       toggle Timer A CTRL = [Timed Interrupt | Continuous Interrupts]

	!zone {		; A
.test 	lda #1
	sta viabase+$4       ; Timer A lo
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
        lda #%00000000
        sta viabase+$b       ; Timed Interrupt when Timer 1 is loaded, no PB7
	ldx #0
.t1b	lda viabase+$4       ; Timer A lo
	sta DTMP,x
	;lda $dc0e
	;eor #$1
	;sta $dc0e       ; toggle timer A start/stop
	lda viabase+$b
	eor #%01000000
        sta viabase+$b
	inx
	bne .t1b
	rts
        * = .test+TESTLEN
        }

	!zone {		; B
.test 	lda #1
	sta viabase+$4       ; Timer A lo
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
        lda #%00000000
        sta viabase+$b       ; Timed Interrupt when Timer 1 is loaded, no PB7
	ldx #0
.t1b	lda viabase+$5       ; Timer A hi
	sta DTMP,x
	;lda $dc0e
	;eor #$1
	;sta $dc0e       ; toggle timer A start/stop
        lda viabase+$b
        eor #%01000000
        sta viabase+$b
	inx
	bne .t1b
	rts
        * = .test+TESTLEN
        }

	!zone {		; C
.test 	lda #1
	sta viabase+$5       ; Timer A hi
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
        lda #%00000000
        sta viabase+$b       ; Timed Interrupt when Timer 1 is loaded, no PB7
	ldx #0
.t1b	lda viabase+$4       ; Timer A lo
	sta DTMP,x
	;lda $dc0e
	;eor #$1
	;sta $dc0e       ; toggle timer A start/stop
        lda viabase+$b
        eor #%01000000
        sta viabase+$b
	inx
	bne .t1b
	rts
        * = .test+TESTLEN
        }

	!zone {		; D
.test 	lda #1
	sta viabase+$5       ; Timer A hi
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
        lda #%00000000
        sta viabase+$b       ; Timed Interrupt when Timer 1 is loaded, no PB7
	ldx #0
.t1b	lda viabase+$5       ; Timer A hi
	sta DTMP,x
	;lda $dc0e
	;eor #$1
	;sta $dc0e       ; toggle timer A start/stop
        lda viabase+$b
        eor #%01000000
        sta viabase+$b
	inx
	bne .t1b
	rts
        * = .test+TESTLEN
        }

	!zone {		; E
.test 	lda #1
	sta viabase+$4       ; Timer A lo
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
        lda #%00000000
        sta viabase+$b       ; Timed Interrupt when Timer 1 is loaded, no PB7
	ldx #0
.t1b	lda viabase+$d       ; IRQ Flags / ACK
	sta DTMP,x
	;lda $dc0e
	;eor #$1
	;sta $dc0e       ; toggle timer A start/stop
        lda viabase+$b
        eor #%01000000
        sta viabase+$b
	inx
	bne .t1b
	rts
        * = .test+TESTLEN
        }

	!zone {		; F
.test 	lda #1
	sta viabase+$5       ; Timer A hi
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
        lda #%00000000
        sta viabase+$b       ; Timed Interrupt when Timer 1 is loaded, no PB7
	ldx #0
.t1b	lda viabase+$d       ; IRQ Flags / ACK
	sta DTMP,x
	;lda $dc0e
	;eor #$1
	;sta $dc0e       ; toggle timer A start/stop
        lda viabase+$b
        eor #%01000000
        sta viabase+$b
	inx
	bne .t1b
	rts
        * = .test+TESTLEN
        }

;------------------------------------------
        !zone {         ; G
.test   lda #1
        sta viabase+$4       ; Timer A lo
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%01000000
        sta viabase+$b       ; Continuous Interrupts, no PB7
        ldx #0
.t1b    lda viabase+$4       ; Timer A lo
        sta DTMP,x
        ;lda $dc0e
        ;eor #$1
        ;sta $dc0e       ; toggle timer A start/stop
        lda viabase+$b
        eor #%01000000
        sta viabase+$b
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; H
.test   lda #1
        sta viabase+$4       ; Timer A lo
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%01000000
        sta viabase+$b       ; Continuous Interrupts, no PB7
        ldx #0
.t1b    lda viabase+$5       ; Timer A hi
        sta DTMP,x
        ;lda $dc0e
        ;eor #$1
        ;sta $dc0e       ; toggle timer A start/stop
        lda viabase+$b
        eor #%01000000
        sta viabase+$b
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; I
.test   lda #1
        sta viabase+$5       ; Timer A hi
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%01000000
        sta viabase+$b       ; Continuous Interrupts, no PB7
        ldx #0
.t1b    lda viabase+$4       ; Timer A lo
        sta DTMP,x
        ;lda $dc0e
        ;eor #$1
        ;sta $dc0e       ; toggle timer A start/stop
        lda viabase+$b
        eor #%01000000
        sta viabase+$b
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; J
.test   lda #1
        sta viabase+$5       ; Timer A hi
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%01000000
        sta viabase+$b       ; Continuous Interrupts, no PB7
        ldx #0
.t1b    lda viabase+$5       ; Timer A hi
        sta DTMP,x
        ;lda $dc0e
        ;eor #$1
        ;sta $dc0e       ; toggle timer A start/stop
        lda viabase+$b
        eor #%01000000
        sta viabase+$b
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; K
.test   lda #1
        sta viabase+$4       ; Timer A lo
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%01000000
        sta viabase+$b       ; Continuous Interrupts, no PB7
        ldx #0
.t1b    lda viabase+$d       ; IRQ Flags / ACK
        sta DTMP,x
        ;lda $dc0e
        ;eor #$1
        ;sta $dc0e       ; toggle timer A start/stop
        lda viabase+$b
        eor #%01000000
        sta viabase+$b
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; L
.test   lda #1
        sta viabase+$5       ; Timer A hi
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%01000000
        sta viabase+$b       ; Continuous Interrupts, no PB7
        ldx #0
.t1b    lda viabase+$d       ; IRQ Flags / ACK
        sta DTMP,x
        ;lda $dc0e
        ;eor #$1
        ;sta $dc0e       ; toggle timer A start/stop
        lda viabase+$b
        eor #%01000000
        sta viabase+$b
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

;------------------------------------------
        !zone {         ; M
.test   lda #1
        sta viabase+$4       ; Timer A lo
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%10000000
        sta viabase+$b       ; Timed Interrupt when Timer 1 is loaded, one-shot on PB7
        ldx #0
.t1b    lda viabase+$4       ; Timer A lo
        sta DTMP,x
        ;lda $dc0e
        ;eor #$1
        ;sta $dc0e       ; toggle timer A start/stop
        lda viabase+$b
        eor #%01000000
        sta viabase+$b
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; N
.test   lda #1
        sta viabase+$4       ; Timer A lo
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%10000000
        sta viabase+$b       ; Timed Interrupt when Timer 1 is loaded, one-shot on PB7
        ldx #0
.t1b    lda viabase+$5       ; Timer A hi
        sta DTMP,x
        ;lda $dc0e
        ;eor #$1
        ;sta $dc0e       ; toggle timer A start/stop
        lda viabase+$b
        eor #%01000000
        sta viabase+$b
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; O
.test   lda #1
        sta viabase+$5       ; Timer A hi
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%10000000
        sta viabase+$b       ; Timed Interrupt when Timer 1 is loaded, one-shot on PB7
        ldx #0
.t1b    lda viabase+$4       ; Timer A lo
        sta DTMP,x
        ;lda $dc0e
        ;eor #$1
        ;sta $dc0e       ; toggle timer A start/stop
        lda viabase+$b
        eor #%01000000
        sta viabase+$b
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; P
.test   lda #1
        sta viabase+$5       ; Timer A hi
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%10000000
        sta viabase+$b       ; Timed Interrupt when Timer 1 is loaded, one-shot on PB7
        ldx #0
.t1b    lda viabase+$5       ; Timer A hi
        sta DTMP,x
        ;lda $dc0e
        ;eor #$1
        ;sta $dc0e       ; toggle timer A start/stop
        lda viabase+$b
        eor #%01000000
        sta viabase+$b
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

;------------------------------------------

        * = DATA
        !bin "via4ref.bin", NUMTESTS * $0100, 2
