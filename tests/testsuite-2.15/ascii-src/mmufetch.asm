
           *= $0801
           .byte $4c,$14,$08,$00,$97
turboass   = 780
           .text "780"
           .byte $2c,$30,$3a,$9e,$32,$30
           .byte $37,$33,$00,$00,$00
           lda #1
           sta turboass
           jmp main

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
           .text "(up)mmufetch"
           .byte 0

           jsr rom
           sei

;a000 ram-rom-ram
           ldy #1
           sty $24
           dey
           sty $25
           lda #$36
           sta 1
           lda $a4df
           pha
           lda $a4e0
           pha
           lda $a4e1
           pha
           lda $a4e2
           pha
           lda $a4e3
           pha
           lda #$86
           sta $a4df
           lda #1
           sta $a4e0
           lda #0
           sta $a4e1
           sta $a4e2
           lda #$60
           sta $a4e3
           lda #$36
           ldx #$37
           jsr $a4df
           pla
           sta $a4e3
           pla
           sta $a4e2
           pla
           sta $a4e1
           pla
           sta $a4e0
           pla
           sta $a4df

;b000 ram-rom-ram
           ldy #1
           sty $14
           dey
           sty $15
           lda #$36
           sta 1
           lda $b828
           pha
           lda $b829
           pha
           lda $b82a
           pha
           lda $b82b
           pha
           lda $b82c
           pha
           lda #$86
           sta $b828
           lda #1
           sta $b829
           lda #0
           sta $b82a
           sta $b82b
           lda #$60
           sta $b82c
           lda #$36
           ldx #$37
           jsr $b828
           pla
           sta $b82c
           pla
           sta $b82b
           pla
           sta $b82a
           pla
           sta $b829
           pla
           sta $b828

;e000 ram-rom-ram
           lda #$86
           sta $ea77
           lda #1
           sta $ea78
           lda #0
           sta $ea79
           sta $ea7a
           lda #$60
           sta $ea7b
           lda #$35
           ldx #$37
           sta 1
           jsr $ea77

;f000 ram-rom-ram
           ldy #1
           sty $c3
           dey
           sty $c4
           lda #$86
           sta $fd25
           lda #1
           sta $fd26
           lda #0
           sta $fd27
           sta $fd28
           lda #$60
           sta $fd29
           lda #$35
           ldx #$37
           sta 1
           jsr $fd25

;d000 ram-rom-ram
           lda $91
           pha
           lda $92
           pha
           ldy #1
           sty $91
           dey
           sty $92
           lda #$34
           sta 1
           lda #$86
           sta $d400
           lda #1
           sta $d401
           lda #0
           sta $d402
           sta $d403
           lda #$60
           sta $d404
           lda #$34
           ldx #$33
           sta 1
           jsr $d400
           pla
           sta $92
           pla
           sta $91

;d000 ram-io-ram
           lda #$37
           sta 1
           lda #$85
           sta $d002
           lda #1
           sta $d003
           lda #0
           sta $d004
           lda #$33
           sta 1
           lda #$86
           sta $d000
           lda #1
           sta $d001
           lda #0
           sta $d002
           sta $d003
           lda #$60
           sta $d004
           lda #$34
           ldx #$37
           sta 1
           jsr $d000

           jsr rom

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
name       .text "mmu"
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


