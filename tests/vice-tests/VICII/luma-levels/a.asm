

                *=$801
        .word +, 2016
        .null $9e, "2061"
+       .word 0

start        
        sei
        lda #11
        sta $d011

        lda $d011
        bpl *-3
        jmp loop
        
        .page
loop    lda $d011
        bmi *-3
        ldx #8
-       dex
        bne -
        nop
        lda $d012
        cmp $d012
        beq *+2

        ldx #9
        cmp #10
        bcc -
        pha
        pla
        jsr w
        jsr w
        jsr w
        ldx #150
        jsr bar

        jsr w
        jsr w
        jsr w
        bit $ea
        ldx #150
        jsr bar

        jmp loop

lp      sta $d020
        lda #7
        sta $d020
        lda #3
        sta $d020
        lda #5
        sta $d020
        lda #14
        sta $d020
        lda #4
        sta $d020
        lda #2
        sta $d020
        lda #6
        sta $d020
        lda #0
        sta $d020
bar     nop
        nop
        lda #1
        dex
        bne lp
w       rts

        .endp
