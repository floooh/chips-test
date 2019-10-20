;**************************************************************************
;*
;* FILE  cia-int.asm
;* Copyright (c) 2010 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;*
;* DESCRIPTION
;*
;******
        processor 6502

        seg.u zp
;**************************************************************************
;*
;* SECTION  zero page
;*
;******
        org $c0
ptr_zp:
        ds.w 1
cnt_zp:
        ds.b 1
test_zp:
        ds.b 1
int_zp:
        ds.b 1
time_zp:
        ds.b 1

        seg code
        org $0801
;**************************************************************************
;*
;* Basic line!
;*
;******
StartOfFile:
        dc.w EndLine
        dc.w 0
        dc.b $9e,"2069 /T.L.R/",0
;         0 SYS2069 /T.L.R/
EndLine:
        dc.w 0

;**************************************************************************
;*
;* SysAddress... When run we will enter here!
;*
;******
SysAddress:
        jsr clearscreen
loop:
        sei
        lda #$7f
        sta $dc0d
        lda $dc0d

        jsr test_present
        
        sei
        lda #$35
        sta $01

        ldx #6
sa_lp1:
        lda vectors - 1,x
        sta $fffa - 1,x
        dex
        bne sa_lp1

        jsr test_prepare
        
        jsr test_perform

enda:
; jmp enda
        sei
        lda #$37
        sta $01
        ldx #0
        stx $d01a
        inx
        stx $d019
        
        jsr $fda3
        jsr test_result

        lda $d020
        and #$0f
        ldx #0 ; success
        cmp #5
        beq nofail
        ldx #$ff ; failure
nofail:
        stx $d7ff

        ; start over
        jmp     loop

vectors:
        ifconst TEST_IRQ
        dc.w nmi_entry, 0, int_entry
        endif
        ifconst TEST_NMI
        dc.w int_entry, 0, int_entry
        endif
        
sa_fl1:
nmi_entry:
        sei
        lda #$37
        sta $01
        jmp $fe66

;**************************************************************************
;*
;* NAME  wait_vb
;*   
;******
wait_vb:
wv_lp1:
        bit $d011
        bpl wv_lp1
wv_lp2:
        bit $d011
        bmi wv_lp2
        rts

;**************************************************************************
;*
;* NAME  test_present
;*   
;******
test_present:
        lda #14
        sta 646
        lda testfail+1
        sta $d020
        lda #6
        sta $d021

        lda #<greet_msg
        ldy #>greet_msg
        jsr $ab1e
        
        ldx #0
tpr_lp1:
        lda 646
        sta $d800,x
        sta $d900,x
        sta $da00,x
        sta $db00,x
        inx
        bne tpr_lp1
        rts

clearscreen:
        ldx #0
tpr_lp2:
        lda #$20
        sta $0400,x
        sta $0500,x
        sta $0600,x
        sta $0700,x
        inx
        bne tpr_lp2
        rts
        
        
greet_msg:
    ifconst TEST_IRQ
        dc.b 19, "CIA-INT (IRQ) R04 / TLR ("
    if NEWCIA = 0
        dc.b    "OLD"
    endif
    if NEWCIA = 1
        dc.b    "NEW"
    endif
        dc.b    " CIA)",13,13
        dc.b "DC0C: A9 XX 60",13
        dc.b 13,13,13
        dc.b "DC0C: A5 XX 60",13
        dc.b 13,13,13
        dc.b "DC0B: 0D A9 XX 60",13
        dc.b 13,13,13
        dc.b "DC0B: 19 FF XX 60",13
        dc.b 13,13,13
        dc.b "DC0C: AC XX A9 09 28 60",13
        dc.b 0
        endif
        ifconst TEST_NMI
        dc.b 19, "CIA-INT (NMI) R04 / TLR ("
    if NEWCIA = 0
        dc.b    "OLD"
    endif
    if NEWCIA = 1
        dc.b    "NEW"
    endif
        dc.b    " CIA)",13,13
        dc.b "DD0C: A9 XX 60",13
        dc.b 13,13,13
        dc.b "DD0C: A5 XX 60",13
        dc.b 13,13,13
        dc.b "DD0B: 0D A9 XX 60",13
        dc.b 13,13,13
        dc.b "DD0B: 19 FF XX 60",13
        dc.b 13,13,13
        dc.b "DD0C: AC XX A9 09 28 60",13
        dc.b 0
    endif

