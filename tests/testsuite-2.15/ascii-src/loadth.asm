;---------------------------------------
;loadth.asm - this file is part
;of the C64 Emulator Test Suite
;public domain, no copyright

           *= $0801
           .byte $4c,$14,$08,$00,$97
turboass   = 780
           .text "780"
           .byte $2c,$30,$3a,$9e,$32,$30
           .byte $37,$33,$00,$00,$00
           .block
           lda #1
           sta turboass
           ldx #0
           stx $d3
           lda thisname
printthis
           jsr $ffd2
           inx
           lda thisname,x
           bne printthis
           jsr main
           lda #$37
           sta 1
           lda #$2f
           sta 0
           jsr $fd15
           jsr $fda3
           jsr print
           .text " - ok"
           .byte 13,0
           lda turboass
           beq loadnext
           jsr waitkey
           jmp $8000
           .bend
loadnext
           .block
           ldx #$f8
           txs
           lda nextname
           cmp #"-"
           bne notempty
           jmp $a474
notempty
           ldx #0
printnext
           jsr $ffd2
           inx
           lda nextname,x
           bne printnext
           lda #0
           sta $0a
           sta $b9
           stx $b7
           lda #<nextname
           sta $bb
           lda #>nextname
           sta $bc
           jmp $e16f
           .bend

;---------------------------------------
;print text which immediately follows
;the JSR and return to address after 0

print
           .block
           pla
           sta next+1
           pla
           sta next+2
           ldx #1
next
           lda $1111,x
           beq end
           jsr $ffd2
           inx
           bne next
end
           sec
           txa
           adc next+1
           sta return+1
           lda #0
           adc next+2
           sta return+2
return
           jmp $1111
           .bend

;---------------------------------------
;print hex byte

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
           bcc noletter
           adc #6
noletter
           jmp $ffd2
           .bend

;---------------------------------------
;wait until raster line is in border
;to prevent getting disturbed by DMAs

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

;---------------------------------------
;wait for a key and check for STOP

waitkey
           .block
           jsr $fd15
           jsr $fda3
           cli
wait
           jsr $ffe4
           beq wait
           cmp #3
           beq stop
           rts
stop
           lda turboass
           beq load
           jmp $8000
load
           jsr print
           .byte 13
           .text "break"
           .byte 13,0
           jmp loadnext
           .bend

;---------------------------------------

thisname   .null "loadth"
nextname   .null "cnto2"

main

;---------------------------------------
;check force load

           .block
           sei
           lda #0
           sta $dc04
           sta $dc05
           sta $dc06
           sta $dc07
           sta $dd04
           sta $dd05
           sta $dd06
           sta $dd07
           lda #%00010000
           sta $dc0e
           sta $dc0f
           sta $dd0e
           sta $dd0f
           lda $dc04
           ora $dc05
           ora $dc06
           ora $dc07
           ora $dd04
           ora $dd05
           ora $dd06
           ora $dd07
           beq ok1
           jsr print
           .byte 13
           .text "force load does "
           .text "not work"
           .byte 0
           jsr waitkey
ok1
           .bend

;---------------------------------------
;write tl while timers are stopped

           .block
           sei
           lda #255
           sta $dc04
           sta $dc06
           sta $dd04
           sta $dd06
           lda $dc04
           ora $dc05
           ora $dc06
           ora $dc07
           ora $dd04
           ora $dd05
           ora $dd06
           ora $dd07
           beq ok1
           jsr print
           .byte 13
           .text "writing tl may not "
           .text "load counter"
           .byte 0
           jsr waitkey
ok1
           .bend

;---------------------------------------
;write th while timers are stopped

           .block
           sei
           lda #255
           sta $dc05
           sta $dc07
           sta $dd05
           sta $dd07
           lda $dc04
           and $dc05
           and $dc06
           and $dc07
           and $dd04
           and $dd05
           and $dd06
           and $dd07
           cmp #255
           beq ok1
           jsr print
           .byte 13
           .text "writing th while "
           .text "stopped didn't load"
           .byte 0
           jsr waitkey
ok1
           .bend

;---------------------------------------
;write th while timers are running

           .block
           sei
           lda #%00100001
           sta $dc0e
           sta $dc0f
           sta $dd0e
           sta $dd0f
           lda #0
           sta $dc05
           sta $dc07
           sta $dd05
           sta $dd07
           lda $dc04
           and $dc05
           and $dc06
           and $dc07
           and $dd04
           and $dd05
           and $dd06
           and $dd07
           cmp #255
           beq ok1
           jsr print
           .byte 13
           .text "writing th while "
           .text "started may not load"
           .byte 0
           jsr waitkey
ok1
           .bend

;---------------------------------------

           rts


