SCREENOFF = 1

    .export Start

oldCIA1 = $02
oldCIA2 = $03

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

    lda #$7f
    sta $dc0d
    sta $dd0d

    lda $d011
    bpl *-3
    lda $d011
    bmi *-3

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
    lda #4
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

    ; This should be interrupted before the INC
    ; only if it's a newer chip.
    lda $dd0d
    lda $d020
    inc oldCIA2
    ; we should not come here
@lp:
    inc $0429
    jmp @lp

continue1:
    stx $dd0d
    lda $dd0d
    pla
    pla
    pla

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
    lda #4
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

    ; This should be interrupted before the INC
    ; only if it's a newer chip.
    lda $dc0d
    lda $d020
    inc oldCIA1
    ; we should not come here
@lp:
    inc $0428
    jmp @lp

continue2:
    stx $dc0d
    lda $dc0d
    pla
    pla
    pla

    ; done
.ifdef SCREENOFF
    lda #$1b
    sta $d011
.endif
    lda oldCIA1
    sta $0400
    lda oldCIA2
    sta $0401
    
    ldx #0
    ldy #0
@l1a:
    lda cia1txt,x
    and #$3f
    sta $0400+(4*40),y
    inx
    iny
    cpy #4
    bne @l1a

    lda oldCIA1
    asl a
    asl a
    tax
@l1:
    lda newold,x
    and #$3f
    sta $0400+(4*40),y
    inx
    iny
    cpy #4+4
    bne @l1

    ldx #0
    ldy #0
@l2a:
    lda cia2txt,x
    and #$3f
    sta $0400+(5*40),y
    inx
    iny
    cpy #4
    bne @l2a

    lda oldCIA2
    asl a
    asl a
    tax
@l2:
    lda newold,x
    and #$3f
    sta $0400+(5*40),y
    inx
    iny
    cpy #4+4
    bne @l2
    
    ldy #0
    ldx #5
    
    lda oldCIA1
.ifdef TESTNEW
    cmp #0
.else
    cmp #1
.endif
    beq @sk1

    ldy #$ff
    ldx #10
@sk1:    
    
    lda oldCIA2
.ifdef TESTNEW
    cmp #0
.else
    cmp #1
.endif
    beq @sk2

    ldy #$ff
    ldx #10
@sk2:    

    stx $d020
    sty $d7ff

    ldx #50
@lp:
    lda $d011
    bpl *-3
    lda $d011
    bmi *-3
    dex
    bne @lp

    inc $0450
    jmp mainloop

newold:
    .byte "new "
    .byte "old "
cia1txt:
    .byte "cia1:"
cia2txt:
    .byte "cia2:"
