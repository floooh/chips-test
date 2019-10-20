
    .macpack cbm

SCREENOFF = 1

    .export Start

oldCIA1 = $02
oldCIA2 = $03

addrl = $04
addrh = $05

delayl = $06
delayh = $07

Start:
    lda #$93
    jsr $ffd2
    
    sei
    
mainloop:
.ifdef SCREENOFF
    lda #$0b
    sta $d011
.endif
    lda #$35
    sta $01
    
    ldx #0
@lp1:
    lda screen,x
    sta $0400,x
    inx
    bne @lp1

    .repeat 5, count
    lda #$7f
    sta $dc0d
    sta $dd0d

    lda $d011
    bpl *-3
    lda $d011
    bmi *-3

    lda #20+count
    jsr testcia1

    lda delayl
    lsr
    lsr
    lsr
    lsr
    tax
    lda hextab,x
    sta $0400+20+(count*3)
    lda delayl
    and #$0f
    tax
    lda hextab,x
    sta $0400+21+(count*3)
    .endrepeat

    .repeat 5, count
    lda #$7f
    sta $dc0d
    sta $dd0d

    lda $d011
    bpl *-3
    lda $d011
    bmi *-3

    lda #20+count
    jsr testcia2

    lda delayl
    lsr
    lsr
    lsr
    lsr
    tax
    lda hextab,x
    sta $0400+(1*40)+20+(count*3)
    lda delayl
    and #$0f
    tax
    lda hextab,x
    sta $0400+(1*40)+21+(count*3)
    .endrepeat

done:
    ; done
.ifdef SCREENOFF
    lda #$1b
    sta $d011
.endif

    ldy #10
    
    ldx #18
@chk1:    
    lda $0400+20,x
.ifdef TESTNEW
    cmp refnew,x
.else
    cmp refold,x
.endif
    bne @fail
    lda $0428+20,x
.ifdef TESTNEW
    cmp refnew,x
.else
    cmp refold,x
.endif
    bne @fail
    dex
    bpl @chk1
    
    ldy #0
    
@fail:    
    lda #5
    sty $d7ff
    cpy #0
    beq @sk1
    lda #10
@sk1:
    sta $d020

    ldx #50
@lp:
    lda $d011
    bpl *-3
    lda $d011
    bmi *-3
    dex
    bne @lp

    inc $07e7
    jmp mainloop

hextab:
    scrcode "0123456789abcdef"
    
screen:
            ;1234567890123456789012345678901234567890
    scrcode "cia1                                    "
    scrcode "cia2                                    "
    scrcode "                                        "
    scrcode "expected:                               "
refold = * + 20
            ;1234567890123456789012345678901234567890
    scrcode "old (6526)          0b 0b 0c 0c 0d      "
refnew = * + 20
    scrcode "new (6526a)         0a 0b 0b 0c 0c      "
    scrcode "                                        "

;-------------------------------------------------------------------------------
testcia2:
    pha

    ; test CIA2 (NMI)

    ; Set NMI vector
    lda #<continue1
    sta $fffa
    lda #>continue1
    sta $fffb

    lda #%10001000
    sta $dd0e
    lda $dd0d

    lda #$81
    sta $dd0d

    ; Set timer to 5 cycles
    ;lda #20
    pla
    sta $dd04
    lda #0
    sta $dd05

    ; Clear the detection flag
    sta oldCIA2

    ldx #$7f

.ifdef ONESHOT
    lda #%10011001 ; Fire a 1-shot timer
.else
    lda #%10010001 ; normal timer
.endif
    sta $dd0e
    lda $dd0d

testcode1:
    .repeat 20
    nop
    .endrepeat
    ; we should not come here
@lp:
    inc $0429
    jmp @lp

continue1:
    stx $dd0d
    lda $dd0d
    pla
    pla
    sta addrl
    pla
    sta addrh
    
    sec
    lda addrl
    sbc #<testcode1
    sta delayl
    lda addrh
    sbc #>testcode1
    sta delayh
    
    rts
    
;-------------------------------------------------------------------------------
testcia1:
    pha
    ; test CIA1 (IRQ)

    ; Set IRQ vector
    lda #<continue2
    sta $fffe
    lda #>continue2
    sta $ffff

    lda #%10001000
    sta $dc0e
    lda $dc0d

    lda #$81
    sta $dc0d
    cli

    ; Set timer to 5 cycles
    ;lda #4
    pla
    sta $dc04
    lda #0
    sta $dc05

    ; Clear the detection flag
    sta oldCIA1

    ldx #$7f

.ifdef ONESHOT
    lda #%10011001 ; Fire a 1-shot timer
.else
    lda #%10010001 ; normal timer
.endif
    sta $dc0e
    lda $dc0d

testcode2:
    .repeat 20
    nop
    .endrepeat
    ; we should not come here
@lp:
    inc $0428
    jmp @lp

continue2:
    stx $dc0d
    lda $dc0d
    pla
    pla
    sta addrl
    pla
    sta addrh
    
    sec
    lda addrl
    sbc #<testcode2
    sta delayl
    lda addrh
    sbc #>testcode2
    sta delayh
    
    rts

    
    
