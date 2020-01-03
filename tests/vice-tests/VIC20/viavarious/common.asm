        !convtab pet
        !cpu 6510

expanded=1

;-------------------------------------------------------------------------------

viabase = $9110
;viabase = $9120


charsperline = 22+2
numrows = 23+2

!if (expanded=1) {
basicstart = $1201      ; expanded
screenmem = $1000
colormem = $9400
} else {
basicstart = $1001      ; unexpanded
screenmem = $1e00
colormem = $9600
}

refdataoffs = (charsperline * (($100 / charsperline) + 2))


                * = basicstart
                !word basicstart + $0c
                !byte $0a, $00
                !byte $9e
!if (expanded=1) {
                !byte $34, $36, $32, $32
} else {
                !byte $34, $31, $31, $30
}
                !byte 0,0,0

                * = basicstart + $0d
start:
        jmp start2
                * = basicstart + $0100
;-------------------------------------------------------------------------------
start2:
        jsr screen_enable
        jsr clrscr

        lda #6
        ldx #$ff
-
        dex
        sta ERRBUF,x
        bpl -

        lda #5
        sta ERRBUF+$ff

        !if (1) {
        ; run all tests once
        ldx #0
clp1b
        txa
        sta tmp
        pha
        jsr calcaddr
        jsr displayprepare
        jsr dotest
        pla
        tax
        inx
        cpx #NUMTESTS
        bne clp1b
        }

        ; check if test failed and set border color accordingly
        ldy #5 ; green
        ldx #0
-
        lda ERRBUF,x
        cmp #5 ; green
        beq +
        ldy #2 ; red
+
        inx
        cpx #NUMTESTS
        bne -

        sty $900f

        ; store value to "debug cart"
        lda #0 ; success
        cpy #5 ; green
        beq +
        lda #$ff ; failure
+
        sta $910f
        
loop
;        jsr calcaddr
;        jsr displayresults

        ; wait for keypress
wkey
        cli

        jsr $ffe4
        cmp #$20
        bne +
        jmp $fd22
+
        cmp #"a"
        bcc wkey
        cmp #"a"+NUMTESTS
        bcs wkey

        sei

        tax
        sec
        sbc #"a"
        sta tmp
        txa
        and #$3f
        sta screenmem+((numrows-1)*charsperline)+(charsperline-1)

        lda #$00
        sta viabase+$0
-       lda viabase+$1
        cmp #$ff
        bne -
        lda #$ff
        sta viabase+$0

        jsr calcaddr
        jsr displayprepare
        jsr dotest
;        jsr calcaddr
;        jsr displayresults
        jmp loop

;-------------------------------------------------------------------------------

dotest
        inc $900F

        lda #$20
        ldx #0
clp1    sta screenmem,x
        inx
        bne clp1

        lda #0
        sta addr+1
        lda tmp
        asl
        rol addr+1
        asl
        rol addr+1
        asl
        rol addr+1
        asl
        rol addr+1
        asl
        rol addr+1
!if (TESTLEN >= $40) {
        asl
        rol addr+1
}
        sta addr
        sta jumpaddr+1
        lda #>TESTSLOC        ; testloc hi
        clc
        adc addr+1
        sta addr+1
        sta jumpaddr+2


        sei


        jsr screen_disable

        lda #250 >> 1
        cmp $9004
        bne *-3
        cmp $9004
        beq *-3

;        dex
;        bne -


        inc $900F
        jsr dotest2

        dec $900F

        jsr screen_enable

        jsr calcaddr

        ; copy data from screen to TMP
        ldy #0
-
        lda screenmem,y
        sta (addr),y
        iny
        bne -

;        dec $900F

        jsr displayresults

;        inc $900F

        rts

calcaddr:
        lda tmp
        clc
        adc #>TMP
        sta addr+1
        lda #<TMP
        sta addr

        lda tmp
        clc
        adc #>DATA
        sta add2+1
        lda #<DATA
        sta add2
        rts

displayprepare:
        lda #0
        sta $900f

        ldy #0
-
        lda #$20
        sta screenmem,y