;**************************************************************************
;*
;* NAME  test_result
;*   
;******
test_result:
        lda #>(test_reference+0*$10)
        sta chkba1+2
        lda #<(test_reference+0*$10)
        sta chkba1+1

        lda #>($0400+3*40)
        sta chkba2+2
        lda #<($0400+3*40)
        sta chkba2+1

        lda #>($d800+3*40)
        sta chkba3+2
        lda #<($d800+3*40)
        sta chkba3+1

        lda #0
        sta mlpcnt+1
ckmlp:

        ldx #0
        ldy #0
cklp1:
        jsr chkbyte
        iny
        inx
        cpx #$18
        bne cklp1

        ;ldx #$18
        ldy #$28
cklp2:
        jsr chkbyte
        iny
        inx
        cpx #$18+$18
        bne cklp2

        clc
        lda chkba1+1
        adc #$18+$18
        sta chkba1+1
        bcc cksk1
        inc chkba1+2
cksk1:
        clc
        lda chkba2+1
        adc #$28*4
        sta chkba2+1
        bcc cksk2
        inc chkba2+2
cksk2:

        clc
        lda chkba3+1
        adc #$28*4
        sta chkba3+1
        bcc cksk3
        inc chkba3+2
cksk3:

        inc mlpcnt+1
mlpcnt: lda #0
        cmp #5
        bne ckmlp

testfail:
        lda #5
        sta $d020
        rts

chkbyte:
chkba1: lda test_reference+0*$10,x
chkba2: cmp $0400+3*40,y
        jsr chkref
chkba3: sta $d800+3*40,y
        rts
        
chkref:
        beq ckrok
        lda #2
        sta testfail+1
        rts
ckrok:
        lda #5
        rts

;**************************************************************************
;*
;* NAME  test_prepare
;*   
;******
test_prepare:
        lda #$34
        sta $01

; make LDA $xx mostly return xx ^ $2f.
        ldx #2
tpp_lp1:
        txa
        eor #$2f
        sta $00,x
        inx
        bne tpp_lp1

; make LDA $xxA9 mostly return xx.
        ldx #0
        stx $00a9
        inx
        stx $01a9
        inx
        stx $02a9
        inx
        stx $03a9
        
        ldx #>[end+$ff]
tpp_lp2:
        stx tpp_sm1+2
tpp_sm1:
        stx.w $00a9
        inx
        bne tpp_lp2

; make LDA $xxFF,Y with Y=$19 mostly return xx.
        ldx #0
        stx $00ff+$19
        inx
        stx $01ff+$19
        inx
        stx $02ff+$19
        
        ldx #>[end+$ff]
tpp_lp3:
        stx tpp_sm2+2
        inc tpp_sm2+2
tpp_sm2:
        stx.w $00ff+$19
        inx
        bne tpp_lp3

; make LDA $A9xx return xx.
        ldx #0
tpp_lp4:
        txa
        sta $a900,x
        inx
        bne tpp_lp4

        inc $01

; initial test sequencer
        lda #0
        sta test_zp

        rts

        
;**************************************************************************
;*
;* NAME  test_perform
;*   
;******
test_perform:
; stay away from bad lines
tp_lp00:
        lda $d011
        bmi tp_lp00
tp_lp01:
        lda $d011
        bpl tp_lp01
; just below the visible screen here
        
        inc $d020

; kill all interrupts
        lda #$7f
        sta $dc0d
        sta $dd0d
        lda $dc0d
        lda $dd0d

        ldx test_zp
        jsr do_test

