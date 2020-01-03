        !to "via9.prg", cbm

TESTID =          9

tmp=$fc
addr=$fd
add2=$f9

ERRBUF = $1f00
TMP    = $2000          ; measured data on C64 side
DATA   = $3000          ; reference data

TESTLEN =         $20

NUMTESTS =        12

TESTSLOC = $1800

DTMP=screenmem

        !src "common.asm"


        * = TESTSLOC

;------------------------------------------
; before:
;       [Timer B lo | Timer B hi] = 1
;       Timer B CTRL = [timer B counts clock | PB6]
; in the loop:
;       read [Timer B lo | Timer B hi | IRQ Flags]
;       toggle toggle timer B counts PB6/Clock

	!zone {		; A
.test 	lda #1
	sta viabase+$8       ; Timer B lo
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
        lda #%00000000
        sta viabase+$b       ; Continuous
	ldx #0
.t1b	lda viabase+$8       ; Timer B lo
	sta DTMP,x
        ;lda $dc0e
        ;eor #$20
        ;sta $dc0e       ; toggle timer A counts CNT/Clock
        lda viabase+$b
        eor #$20
        sta viabase+$b       ; toggle timer B counts PB6/Clock
	inx
	bne .t1b
	rts
        * = .test+TESTLEN
        }

	!zone {		; B
.test 	lda #1
	sta viabase+$8       ; Timer B lo
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
        lda #%00000000
        sta viabase+$b       ; Continuous
	ldx #0
.t1b	lda viabase+$9       ; Timer B hi
	sta DTMP,x
	;lda $dc0e
	;eor #$20
	;sta $dc0e       ; toggle timer A counts CNT/Clock
        lda viabase+$b
        eor #$20
        sta viabase+$b       ; toggle timer B counts PB6/Clock
	inx
	bne .t1b
	rts
        * = .test+TESTLEN
        }

	!zone {		; C
.test 	lda #1
	sta viabase+$8       ; Timer B lo
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
        lda #%00000000
        sta viabase+$b       ; Continuous
	ldx #0
.t1b	lda viabase+$d       ; IRQ Flags / ACK
	sta DTMP,x
	;lda $dc0e
	;eor #$20
	;sta $dc0e       ; toggle timer A counts CNT/Clock
        lda viabase+$b
        eor #$20
        sta viabase+$b       ; toggle timer B counts PB6/Clock
	inx
	bne .t1b
	rts
        * = .test+TESTLEN
        }
        
        ;---------------------------------------

	!zone {		; D
.test 	lda #1
	sta viabase+$9       ; Timer B hi
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
        lda #%00000000
        sta viabase+$b       ; Continuous
	ldx #0
.t1b	lda viabase+$8       ; Timer B lo
	sta DTMP,x
	;lda $dc0e
	;eor #$20
	;sta $dc0e       ; toggle timer A counts CNT/Clock
        lda viabase+$b
        eor #$20
        sta viabase+$b       ; toggle timer B counts PB6/Clock
	inx
	bne .t1b
	rts
        * = .test+TESTLEN
        }

	!zone {		; E
.test 	lda #1
	sta viabase+$9       ; Timer B hi
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
        lda #%00000000
        sta viabase+$b       ; Continuous
	ldx #0
.t1b	lda viabase+$9       ; Timer B hi
	sta DTMP,x
	;lda $dc0e
	;eor #$20
	;sta $dc0e       ; toggle timer A counts CNT/Clock
        lda viabase+$b
        eor #$20
        sta viabase+$b       ; toggle timer B counts PB6/Clock
	inx
	bne .t1b
	rts
        * = .test+TESTLEN
        }

	!zone {		; F
.test 	lda #1
	sta viabase+$9       ; Timer B hi
	;lda #$11
	;sta $dc0e       ; start timer A continuous, force reload
        lda #%00000000
        sta viabase+$b       ; Continuous
	ldx #0
.t1b	lda viabase+$d       ; IRQ Flags / ACK
	sta DTMP,x
	;lda $dc0e
	;eor #$20
	;sta $dc0e       ; toggle timer A counts CNT/Clock
        lda viabase+$b
        eor #$20
        sta viabase+$b       ; toggle timer B counts PB6/Clock
	inx
	bne .t1b
	rts
        * = .test+TESTLEN
        }


        !zone {         ; G
.test   lda #1
        sta viabase+$8       ; Timer B lo
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%00100000
        sta viabase+$b       ; Count Pulses on PB6
        ldx #0
.t1b    lda viabase+$8       ; Timer B lo
        sta DTMP,x
        ;lda $dc0e
        ;eor #$20
        ;sta $dc0e       ; toggle timer A counts CNT/Clock
        lda viabase+$b
        eor #$20
        sta viabase+$b       ; toggle timer B counts PB6/Clock
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; H
.test   lda #1
        sta viabase+$8       ; Timer B lo
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%00100000
        sta viabase+$b       ; Count Pulses on PB6
        ldx #0
.t1b    lda viabase+$9       ; Timer B hi
        sta DTMP,x
        ;lda $dc0e
        ;eor #$20
        ;sta $dc0e       ; toggle timer A counts CNT/Clock
        lda viabase+$b
        eor #$20
        sta viabase+$b       ; toggle timer B counts PB6/Clock
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; I
.test   lda #1
        sta viabase+$8       ; Timer B lo
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%00100000
        sta viabase+$b       ; Count Pulses on PB6
        ldx #0
.t1b    lda viabase+$d       ; IRQ Flags / ACK
        sta DTMP,x
        ;lda $dc0e
        ;eor #$20
        ;sta $dc0e       ; toggle timer A counts CNT/Clock
        lda viabase+$b
        eor #$20
        sta viabase+$b       ; toggle timer B counts PB6/Clock
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }
        
        ;---------------------------------------

        !zone {         ; J
.test   lda #1
        sta viabase+$9       ; Timer B hi
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%00100000
        sta viabase+$b       ; Count Pulses on PB6
        ldx #0
.t1b    lda viabase+$8       ; Timer B lo
        sta DTMP,x
        ;lda $dc0e
        ;eor #$20
        ;sta $dc0e       ; toggle timer A counts CNT/Clock
        lda viabase+$b
        eor #$20
        sta viabase+$b       ; toggle timer B counts PB6/Clock
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; K
.test   lda #1
        sta viabase+$9       ; Timer B hi
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%00100000
        sta viabase+$b       ; Count Pulses on PB6
        ldx #0
.t1b    lda viabase+$9       ; Timer B hi
        sta DTMP,x
        ;lda $dc0e
        ;eor #$20
        ;sta $dc0e       ; toggle timer A counts CNT/Clock
        lda viabase+$b
        eor #$20
        sta viabase+$b       ; toggle timer B counts PB6/Clock
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; L
.test   lda #1
        sta viabase+$9       ; Timer B hi
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%00100000
        sta viabase+$b       ; Count Pulses on PB6
        ldx #0
.t1b    lda viabase+$d       ; IRQ Flags / ACK
        sta DTMP,x
        ;lda $dc0e
        ;eor #$20
        ;sta $dc0e       ; toggle timer A counts CNT/Clock
        lda viabase+$b
        eor #$20
        sta viabase+$b       ; toggle timer B counts PB6/Clock
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }
        
        
        * = DATA
        !bin "via9ref.bin", NUMTESTS * $0100, 2
