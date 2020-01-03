        !to "via3a.prg", cbm

TESTID =          $3a

tmp=$fc
addr=$fd
add2=$f9

ERRBUF = $1f00
TMP    = $2000          ; measured data on C64 side
DATA   = $3000          ; reference data

TESTLEN =         $20

NUMTESTS =        8

TESTSLOC = $1800

DTMP=screenmem

        !src "common.asm"


        * = TESTSLOC


;------------------------------------------
; before:
;       [Timer A latch lo | Timer A latch hi] = 1
;       Timer A CTRL = [Timed Interrupt when Timer 1 is loaded, no PB7 |
;                       Continuous Interrupts, no PB7 |
;                       Timed Interrupt when Timer 1 is loaded, one-shot on PB7 |
;                       Continuous Interrupts, square-wave on PB7]
; in the loop:
;       read IRQ Flags

        !zone {         ; A
.test   lda #1
        sta viabase+$6       ; Timer A lo
        ;lda #$1
        ;sta $dc0e       ; start timer A continuous
        lda #%00000000
        sta viabase+$b       ; Timed Interrupt when Timer 1 is loaded, no PB7

        ldx #0
.t1b    lda viabase+$d       ; IRQ Flags / ACK
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; B
.test   lda #1
        sta viabase+$6       ; Timer A lo
        ;lda #$1
        ;sta $dc0e       ; start timer A continuous
        lda #%01000000
        sta viabase+$b       ; Continuous Interrupts, no PB7

        ldx #0
.t1b    lda viabase+$d       ; IRQ Flags / ACK
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; C
.test   lda #1
        sta viabase+$6       ; Timer A lo
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%10000000
        sta viabase+$b       ; Timed Interrupt when Timer 1 is loaded, one-shot on PB7

        ldx #0
.t1b    lda viabase+$d       ; IRQ Flags / ACK
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; D
.test   lda #1
        sta viabase+$6       ; Timer A lo
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%11000000
        sta viabase+$b       ; Continuous Interrupts, square-wave on PB7

        ldx #0
.t1b    lda viabase+$d       ; IRQ Flags / ACK
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; E
.test   lda #1
        sta viabase+$7       ; Timer A hi
        ;lda #$1
        ;sta $dc0e       ; start timer A continuous
        lda #%00000000
        sta viabase+$b       ; Timed Interrupt when Timer 1 is loaded, no PB7

        ldx #0
.t1b    lda viabase+$d       ; IRQ Flags / ACK
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; F
.test   lda #1
        sta viabase+$7       ; Timer A hi
        ;lda #$1
        ;sta $dc0e       ; start timer A continuous
        lda #%01000000
        sta viabase+$b       ; Continuous Interrupts, no PB7

        ldx #0
.t1b    lda viabase+$d       ; IRQ Flags / ACK
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; G
.test   lda #1
        sta viabase+$7       ; Timer A hi
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%10000000
        sta viabase+$b       ; Timed Interrupt when Timer 1 is loaded, one-shot on PB7

        ldx #0
.t1b    lda viabase+$d       ; IRQ Flags / ACK
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; H
.test   lda #1
        sta viabase+$7       ; Timer A hi
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        lda #%11000000
        sta viabase+$b       ; Continuous Interrupts, square-wave on PB7

        ldx #0
.t1b    lda viabase+$d       ; IRQ Flags / ACK
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        * = DATA
        !bin "via3aref.bin", NUMTESTS * $0100, 2
        