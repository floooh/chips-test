        !to "via4a.prg", cbm

TESTID =          4

tmp=$fc
addr=$fd
add2=$f9

ERRBUF = $1f00
TMP    = $2000          ; measured data on C64 side
DATA   = $3000          ; reference data

TESTLEN =         $20

NUMTESTS =        24 - 16

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


        !zone {         ; Q
.test   lda #1
        sta viabase+$4       ; Timer A lo
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%10000000
        sta viabase+$b       ; Timed Interrupt when Timer 1 is loaded, one-shot on PB7
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

        !zone {         ; R
.test   lda #1
        sta viabase+$5       ; Timer A hi
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%10000000
        sta viabase+$b       ; Timed Interrupt when Timer 1 is loaded, one-shot on PB7
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
        !zone {         ; S
.test   lda #1
        sta viabase+$4       ; Timer A lo
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%11000000
        sta viabase+$b       ; Continuous Interrupts, square-wave on PB7
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

        !zone {         ; T
.test   lda #1
        sta viabase+$4       ; Timer A lo
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%11000000
        sta viabase+$b       ; Continuous Interrupts, square-wave on PB7
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

        !zone {         ; U
.test   lda #1
        sta viabase+$5       ; Timer A hi
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%11000000
        sta viabase+$b       ; Continuous Interrupts, square-wave on PB7
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

        !zone {         ; V
.test   lda #1
        sta viabase+$5       ; Timer A hi
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%11000000
        sta viabase+$b       ; Continuous Interrupts, square-wave on PB7
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

        !zone {         ; W
.test   lda #1
        sta viabase+$4       ; Timer A lo
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%11000000
        sta viabase+$b       ; Continuous Interrupts, square-wave on PB7
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

        !zone {         ; X
.test   lda #1
        sta viabase+$5       ; Timer A hi
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%11000000
        sta viabase+$b       ; Continuous Interrupts, square-wave on PB7
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

        * = DATA
        !bin "via4aref.bin", NUMTESTS * $0100, 2