; restore data direction and ports
        ldy #0
        lda #$ff
        sta $dc02
        sty $dc03
        lda #$7f
        sta $dc00
        lda #$3f
        sta $dd02
        sty $dd03
        lda #$17
        sta $dd00

        dec     $d020
        
        ldx test_zp
        inx
        cpx #NUM_TESTS
        bne tp_skp1         ; do next test
        
        ; all tests done, return
        ldx #0
        stx     test_zp
        rts
tp_skp1:
        stx test_zp

; dec $d020

        jmp test_perform ; test forever
; rts

NUM_TESTS equ 5
scrtab:
        dc.w $0400+40*3
        dc.w $0400+40*7
        dc.w $0400+40*11
        dc.w $0400+40*15
        dc.w $0400+40*19
addrtab:
        dc.b $0c
        dc.b $0c
        dc.b $0b
        dc.b $0b
        dc.b $0c
convtab:
        dc.b $ea,$ea  ; NOP
        dc.b $49,$2f  ; EOR #$2f
        dc.b $ea,$ea  ; NOP
        dc.b $ea,$ea  ; NOP
        dc.b $98,$ea  ; TYA
offstab:
        dc.b SEQTAB1
        dc.b SEQTAB2
        dc.b SEQTAB3
        dc.b SEQTAB4
        dc.b SEQTAB5
        
seqtab:
SEQTAB1 equ .-seqtab
        dc.b $0c,$a9  ; LDA #<imm>
; dc.b $0d,%10000010 ; timer B IRQ
        dc.b $0e,$60  ; RTS
        dc.b $ff

SEQTAB2 equ .-seqtab
        dc.b $0c,$a5  ; LDA <zp>
; dc.b $0d,%10000010 ; timer B IRQ
        dc.b $0e,$60  ; RTS
        dc.b $ff

SEQTAB3 equ .-seqtab
        dc.b $0f,%00000000 ; TOD clock mode
        dc.b $0b,$0d  ; ORA <abs>
        dc.b $0c,$a9  ; $a9
; dc.b $0d,%10000010 ; timer B IRQ
        dc.b $0e,$60  ; RTS
        dc.b $ff

SEQTAB4 equ .-seqtab
        dc.b $0f,%00000000 ; TOD clock mode
        dc.b $0b,$19  ; ORA <abs>,y
        dc.b $0c,$ff  ; $ff
; dc.b $0d,%10000010 ; timer B IRQ
        dc.b $0e,$60  ; RTS
        dc.b $ff

SEQTAB5 equ .-seqtab
        dc.b $02,%11111111   ; DDR out
        dc.b $03,%11111111   ; DDR out
        dc.b $0c,$ac  ; LDY <abs>
; dc.b $0d,%10000010 ; timer B IRQ
        dc.b $0e,$a9  ;
; dc.b $0f,%10000000 ; ORA #<imm> or PHP 
        dc.b $10,$28  ; PLP
        dc.b $11,$60  ; RTS
        dc.b $ff

        subroutine do_test
        ifconst TEST_IRQ
.cia_dut equ $dc00
.cia_sec equ $dd00
        endif
        ifconst TEST_NMI
.cia_dut equ $dd00
.cia_sec equ $dc00
        endif

do_test:
        txa
        asl
        tay
        lda scrtab,y
        sta ptr_zp
        lda scrtab+1,y
        sta ptr_zp+1
        lda addrtab,x
        sta dt_sm1+1
        lda convtab,y
        sta dt_sm2
        lda convtab+1,y
        sta dt_sm2+1

        lda .cia_dut+$08  ; reset tod state
; set up test parameters
        ldy offstab,x
dt_lp0:
        lda seqtab,y
        bmi dt_skp1
        sta dt_sm0+1
        lda seqtab+1,y
dt_sm0:
        sta .cia_dut
        iny
        iny
        bne dt_lp0
dt_skp1:

        ldy #0
