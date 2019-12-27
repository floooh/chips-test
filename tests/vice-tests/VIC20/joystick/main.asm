

!if (expanded=1) {
basicstart = $1201      ; expanded
screenmem = $1000
colormem = $9400
} else {
basicstart = $1001      ; unexpanded
screenmem = $1e00
colormem = $9600
}

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
        jsr clrscr

mainlp:

        lda #$00
        sta $9113 ; DDR
        sta $9122 ; DDR
        
        lda $9111 ; data
        sta temp1
        
        ldx #0
-
        ldy #'.'
        asl temp1
        bcs +
        ldy #'*'
+
        tya
        sta screenmem,x
        inx
        cpx #8
        bne -
        
        lda $9120 ; data
        sta temp2

        ldx #0
-
        ldy #'.'
        asl temp2
        bcs +
        ldy #'*'
+
        tya
        sta screenmem+10,x
        inx
        cpx #8
        bne -

        lda $9111 ; data
        sta temp1
        lda $9120 ; data
        sta temp2
        
        ldy #'.'
        lda temp1
        and #%00100000
        bne +
        ldy #'*'
+
        sty screenmem+80 ; fire
        
        ldy #'.'
        lda temp1
        and #%00010000
        bne +
        ldy #'*'
+
        sty screenmem+80-1 ; left
        
        ldy #'.'
        lda temp1
        and #%00001000
        bne +
        ldy #'*'
+
        sty screenmem+80+22 ; down
        
        ldy #'.'
        lda temp1
        and #%00000100
        bne +
        ldy #'*'
+
        sty screenmem+80-22 ; up  

        ldy #'.'
        lda temp2
        and #%10000000
        bne +
        ldy #'*'
+
        sty screenmem+80+1 ; right
        
        jmp mainlp

temp1: !byte 0
temp2: !byte 0
        
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

