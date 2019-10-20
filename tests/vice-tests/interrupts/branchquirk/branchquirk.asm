
testloc = $02
testnum = $03

!if NMI = 0 {
    ciabase = $dc00
} else {
    ciabase = $dd00
}

        *=$0801
        ; BASIC stub: "1 SYS 2061"
        !byte $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00

start: 
        ldx #0
        stx $d020
        stx $d021
-
        lda #$20
        sta $0400,x
        sta $0500,x
        sta $0600,x
        sta $0700,x
        lda #$01
        sta $d800,x
        sta $d900,x
        sta $da00,x
        sta $db00,x
        inx
        bne -

        ldx #39
-
        lda testname,x
        sta $0400+(24*40),x
        dex
        bpl -
        
        sei
        lda #$7f
        sta ciabase + $0d
!if NMI = 1 {
        sta $dc0d
        lda $dc0d
}
        lda ciabase + $0d
        lda #$35
        sta $01
        lda #>irq
        sta $ffff
        sta $fffb
        lda #<irq
        sta $fffe
        sta $fffa
        cli
        jmp dotests
        
!macro test taken {
        ldx #0
--
        stx testnum
        
        ; stop timer
        lda #0
        sta ciabase + $0e

        lda #$7f
        sta ciabase + $0d
        
        lda #0
        sta ciabase + $05
        txa
        clc
        adc #46
        sta ciabase + $04
        
        lda #$81
        sta ciabase + $0d
        lda ciabase + $0d

        lda #$00
        sta testloc
        ldy #42
        
-       lda $d011
        bpl -
-       lda $d011
        bmi -
        
        ; force load and start
        lda #%00010001
        sta ciabase + $0e
        
        bit $eaea
        bit $eaea
        bit $eaea
        bit $eaea
        bit $eaea
        bit $eaea
        bit $eaea
        bit $eaea
        bit $eaea
        bit $eaea
        lda #$ff
        nop
        nop
        nop
        nop
        nop
        nop

!if (taken = 1) {
        ; will always be taken
        bne +
} else {
        beq +
}
        sty testloc
+
        inc testloc
        inc testloc
        inc testloc

        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        
        inx
        cpx #$20
        bne --
        
        lda #$7f
        sta ciabase + $0d
        lda ciabase + $0d
        
        rts
}

        * = $900
test1:
        +test 1

        * = $980
test3:
        +test 0
        
        * = $a00 + ($100 - $60)
test2:
        +test 1
        
        * = $c00 + ($100 - $60)
test4:
        +test 0
        
dotests:
        lda #>($0400+(40*0))
        sta screenaddr1+1
        lda #<($0400+(40*0))
        sta screenaddr1+0

        lda #>($0400+(40*5))
        sta screenaddr2+1
        lda #<($0400+(40*5))
        sta screenaddr2+0

        jsr test1
        jsr nextline

        jsr test2
        jsr nextline

        jsr test3
        jsr nextline

        jsr test4
        
        jsr compare
        
        jmp dotests

nextline:
        lda screenaddr1+0
        clc 
        adc #40
        sta screenaddr1+0
        bcc +
        inc screenaddr1+1
+
        lda screenaddr2+0
        clc 
        adc #40
        sta screenaddr2+0
        bcc +
        inc screenaddr2+1
+
        rts
        
compare:
        lda #5
        sta result

        ldx #0
-        

        ldy #5
        lda $0400,x
        cmp reference,x
        beq +
        ldy #10
        sty result
+
        tya
        sta $d800,x
        

        ldy #5
        lda $0500,x
        cmp reference+$100,x
        beq +
        ldy #10
        sty result
+
        tya
        sta $d900,x
        

        inx
        bne -
        
        ldy #$00
result = * + 1
        lda #0
        sta $d020
        cmp #5
        beq +
        ldy #$ff
+
        sty $d7ff
        rts
        
        !align 255,0,0
irq:
        pha
        txa
        pha

        lda ciabase + $04
        pha
        
        ; stop timer
        lda #0
        sta ciabase + $0e

        lda #$7f
        sta ciabase + $0d

        lda testnum
        lda testloc
screenaddr1 = * + 1
        sta $0400,x
        
        pla
screenaddr2 = * + 1
        sta $0400,x
        
        lda ciabase + $0d

        pla
        tax
        pla
        rti
        
reference:
!if CIA = 0 {
        !binary "dumpold.prg",,2
} else {
        !binary "dumpnew.prg",,2
}

testname:
         
    !scr "branchquirk "
!if NMI = 0 {
    !scr "irq"
} else {
    !scr "nmi"
}
    !scr "                  "
!if CIA = 0 {
    !scr "old"
} else {
    !scr "new"
}
    !scr " cia"
