        !to "via1.prg", cbm

TESTID = 1

tmp=$fc
addr=$fd
add2=$f9

ERRBUF = $1f00
TMP    = $2000          ; measured data on C64 side
DATA   = $3000          ; reference data

TESTLEN = $20

NUMTESTS = 14 - 6

TESTSLOC = $1800

DTMP=screenmem

        !src "common.asm"

        * = TESTSLOC

;-------------------------------------------------------------------------------
; before:
;       ---
; in the loop:
;       read [Timer A lo | Timer A hi | Timer A lo latch | Timer A hi latch | Timer B lo | Timer B hi]

        !zone {         ; A
.test    ldx #0
.t1b     lda viabase+$4       ; Timer A lo
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; B
.test    ldx #0
.t1b     lda viabase+$5       ; Timer A hi
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; C
.test    ldx #0
.t1b     lda viabase+$6       ; Timer A latch lo
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; D
.test    ldx #0
.t1b     lda viabase+$7       ; Timer A latch hi
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; E
.test    ldx #0
.t1b     lda viabase+$8       ; Timer B lo
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; F
.test    ldx #0
.t1b     lda viabase+$9       ; Timer B hi
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

;-------------------------------------------------------------------------------
; before:
;       start Timer B (switch to count clock cycles)
; in the loop:
;       read [Timer B lo | Timer B hi]

        !zone {         ; G
.test   ;lda #$1
        ;sta $dc0f       ; start timer B continuous
        lda #%00000000
        sta viabase+$b

        ldx #0
.t1b     lda viabase+$8       ; Timer B lo
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

        !zone {         ; H
.test   ;lda #$1
        ;sta $dc0f       ; start timer B continuous
        lda #%00000000
        sta viabase+$b

        ldx #0
.t1b     lda viabase+$9       ; Timer B hi
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
        }

;-------------------------------------------------------------------------------

        * = DATA
        !bin "via1ref.bin", NUMTESTS * $0100, 2
 
