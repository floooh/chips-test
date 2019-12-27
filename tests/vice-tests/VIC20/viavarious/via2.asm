        !to "via2.prg", cbm

TESTID = 2

tmp =$fc
addr=$fd
add2=$f9

ERRBUF = $1f00
TMP    = $2000          ; measured data on C64 side
DATA   = $3000          ; reference data

TESTLEN =        $20

NUMTESTS =       16 - 4

TESTSLOC = $1800

DTMP=screenmem

        !src "common.asm"


        * = TESTSLOC

;------------------------------------------
; before:
;       [Timer A lo | Timer A hi] = 1
; in the loop:
;       read [Timer A lo | Timer A hi | Timer A latch lo | Timer A latch hi]

        !zone {         ; A
.test   lda #1
        sta viabase+$4        ; Timer A lo
        ;lda #$1
        ;sta $dc0e       ; start timer A continuous
        ldx #0
.t1b    lda viabase+$4        ; Timer A lo
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; B
.test   lda #1
        sta viabase+$4        ; Timer A lo
        ;lda #$1
        ;sta $dc0e       ; start timer A continuous
        ldx #0
.t1b    lda viabase+$5        ; Timer A hi
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; C
.test   lda #1
        sta viabase+$4
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        ldx #0
.t1b    lda viabase+$6        ; Timer A latch lo
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; D
.test   lda #1
        sta viabase+$4
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        ldx #0
.t1b    lda viabase+$7        ; Timer A latch hi
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        ;-----------------------------------

        !zone {         ; E
.test   lda #1
        sta viabase+$5        ; Timer A hi
        ;lda #$1
        ;sta $dc0e       ; start timer A continuous
        ldx #0
.t1b    lda viabase+$4
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; F
.test    lda #1
        sta viabase+$5
        ;lda #$1
        ;sta $dc0e       ; start timer A continuous
        ldx #0
.t1b    lda viabase+$5
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; G
.test   lda #1
        sta viabase+$5
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        ldx #0
.t1b    lda viabase+$6
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; H
.test    lda #1
        sta viabase+$5
        ;lda #$11
        ;sta $dc0e       ; start timer A continuous, force reload
        ldx #0
.t1b    lda viabase+$7
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

;------------------------------------------
; before:
;       [Timer B lo | Timer B hi] = 1
;       start Timer B (switch to count clock cycles)
; in the loop:
;       read [Timer B lo | Timer B hi]


        !zone {         ; I
.test   lda #1
        sta viabase+$8        ; Timer B lo
        ;lda #$1
        ;sta $dc0f       ; start timer B continuous
        lda #%00000000
        sta viabase+$b
        ldx #0
.t1b    lda viabase+$8        ; Timer B lo
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; J
.test    lda #1
        sta viabase+$8        ; Timer B lo
        ;lda #$1
        ;sta $dc0f       ; start timer B continuous
        lda #%00000000
        sta viabase+$b
        ldx #0
.t1b     lda viabase+$9
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; K
.test    lda #1
        sta viabase+$9
        ;lda #$1
        ;sta $dc0f       ; start timer B continuous
        lda #%00000000
        sta viabase+$b
        ldx #0
.t1b     lda viabase+$8
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; L
.test    lda #1
        sta viabase+$9
        ;lda #$1
        ;sta $dc0f       ; start timer B continuous
        lda #%00000000
        sta viabase+$b
        ldx #0
.t1b     lda viabase+$9
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        * = DATA
        !bin "via2ref.bin", NUMTESTS * $0100, 2
