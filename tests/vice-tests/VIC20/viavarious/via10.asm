        !to "via10.prg", cbm

TESTID =          10

tmp=$fc
addr=$fd
add2=$f9

ERRBUF = $1f00
TMP    = $2000          ; measured data on C64 side
DATA   = $3000          ; reference data

TESTLEN = $40

NUMTESTS =        16 - 8

TESTSLOC = $1800

DTMP=screenmem

        !src "common.asm"


        * = TESTSLOC

;------------------------------------------
; - output timer A at PB7 and read back PB

!macro  TEST .DDRB,.PRB,.CR,.TIMER,.THIFL {
.test
        lda #.DDRB
        sta viabase+$2                       ; port B ddr input
        lda #.PRB
        sta viabase+$0                       ; port B data
        lda #1
        sta viabase+$4+(.TIMER*4)+.THIFL
        lda #.CR                        ; control reg
        sta viabase+$b+.TIMER
        ldx #0
.t1b    lda viabase+$0                       ; port B data
        sta DTMP,x
        inx
        bne .t1b
        rts
        * = .test+TESTLEN
}

; timer A force-load, start, no output at PB7 (pulse)
+TEST $00,$00,$00,0,0 ; a
+TEST $00,$00,$00,0,1 ; b

; timer A force-load, start, output at PB7 (pulse)
+TEST $00,$00,$80,0,0 ; c
+TEST $00,$00,$80,0,1 ; d

; timer A force-load, start, no output at PB7 (toggle)
+TEST $00,$00,$40,0,0 ; e
+TEST $00,$00,$40,0,1 ; f

; timer A force-load, start, output at PB7 (toggle)
+TEST $00,$00,$c0,0,0 ; g
+TEST $00,$00,$c0,0,1 ; h

        * = DATA
        !bin "via10ref.bin", NUMTESTS * $0100, 2