dt_lp1:
        sty cnt_zp

        lda #255
        sta .cia_sec+$04
        ldx #0
        stx .cia_sec+$05
        sty .cia_dut+$06    ; 0..23
        stx .cia_dut+$07
        stx int_zp
        stx time_zp
        bit .cia_dut+$0d
        lda #%10000010 ; timer B IRQ
        sta .cia_dut+$0d
        txa
        ldy #%00011001
; X=0, Y=$19, Acc=0
;-------------------------------
; Test start
;-------------------------------
        cli
        sty .cia_sec+$0e ; measurement
        sty .cia_dut+$0f ; actual test
dt_sm1:
        jsr .cia_dut+$0c
        sei
        ldx #%01111111
        stx .cia_dut+$0d
;-------------------------------
; Test end
;-------------------------------
dt_sm2:
        ds.b 2,$ea
; Acc=$Dx0D, int_zp, time_zp
        ldy cnt_zp
        cmp #0
        bne dt_skp2
        lda #"-"
dt_skp2:
        sta (ptr_zp),y      ; result to screen
        tya
        clc
        adc #40
        tay
        lda time_zp
        eor #$7f
        clc
        sbc #16
        ldx int_zp
        bne dt_skp3
        lda #"-"
dt_skp3:
        sta (ptr_zp),y      ; result to screen

        ldy cnt_zp
        iny
        cpy #24
        bne dt_lp1
        rts

int_entry:
        ldx .cia_sec+$04
        bit .cia_dut+$0d
        stx time_zp
        inc int_zp
        rti

;-------------------------------------------------------------------------------
; reference data, see readme.txt

