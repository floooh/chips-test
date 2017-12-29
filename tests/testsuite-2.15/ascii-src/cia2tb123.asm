
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

basic
           jmp $a474
           .bend


newbrk
           pla
           pla
           pla
           pla
           pla
           pla
           rts

setbrk
           sei
           lda #$00
           sta $dd0e
           bit $dd0b
           sta $dd0b
           sta $dd09
           sta $dd08
           bit $dd0b
           lda #$7f
           sta $dd0d
           bit $dd0d
           lda #<newbrk
           sta $0316
           lda #>newbrk
           sta $0317
           rts

restorebrk
           pha
           lda #$66
           sta $0316
           lda #$fe
           sta $0317
           jsr $fda3
           pla
           cli
           rts


main
           jsr print
           .byte 13
           .text "(up)cia2tb123"
           .byte 0





           .block
           jmp start
code
           nop
           sta $dd0f
           asl a
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           jsr waitborder
           lda #$01
           jsr $dd02
           jsr restorebrk
           cmp #2
           beq ok
           jsr print
           .byte 13,13
           .text "error 00 01 cycle 1"
           .byte 0
           jsr waitkey
ok
           .bend

           .block
           jmp start
code
           sta $dd0f
           lda #$0a
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           jsr waitborder
           lda #$01
           jsr $dd02
           jsr restorebrk
           cmp #$0a
           beq ok
           jsr print
           .byte 13,13
           .text "error 00 01 cycle 2"
           .byte 0
           jsr waitkey
ok
           .bend


           .block
           jmp start
code
           sta $dd0f
           nop
           .byte $0b
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           jsr waitborder
           lda #$01
           jsr $dd02
           jsr restorebrk
           cmp #2
           beq ok
           jsr print
           .byte 13,13
           .text "error 00 01 cycle 3"
           .byte 0
           jsr waitkey
ok
           .bend





           .block
           jmp start
code
           nop
           sta $dd0f
           nop
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           lda #$0a
           sta $dd06
           jsr waitborder
           lda #$10
           jsr $dd02
           jsr restorebrk
           cmp #$10
           beq ok
           jsr print
           .byte 13,13
           .text "error 00 10 cycle 1"
           .byte 0
           jsr waitkey
ok
           .bend


           .block
           jmp start
code
           sta $dd0f
           lda #$ea
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           lda #$0a
           sta $dd06
           jsr waitborder
           lda #$10
           jsr $dd02
           jsr restorebrk
           cmp #$0a
           beq ok
           jsr print
           .byte 13,13
           .text "error 00 10 cycle 2"
           .byte 0
           jsr waitkey
ok
           .bend


           .block
           jmp start
code
           sta $dd0f
           nop
           nop
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           lda #$0a
           sta $dd06
           jsr waitborder
           lda #$10
           jsr $dd02
           jsr restorebrk
           cmp #$20
           beq ok
           jsr print
           .byte 13,13
           .text "error 00 10 cycle 3"
           .byte 0
           jsr waitkey
ok
           .bend





           .block
           jmp start
code
           nop
           sta $dd0f
           nop
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           lda #$0a
           sta $dd06
           jsr waitborder
           lda #$11
           jsr $dd02
           jsr restorebrk
           cmp #$11
           beq ok
           jsr print
           .byte 13,13
           .text "error 00 11 cycle 1"
           .byte 0
           jsr waitkey
ok
           .bend


           .block
           jmp start
code
           sta $dd0f
           lda #$ea
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           lda #$0a
           sta $dd06
           jsr waitborder
           lda #$11
           jsr $dd02
           jsr restorebrk
           cmp #$0a
           beq ok
           jsr print
           .byte 13,13
           .text "error 00 11 cycle 2"
           .byte 0
           jsr waitkey
ok
           .bend


           .block
           jmp start
code
           sta $dd0f
           nop
           nop
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           lda #$0a
           sta $dd06
           jsr waitborder
           lda #$11
           jsr $dd02
           jsr restorebrk
           cmp #$22
           beq ok
           jsr print
           .byte 13,13
           .text "error 00 11 cycle 3"
           .byte 0
           jsr waitkey
