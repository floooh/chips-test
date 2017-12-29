;---------------------------------------
;flipos.asm - this file is part
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

thisname   .null "flipos"
nextname   .null "oneshot"

main

;---------------------------------------
;set oneshot at underflow-1

           .block
           sei
           lda #0
           sta $dc0e
           sta $dc0f
           lda #$7f
           sta $dc0d
           bit $dc0d
           lda #3
           sta $dc04
           lda #0
           sta $dc05
           lda #%00100001
           sta $dc0e
           lda #255
           sta $dc04
           sta $dc05
           lda #%00000000
           sta $dc0e
           jsr waitborder
           lda #%00000001
           ldx #%00001001
           sta $dc0e
           stx $dc0e
           lda $dc04
           and $dc05
           cmp #255
           beq ok1
           jsr print
           .byte 13
           .text "set oneshot at t-1 "
           .text "did not stop counter"
           .byte 0
           jsr waitkey
ok1
           .bend

;---------------------------------------
;set oneshot at underflow

           .block
           sei
           lda #0
           sta $dc0e
           sta $dc0f
           lda #$7f
           sta $dc0d
           bit $dc0d
           lda #2
           sta $dc04
           lda #0
           sta $dc05
           lda #%00100001
           sta $dc0e
           lda #255
           sta $dc04
           sta $dc05
           lda #%00000000
           sta $dc0e
           jsr waitborder
           lda #%00000001
           ldx #%00001001
           sta $dc0e
           stx $dc0e
           lda $dc04
           and $dc05
           sta 16384
           cmp #252
           beq ok1
           jsr print
           .byte 13
           .text "set oneshot at t "
           .text "may not stop counter"
           .byte 0
           jsr waitkey
ok1
           .bend

;---------------------------------------
;clear oneshot at underflow-1

           .block
           sei
           lda #0
           sta $dc0e
           sta $dc0f
           lda #$7f
           sta $dc0d
           bit $dc0d
           lda #3
           sta $dc04
           lda #0
           sta $dc05
           lda #%00100001
           sta $dc0e
           lda #255
           sta $dc04
           sta $dc05
           lda #%00000000
           sta $dc0e
           jsr waitborder
           lda #%00001001
           ldx #%00000001
           sta $dc0e
           stx $dc0e
           lda $dc04
           and $dc05
           cmp #255
           beq ok1
           jsr print
           .byte 13
           .text "clr oneshot at t-1 "
           .text "did not stop counter"
           .byte 0
           jsr waitkey
ok1
           .bend

;---------------------------------------
;clear oneshot at underflow-2

           .block
           sei
           lda #0
           sta $dc0e
           sta $dc0f
           lda #$7f
           sta $dc0d
           bit $dc0d
           lda #4
           sta $dc04
           lda #0
           sta $dc05
           lda #%00100001
           sta $dc0e
           lda #255
           sta $dc04
           sta $dc05
           lda #%00000000
           sta $dc0e
           jsr waitborder
           lda #%00001001
           ldx #%00000001
           sta $dc0e
           stx $dc0e
           lda $dc04
           and $dc05
           cmp #254
           beq ok1
           jsr print
           .byte 13
           .text "clr oneshot at t-2 "
           .text "may not stop counter"
           .byte 0
           jsr waitkey
ok1
           .bend

;---------------------------------------

           rts


