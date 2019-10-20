         PROCESSOR 6502
         ORG $801

; BASIC listing
         hex 0b 08 0a 00  9e 32 30 36  31 00 00 00

;reset TOD alarm to 0
        JSR resalm
;set TOD time to constant value
        JSR set
        LDY #$00
;check if TOD time and TOD alarm are identical
ckagain JSR check
        INY
        CPY #$05
        BEQ stpck
        LDA #$00
        STA $dc07,Y
        JMP ckagain
;exit
stpck
        ldy #5
        ldx #4
chklp:
        lda expected,x
        cmp $0400,x
        beq chkskp
        ldy #10
chkskp
        dex
        bpl chklp
        sty $d020

        lda $d020
        and #$0f
        ldx #0 ; success
        cmp #5
        beq nofail
        ldx #$ff ; failure
nofail:
        stx $d7ff

        RTS

expected: hex 18 18 18 18 04

;check if TOD time and TOD alarm are identical
;if they are, prints a value (depending on Y register)
;if they are not, prints an X
check   LDA $dc0d
        LSR
        LSR
        LSR
        TYA
        BCS fine
        LDA #$18
fine    STA $400,Y
        RTS

;constant value to which TOD time is set: 9hours,30mins,12.3secs
vax     hex 03 12 30 09

;sets TOD time (or TOD alarm if called when bit 7 of DC0F is 1) to above constant value
set     LDY #$03
iniz    LDA vax,Y
        STA $dc08,Y
        DEY
        BPL iniz
        rts

;sets TOD alarm to above constant value
setalm  lda $dc0f
        ora #$80
        sta $dc0f
        JSR set
        lda $dc0f
        and #$7f
        sta $dc0f
        rts

;sets TOD alarm to 0:0:0.0
resalm  lda $dc0f
        ora #$80
        sta $dc0f
        LDY #$00
        LDA #$00
cont    STA $dc08,Y
        INY
        CPY #$04
        BCC cont
        lda $dc0f
        and #$7F
        sta $dc0f
        rts


