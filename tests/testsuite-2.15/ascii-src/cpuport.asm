
           *= $0801
           .byte $4c,$14,$08,$00,$97
turboass   = 780
           .text "780"
           .byte $2c,$30,$3a,$9e,$32,$30
           .byte $37,$33,$00,$00,$00
           lda #1
           sta turboass
           jmp main


config     .byte 0
abackup    .byte 0,0
laststate  .byte 0
right      .byte 0


rom
           lda #$2f
           sta 0
           lda #$37
           sta 1
           cli
           rts


main
           jsr print
           .byte 13
           .text "(up)cpuport"
           .byte 0

           lda #0
           sta config
nextconfig
           sei
           lda #$ff
           sta 0
           sta 1
           sta abackup+0
           sta abackup+1
           sta laststate
           ldx #8
           lda config
push
           asl a
           php
           dex
           bne push
           ldx #4
pull
           pla
           and #1
           tay
           lda #0
           plp
           sbc #0
           sta 0,y
           sta abackup,y

;inputs: keep last state
           lda abackup+0
           eor #$ff
           and laststate
           sta or1+1
;outputs: set new state
           lda abackup+0
           and abackup+1
or1        ora #$11
           sta laststate

;delay for larger capacitives
           ldy #0
delay
           iny
           bne delay

           dex
           bne pull

           lda abackup+0
           cmp 0
           bne error

           lda abackup+0
           eor #$ff
           ora abackup+1
           and #$37
           sta or2+1
           lda laststate
           and #$c8
or2        ora #$11

;bit 5 is drawn low if input
           tax
           lda #$20
           bit abackup+0
           bne no5low
           txa
           and #$df
           tax
no5low
           stx right
           cpx 1
           bne error
noerror
           inc config
           bne nextconfig
           jsr rom
           jmp ok

error
           lda 1
           pha
           lda 0
           pha
           jsr rom

           jsr print
           .byte 13
           .text "0=ff 1=ff"
           .byte 0

           ldx #8
           lda config
push1
           asl a
           php
           dex
           bne push1
           ldx #4
pull1
           lda #32
           jsr $ffd2
           pla
           and #1
           ora #"0"
           jsr $ffd2
           lda #"="
           jsr $ffd2
           lda #0
           plp
           sbc #0
           stx oldx+1
           jsr printhb
oldx
           ldx #$11
           dex
           bne pull1
           jsr print

           .byte 13
           .text "after  "
           .byte 0

           pla
           jsr printhb
           lda #32
           jsr $ffd2
           pla
           jsr printhb
           jsr print
           .byte 13
           .text "right  "
           .byte 0
           lda abackup+0
           jsr printhb
           lda #32
           jsr $ffd2
           lda right
           jsr printhb
           lda #13
           jsr $ffd2

waitk
           jsr $ffe4
           beq waitk
           cmp #3
           beq stop
           jmp noerror
stop
           lda turboass
           beq basic
           jmp $8000
basic
           jmp $a474


ok
           jsr print
           .text " - ok"
           .byte 13,0
           lda turboass
           beq load
wait       jsr $ffe4
           beq wait
           jmp $8000

load
           lda #47
           sta 0
           jsr print
name       .text "cputiming"
namelen    = *-name
           .byte 0
           lda #0
           sta $0a
           sta $b9
           lda #namelen
           sta $b7
           lda #<name
           sta $bb
           lda #>name
           sta $bc
           pla
           pla
           jmp $e16f


print      pla
           .block
           sta print0+1
           pla
           sta print0+2
           ldx #1
print0     lda !*,x
           beq print1
           jsr $ffd2
           inx
           bne print0
print1     sec
           txa
           adc print0+1
           sta print2+1
           lda #0
           adc print0+2
           sta print2+2
print2     jmp !*
           .bend

printhb
           .block
           pha
           lsr a
           lsr a
           lsr a
           lsr a
           jsr printhn
           pla
           and #$0f
printhn
           ora #$30
           cmp #$3a
           bcc printhn0
           adc #6
printhn0
           jsr $ffd2
           rts
           .bend


