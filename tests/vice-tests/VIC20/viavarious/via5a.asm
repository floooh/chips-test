        !to "via5a.prg", cbm

TESTID =          5

tmp=$fc
addr=$fd
add2=$f9

ERRBUF = $1f00
TMP    = $2000          ; measured data on C64 side
DATA   = $3000          ; reference data

TESTLEN =         $20

NUMTESTS =        18 - 12

TESTSLOC = $1800

DTMP=screenmem

        !src "common.asm"


        * = TESTSLOC


;------------------------------------------
; before: --
; in the loop:
;       [Timer B lo | Timer B hi] = loop counter
;       start Timer B (set to count CLK)
;       read [Timer B lo | Timer B hi | IRQ Flags]

	!zone {		; M
.test 	ldx #0
.l1	stx viabase+$8       ; Timer B lo
	;lda #$11
	;sta $dc0f       ; start timer B continuous, force reload
        lda #%00000000
        sta viabase+$b
	lda viabase+$8       ; Timer B lo
	sta DTMP,x
	inx
	bne .l1
	rts
        * = .test+TESTLEN
        }

	!zone {		; N
.test 	ldx #0
.l1	stx viabase+$8       ; Timer B lo
	;lda #$11
	;sta $dc0f       ; start timer B continuous, force reload
        lda #%00000000
        sta viabase+$b
	lda viabase+$9       ; Timer B hi
	sta DTMP,x
	inx
	bne .l1
	rts
        * = .test+TESTLEN
        }

	!zone {		; O
.test 	ldx #0
.l1	stx viabase+$9       ; Timer B hi
	;lda #$11
	;sta $dc0f       ; start timer B continuous, force reload
        lda #%00000000
        sta viabase+$b
	lda viabase+$8       ; Timer B lo
	sta DTMP,x
	inx
	bne .l1
	rts
        * = .test+TESTLEN
        }

	!zone {		; P
.test 	ldx #0
.l1	stx viabase+$9       ; Timer B hi
	;lda #$11
	;sta $dc0f       ; start timer B continuous, force reload
        lda #%00000000
        sta viabase+$b
	lda viabase+$9       ; Timer B hi
	sta DTMP,x
	inx
	bne .l1
	rts
        * = .test+TESTLEN
        }

	!zone {		; Q
.test 	ldx #0
.l1	stx viabase+$8       ; Timer B lo
	;lda #$11
	;sta $dc0f       ; start timer B continuous, force reload
        lda #%00000000
        sta viabase+$b
	lda viabase+$d       ; IRQ Flags / ACK
	sta DTMP,x
	inx
	bne .l1
	rts
        * = .test+TESTLEN
        }

	!zone {		; R
.test 	ldx #0
.l1	stx viabase+$9       ; Timer B hi
	;lda #$11
	;sta $dc0f       ; start timer B continuous, force reload
        lda #%00000000
        sta viabase+$b
	lda viabase+$d       ; IRQ Flags / ACK
	sta DTMP,x
	inx
	bne .l1
	rts
        * = .test+TESTLEN
        }

        * = DATA
        !bin "via5aref.bin", NUMTESTS * $0100, 2