;        sta screenmem+$100,y
        lda (add2),y
        sta screenmem+refdataoffs,y
        iny
        bne -

        ldx tmp
        lda #1
        sta ERRBUF,x
        
        jsr .displaytestid
        jsr .displayerrlog

        rts

.displaytestid:
        ; test id in bottom right corner
        ldx tmp
        txa
        clc
        adc #"a"
        and #$3f
        sta screenmem+((numrows-1)*charsperline)+(charsperline-1)
        rts


.displayerrlog:
        ; show errors
        ldy #0
clp1a
        tya
        clc
        adc #"a"
        and #$3f
        sta screenmem+((numrows-1)*charsperline),y
        lda ERRBUF,y
        sta colormem+((numrows-1)*charsperline),y
        iny
        cpy #NUMTESTS
        bne clp1a
        rts

displayresults:

        jsr .displaytestid

        ldx tmp
        lda #5
        sta ERRBUF,x

        ; check TMP for errors, display screen
        lda #$05        ; green
        ldy #0
lla     sta colormem,y
        sta colormem+refdataoffs,y
        iny
        bne lla

        ldy #0
ll      lda (addr),y
        sta screenmem,y
        ;lda screenmem,y
        ;sta (addr),y
        cmp (add2),y
        beq rot
        lda #$02        ; red
        sta colormem,y
        sta colormem+refdataoffs,y

        sta ERRBUF,x
        sta ERRBUF+$ff
rot
        lda (add2),y
        sta screenmem+refdataoffs,y
        iny
        bne ll

        jsr .displayerrlog

        lda ERRBUF+$ff
;        sta $900F

;        lda #0
;        sta $d021

        rts

;-------------------------------------------------------------------------------

dotest2:
        sei
        jsr .setdefaults

        ldy #$00
-
        iny
        bne -

        ; call actual test
jumpaddr:
        jsr $dead

        sei
        jsr .setdefaults

        ; (more or less) reset VIA regs and drive
;        sei
        rts

.setdefaults:
!if (1) {
        ; serial port: disabled
        ; timer A: count clk, continuous, Timed Interrupt when Timer 1 is loaded, no PB7
        ; timer B: count PB6 pulses (=stop)
        ; port A latching disabled
        ; port B latching disabled
        lda #%00100000
        sta viabase+$b
        ; CB2 Control: Input negative active edge 
        ; CB1 Interrupt Control: Negative active edge
        ; CA2 Control: Input negative active edge 
        ; CA1 Interrupt Control: Negative active edge
        lda #0
        sta viabase+$c

        lda #$a5
        sta viabase+$a ; serial shift register

        ; disable IRQs
        lda #$7f
        sta viabase+$e

        ; DDR output
        lda #$ff
        sta viabase+$2
        sta viabase+$3

        lda #$ff
        sta viabase+$0
        sta viabase+$1

        ; DDR input
        lda #0
        sta viabase+$2
        sta viabase+$3

        ; init timers
        ; after this all timer values will be = $0000
        ldy #0
        tya
t1a     sta viabase+$5  ; Timer A lo
        sta viabase+$4
        sta viabase+$7
        sta viabase+$6
        sta viabase+$9
        sta viabase+$8
        dey
        bne t1a

        ; acknowledge pending IRQs
        lda viabase+$d
        sta viabase+$d

        rts
}

;-------------------------------------------------------------------------------

clrscr:
        ldx #0
        stx $900F
clp:
        lda #$20
        sta screenmem,x
        sta screenmem+$0100,x
        sta screenmem+$0200,x
        lda #1
        sta colormem,x
        sta colormem+$0100,x
        sta colormem+$0200,x
        inx
        bne clp
        rts

screen_disable:
        lda #250 >> 1
        cmp $9004
        bne *-3
        cmp $9004
        beq *-3

        lda #0
        sta $9002
        sta $9003
        rts
screen_enable:
        lda #250 >> 1
        cmp $9004
        bne *-3
        cmp $9004
        beq *-3

        lda #$00 | charsperline
        sta $9002
        lda #numrows << 1
        sta $9003

        lda #12 - 2
        sta $9000
        lda #38 - 2
        sta $9001
        rts

;hextab:
;        !byte $30,$31,$32,$33,$34,$35,$36,$37,$38,$39,$01,$02,$03,$04,$05,$06
