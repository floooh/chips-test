
           *= $0801
           .byte $4c,$14,$08,$00,$97
turboass   = 780
           .text "780"
           .byte $2c,$30,$3a,$9e,$32,$30
           .byte $37,$33,$00,$00,$00
           lda #1
           sta turboass
           jmp main


print
           .block
           pla
           sta print0+1
           pla
           sta print0+2
           ldx #1
print0
           lda $1111,x
           beq print1
           jsr $ffd2
           inx
           bne print0
print1
           sec
           txa
           adc print0+1
           sta print2+1
           lda #0
           adc print0+2
           sta print2+2
print2
           jmp $1111
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


waitborder
           .block
           lda $d011
           bmi ok
wait
           lda $d012
           cmp #30
           bcs wait
ok
           rts
           .bend


waitkey
           .block
           jsr $fda3
wait
           jsr $ffe4
           beq wait
           cmp #3
           beq stop
           rts
stop
           lda turboass
           beq basic
           jmp $8000
basic
           jmp $a474
           .bend

;---------------------------------------

index      .byte 0
reg        .byte 0
areg       .byte $04,$06,$01,$0d

main
           jsr print
           .byte 13,145
           .text "cia1tab"
           .byte 0

           ldx #$7e
           lda #$ea ;nop
makechain
           sta $2000,x
           dex
           bpl makechain
           lda #$60 ;rts
           sta $207f

           sei
           lda #0
           sta write+1
           sta reg
nextreg
           lda #0
           sta index
nextindex
           lda #$ff
           sta $dc03
           lda #$00
           sta $dc01
           sta $dc0e
           sta $dc0f
           lda #$7f
           sta $dc0d
           bit $dc0d
           lda #21
           sta $dc04
           lda #2
           sta $dc06
           ldx #0
           stx $dc05
           stx $dc07
           sta $dc04
           lda #$82
           sta $dc0d
           lda index
           eor #$ff
           lsr a
           php
           sta jump+1
           ldx reg
           lda areg,x
           sta readreg+1
           jsr waitborder
           lda #%01000111
           sta $dc0f
           lda #%00000011
           sta $dc0e
           plp
           bcc jump
jump
           jsr $2011
readreg
           lda $dc11
write
           sta $2111
           inc write+1
           inc index
           lda index
           cmp #12
           bcc nextindex
           inc reg
           lda reg
           cmp #4
           bcc nextreg

;---------------------------------------
;compare result

           jmp compare
right      .byte $01,$02,$02,$01,$02,$02
           .byte $01,$02,$02,$01,$02,$02
           .byte $02,$02,$02,$01,$01,$01
           .byte $00,$00,$02,$02,$02,$02
           .byte $80,$c0,$80,$80,$c0,$80
           .byte $80,$c0,$00,$00,$40,$00
           .byte $00,$01,$01,$01,$01,$01
           .byte $01,$01,$03,$83,$83,$83
compare
           jsr $fda3
           sei
           ldx #0
comp
           lda $2100,x
           cmp right,x
           bne diff
           inx
           cpx #12*4
           bcc comp
           jmp ok
diff


;---------------------------------------
;print result

           ldy #0
           jsr print
           .byte 13
           .text "ta "
           .byte 13
           .text "   "
           .byte 0
           jsr print12
           jsr print
           .text "tb "
           .byte 13
           .text "   "
           .byte 0
           jsr print12
           jsr print
           .text "pb "
           .byte 13
           .text "   "
           .byte 0
           jsr print12
           jsr print
           .text "icr"
           .byte 13
           .text "   "
           .byte 0
           jsr print12
           jsr waitkey
           jmp outend

print12
           ldx #12
loop12
           lda #32
           jsr $ffd2
           lda right,y
           jsr printhb
           dec 211
           dec 211
           dec 211
           lda #145
           jsr $ffd2
           lda #32
           jsr $ffd2
           lda 646
           pha
           lda $2100,y
           cmp right,y
           beq nodiff
           pha
           lda #2
           sta 646
           pla
nodiff
           jsr printhb
           pla
           sta 646
           lda #17
           jsr $ffd2
           iny
           dex
           bne loop12
           lda #13
           jmp $ffd2
outend


;---------------------------------------
;load next part of the test suite

ok
           jsr print
           .text " - ok"
           .byte 13,0
           lda turboass
           beq load
           jsr waitkey
           jmp $8000
load
           jsr print
name       .text "loadth"
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


