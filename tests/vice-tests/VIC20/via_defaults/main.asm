        !convtab pet
        !cpu 6510

expanded=1

;-------------------------------------------------------------------------------

viabase1 = $9110
viabase2 = $9120

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
        sei
        jsr screen_disable
        jsr clrscr

        lda #0
        sta screenmem + (charsperline * 4) + 4   ; timer 1 lo saved
        sta screenmem + (charsperline * 4) + 5   ; timer 1 hi saved
        sta screenmem + (charsperline * 4) + 8   ; timer 2 lo saved
        sta screenmem + (charsperline * 4) + 9   ; timer 2 hi saved
        sta screenmem + (charsperline * 14) + 4   ; timer 1 lo saved
        sta screenmem + (charsperline * 14) + 5   ; timer 1 hi saved
        sta screenmem + (charsperline * 14) + 8   ; timer 2 lo saved
        sta screenmem + (charsperline * 14) + 9   ; timer 2 hi saved

        ldx #0
-
        lda hdr,x
        sta screenmem + (charsperline * 0),x
        sta screenmem + (charsperline * 10),x

        ;--------------

        lda viabase1,x
        sta buffer,x
        sta screenmem + (charsperline * 2),x
        lda mask,x
        sta colormem + (charsperline * 0),x
        sta colormem + (charsperline * 1),x
        sta colormem + (charsperline * 2),x
        cmp #5
        bne +
        lda #'*'
        sta screenmem + (charsperline * 1),x
+
        lda buffer,x
        cmp reference,x
        beq +
        lda #2
        sta colormem + (charsperline * 1),x
        sta colormem + (charsperline * 2),x
+

        ;--------------

        lda viabase2,x
        sta buffer,x
        sta screenmem + (charsperline * 12),x
        lda mask2,x
        sta colormem + (charsperline * 10),x
        sta colormem + (charsperline * 11),x
        sta colormem + (charsperline * 12),x
        cmp #5
        bne +
        lda #'*'
        sta screenmem + (charsperline * 11),x
+
        lda buffer,x
        cmp reference2,x
        beq +
        lda #2
        sta colormem + (charsperline * 11),x
        sta colormem + (charsperline * 12),x
+
        inx
        cpx #$10
        bne -

        jsr screen_enable

--
        ldx #0
-
        lda viabase1,x
        sta screenmem + (charsperline * 3),x
        lda viabase2,x
        sta screenmem + (charsperline * 13),x
        inx
        cpx #$10
        bne -

        lda screenmem + (charsperline * 3) + 4   ; timer 1 lo
        cmp screenmem + (charsperline * 4) + 4   ; timer 1 lo saved
        lda screenmem + (charsperline * 3) + 5   ; timer 1 hi
        sbc screenmem + (charsperline * 4) + 5   ; timer 1 hi saved
        bcc + ; hi < old hi
        lda screenmem + (charsperline * 3) + 4   ; timer 1 lo
        sta screenmem + (charsperline * 4) + 4   ; timer 1 lo saved
        lda screenmem + (charsperline * 3) + 5   ; timer 1 hi
        sta screenmem + (charsperline * 4) + 5   ; timer 1 hi saved
+

        lda screenmem + (charsperline * 13) + 4   ; timer 1 lo
        cmp screenmem + (charsperline * 14) + 4   ; timer 1 lo saved
        lda screenmem + (charsperline * 13) + 5   ; timer 1 hi
        sbc screenmem + (charsperline * 14) + 5   ; timer 1 hi saved
        bcc + ; hi < old hi
        lda screenmem + (charsperline * 13) + 4   ; timer 1 lo
        sta screenmem + (charsperline * 14) + 4   ; timer 1 lo saved
        lda screenmem + (charsperline * 13) + 5   ; timer 1 hi
        sta screenmem + (charsperline * 14) + 5   ; timer 1 hi saved
+

        lda screenmem + (charsperline * 3) + 8   ; timer 1 lo
        cmp screenmem + (charsperline * 4) + 8   ; timer 1 lo saved
        lda screenmem + (charsperline * 3) + 9   ; timer 1 hi
        sbc screenmem + (charsperline * 4) + 9   ; timer 1 hi saved
        bcc + ; hi < old hi
        lda screenmem + (charsperline * 3) + 8   ; timer 1 lo
        sta screenmem + (charsperline * 4) + 8   ; timer 1 lo saved
        lda screenmem + (charsperline * 3) + 9   ; timer 1 hi
        sta screenmem + (charsperline * 4) + 9   ; timer 1 hi saved
+

        lda screenmem + (charsperline * 13) + 8   ; timer 1 lo
        cmp screenmem + (charsperline * 14) + 8   ; timer 1 lo saved
        lda screenmem + (charsperline * 13) + 9   ; timer 1 hi
        sbc screenmem + (charsperline * 14) + 9   ; timer 1 hi saved
        bcc + ; hi < old hi
        lda screenmem + (charsperline * 13) + 8   ; timer 1 lo
        sta screenmem + (charsperline * 14) + 8   ; timer 1 lo saved
        lda screenmem + (charsperline * 13) + 9   ; timer 1 hi
        sta screenmem + (charsperline * 14) + 9   ; timer 1 hi saved
+



        jmp --

hdr:    !byte $30,$31,$32,$33,$34,$35,$36,$37,$38,$39,$01,$02,$03,$04,$05,$06

; 12 13 1b 1c 1e 1f

mask:
        !byte 5,5,7,7
        !byte 5,5,5,5
        !byte 5,5,5,7
        !byte 7,5,7,7

buffer:
        !byte 0,0,0,0
        !byte 0,0,0,0
        !byte 0,0,0,0
        !byte 0,0,0,0

reference:
        !byte $ff, $7f, $00, $80
        !byte $8d, $8c, $ff, $ff
        !byte $85, $8b, $00, $40
        !byte $fe, $00, $82, $7f

; 20 22 23 24 25 2b 2c 2e 2f

mask2:
        !byte 7,5,7,7
        !byte 7,7,5,5
        !byte 5,5,5,7
        !byte 7,5,7,7

reference2:
        !byte $f7, $ff, $ff, $00
        !byte $9c, $35, $26, $48
        !byte $2a, $38, $00, $40
        !byte $dc, $00, $c0, $ff

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