ok
           .bend





           .block
           jmp start
code
           nop
           stx $dd0f
           .byte $15
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           jsr waitborder
           lda #$01
           ldx #$11
           sta $dd0f
           jsr $dd02
           jsr restorebrk
           cmp #$02
           beq ok
           jsr print
           .byte 13,13
           .text "error 01 11 cycle 1"
           .byte 0
           jsr waitkey
ok
           .bend


           .block
           jmp start
code
           stx $dd0f
           lda #$0a
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           jsr waitborder
           lda #$01
           ldx #$11
           sta $dd0f
           jsr $dd02
           jsr restorebrk
           cmp #$0a
           beq ok
           jsr print
           .byte 13,13
           .text "error 01 11 cycle 2"
           .byte 0
           jsr waitkey
ok
           .bend


           .block
           jmp start
code
           stx $dd0f
           nop
           .byte $0a
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           jsr waitborder
           lda #$01
           ldx #$11
           sta $dd0f
           jsr $dd02
           jsr restorebrk
           cmp #$02
           beq ok
           jsr print
           .byte 13,13
           .text "error 01 11 cycle 3"
           .byte 0
           jsr waitkey
ok
           .bend





           .block
           jmp start
code
           nop
           stx $dd0f
           .byte $15
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           jsr waitborder
           lda #$01
           ldx #$10
           sta $dd0f
           jsr $dd02
           jsr restorebrk
           cmp #$02
           beq ok
           jsr print
           .byte 13,13
           .text "error 01 10 cycle 1"
           .byte 0
           jsr waitkey
ok
           .bend


           .block
           jmp start
code
           stx $dd0f
           lda #$0a
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           jsr waitborder
           lda #$01
           ldx #$10
           sta $dd0f
           jsr $dd02
           jsr restorebrk
           cmp #$0a
           beq ok
           jsr print
           .byte 13,13
           .text "error 01 10 cycle 2"
           .byte 0
           jsr waitkey
ok
           .bend


           .block
           jmp start
code
           stx $dd0f
           nop
           asl a
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           jsr waitborder
           lda #$01
           ldx #$10
           sta $dd0f
           jsr $dd02
           jsr restorebrk
           cmp #$02
           beq ok
           jsr print
           .byte 13,13
           .text "error 01 10 cycle 3"
           .byte 0
           jsr waitkey
ok
           .bend





           .block
           jmp start
code
           nop
           stx $dd0f
           .byte $15
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           jsr waitborder
           lda #$01
           ldx #$00
           sta $dd0f
           jsr $dd02
           jsr restorebrk
           cmp #$02
           beq ok
           jsr print
           .byte 13,13
           .text "error 01 00 cycle 1"
           .byte 0
           jsr waitkey
ok
           .bend


           .block
           jmp start
code
           stx $dd0f
           lda #$0a
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           jsr waitborder
           lda #$01
           ldx #$00
           sta $dd0f
           jsr $dd02
           jsr restorebrk
           cmp #$00
           beq ok
           jsr print
           .byte 13,13
           .text "error 01 00 cycle 2"
           .byte 0
           jsr waitkey
ok
           .bend


           .block
           jmp start
code
           stx $dd0f
           nop
           .byte $14
           rts
start
           jsr setbrk
           ldx #0
           stx $dd0f
copy
           lda code,x
           sta $dd02,x
           inx
           cpx #6
           bcc copy
           jsr waitborder
           lda #$01
           ldx #$00
           sta $dd0f
           jsr $dd02
           jsr restorebrk
           cmp #$02
           beq ok
           jsr print
           .byte 13,13
           .text "error 01 00 cycle 3"
           .byte 0
           jsr waitkey
ok
           .bend





           jsr print
           .text " - ok"
           .byte 13,0
           lda turboass
           beq load
           jsr waitkey
           jmp $8000
load
           jsr print
name       .text "cia1pb6"
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