test_reference:
    ; CIA1/IRQ test
    ifconst TEST_IRQ
    if NEWCIA = 0
        ; PAL, "old" CIA
        dc.b    $2d,$2d,$82,$82,$82,$02,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;--....------------------
        dc.b    $81,$81,$83,$83,$2d,$2d,$89,$89,$89,$89,$8b,$8b,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;....--......------------

        dc.b    $2d,$2d,$82,$82,$82,$02,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;--....------------------
        dc.b    $81,$81,$84,$84,$84,$2d,$8a,$8a,$8a,$8a,$8a,$8c,$8c,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;.....-.......-----------

        dc.b    $2d,$2d,$82,$82,$82,$82,$02,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;--.....-----------------
        dc.b    $81,$81,$85,$85,$85,$85,$2d,$8b,$8b,$8b,$8b,$8b,$8d,$8d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;......-.......----------

        dc.b    $2d,$2d,$82,$82,$82,$82,$02,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;--.....-----------------
        dc.b    $81,$81,$2d,$2d,$2d,$2d,$2d,$8c,$8c,$8c,$8c,$8c,$8c,$8e,$8e,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;..-----........---------

        dc.b    $2d,$2d,$82,$82,$82,$02,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;--....------------------
        dc.b    $81,$81,$2d,$2d,$2d,$2d,$88,$88,$88,$8d,$8d,$8d,$8d,$8d,$8f,$8f,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;..----..........--------
    endif
    ; PAL, "new" CIA
    if NEWCIA = 1
        dc.b    $2d,$2d,$2d,$82,$82,$82,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;---...------------------
        dc.b    $81,$81,$81,$83,$83,$2d,$89,$89,$89,$89,$89,$8b,$8b,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;.....-.......-----------

        dc.b    $2d,$2d,$2d,$82,$82,$82,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;---...------------------
        dc.b    $81,$81,$81,$84,$84,$84,$8a,$8a,$8a,$8a,$8a,$8a,$8c,$8c,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;..............----------

        dc.b    $2d,$2d,$2d,$82,$82,$82,$82,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;---....-----------------
        dc.b    $81,$81,$81,$85,$85,$85,$85,$8b,$8b,$8b,$8b,$8b,$8b,$8d,$8d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;...............---------

        dc.b    $2d,$2d,$2d,$82,$82,$82,$82,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;---....-----------------
        dc.b    $81,$81,$81,$2d,$2d,$2d,$2d,$8c,$8c,$8c,$8c,$8c,$8c,$8c,$8e,$8e,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;...----.........--------

        dc.b    $2d,$2d,$2d,$82,$82,$82,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;---...------------------
        dc.b    $81,$81,$81,$2d,$2d,$2d,$88,$88,$88,$8d,$8d,$8d,$8d,$8d,$8d,$8f,$8f,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;...---...........-------
    endif
    endif

    ; CIA2/NMI test
    ifconst TEST_NMI
    ; PAL, "old" CIA
    if NEWCIA = 0
        dc.b    $2d,$2d,$82,$82,$82,$02,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;--....------------------
        dc.b    $81,$81,$83,$83,$89,$2d,$89,$89,$89,$89,$8b,$8b,$8d,$8d,$91,$91,$91,$91,$93,$93,$2d,$2d,$2d,$2d  ;.....-..............----

        dc.b    $2d,$2d,$82,$82,$82,$02,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;--....------------------
        dc.b    $81,$81,$84,$84,$84,$2d,$8a,$8a,$8a,$8a,$8a,$8c,$8c,$8e,$8e,$92,$92,$92,$92,$94,$94,$2d,$2d,$2d  ;.....-...............---

        dc.b    $2d,$2d,$82,$82,$82,$82,$02,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;--.....-----------------
        dc.b    $81,$81,$85,$85,$85,$85,$2d,$8b,$8b,$8b,$8b,$8b,$8d,$8d,$8f,$8f,$93,$93,$93,$93,$95,$95,$2d,$2d  ;......-...............--

        dc.b    $2d,$2d,$82,$82,$82,$82,$02,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;--.....-----------------
        dc.b    $81,$81,$86,$86,$86,$86,$2d,$8c,$8c,$8c,$8c,$8c,$8c,$8e,$8e,$90,$90,$94,$94,$94,$94,$96,$96,$2d  ;......-................-

        dc.b    $2d,$2d,$82,$82,$82,$02,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;--....------------------
        dc.b    $81,$81,$85,$85,$85,$2d,$88,$88,$88,$8d,$8d,$8d,$8d,$8d,$8f,$8f,$91,$91,$95,$95,$95,$95,$97,$97  ;.....-..................
    endif
    ; PAL, "new" CIA
    if NEWCIA = 1
        dc.b    $2d,$2d,$2d,$82,$82,$82,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;--....------------------
        dc.b    $81,$81,$81,$83,$83,$89,$89,$89,$89,$89,$89,$8b,$8b,$8d,$8d,$91,$91,$91,$91,$93,$2d,$2d,$2d,$2d  ;.....-..............----

        dc.b    $2d,$2d,$2d,$82,$82,$82,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;--....------------------
        dc.b    $81,$81,$81,$84,$84,$84,$8a,$8a,$8a,$8a,$8a,$8a,$8c,$8c,$8e,$8e,$92,$92,$92,$92,$94,$2d,$2d,$2d  ;.....-...............---

        dc.b    $2d,$2d,$2d,$82,$82,$82,$82,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;--.....-----------------
        dc.b    $81,$81,$81,$85,$85,$85,$85,$8b,$8b,$8b,$8b,$8b,$8b,$8d,$8d,$8f,$8f,$93,$93,$93,$93,$95,$2d,$2d  ;......-...............--

        dc.b    $2d,$2d,$2d,$82,$82,$82,$82,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;--.....-----------------
        dc.b    $81,$81,$81,$86,$86,$86,$86,$8c,$8c,$8c,$8c,$8c,$8c,$8c,$8e,$8e,$90,$90,$94,$94,$94,$94,$96,$2d  ;......-................-

        dc.b    $2d,$2d,$2d,$82,$82,$82,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d  ;--....------------------
        dc.b    $81,$81,$81,$85,$85,$85,$88,$88,$88,$8d,$8d,$8d,$8d,$8d,$8d,$8f,$8f,$91,$91,$95,$95,$95,$95,$97  ;.....-..................
    endif
    endif

end:
