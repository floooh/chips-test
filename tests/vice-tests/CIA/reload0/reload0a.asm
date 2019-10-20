
        *=$0801             ; basic start
        !WORD +
        !WORD 10
        !BYTE $9E
        !TEXT "2061"
        !BYTE 0
+       !WORD 0

        jmp main


waitborder
;           lda $d011
;           bmi ok
;wait
;           lda $d012
;           cmp #30
;           bcs wait
;ok
wait
           lda $d011
           bpl wait
           rts

cnt        !byte 0

ieindex    !byte 0
beindex    !byte 0
betab      !byte $00,$01,$08,$09 ; before dc0e (wraps into ietab, len=8!)
           !byte $10,$11,$18,$19

ietab      !byte $10,$11,$18,$19 ; init dc0e
           !byte 0

i4         !byte 0 ; init dc04
ie         !byte 0 ; init dc0e
b4         !byte 0 ; before dc04
be         !byte 0 ; before dc0e
a4         !byte 0 ; after dc04
ad         !byte 0 ; after dc0d
ae         !byte 0 ; after dc0e

main
           ldx #0
-
           lda #1
           sta $d800,x
           sta $d900,x
           sta $da00,x
           sta $db00,x
           lda #$20
           sta $0400,x
           sta $0500,x
           sta $0600,x
           sta $0700,x
           inx
           bne -

           lda #0
           sta i4
           lda #4
           sta b4
           lda #0
           sta ieindex
           sta beindex
loop
           sei
           lda #$7f
           sta $dc0d
           lda #$81
           sta $dc0d
           jsr waitborder

           lda #0
           sta $dc0e
           sta $dc05

           ldx ieindex
           lda ietab,x
           sta ie

           ldx beindex
           ldy betab,x
           sty be

           ; write values "init"
           ldx i4
           stx $dc04    ; i4
           ldx #$10     ;      force load, but do not start
           stx $dc0e
           bit $dc0d    ; ack
           sta $dc0e    ; ie

           ; write values "before"
           lda b4
           sta $dc04    ; b4
           sty $dc0e    ; be

           ; get values "after"
           lda $dc04    ; a4
           ldx $dc0d    ; ad
           ldy $dc0e    ; ae
           sta a4
           stx ad
           sty ae

           jsr $fda3    ; initialize I/O
           cli

; ------------------------------------------------------------------------------

compare
           ldx cnt

           lda a4
           sta $0400,x
           lda ad
           sta $0400+(7*40),x
           lda ae
           sta $0400+(14*40),x

           inx
           stx cnt

           inc beindex
           lda beindex
           cmp #8
           bcc jmploop
           lda #0
           sta beindex

           inc ieindex
           lda ieindex
           cmp #5
           bcc jmploop
           lda #0
           sta ieindex

;           jmp *

           dec b4
           lda b4
           cmp #$fe
           bne jmploop
;           lda #20
;           sta b4
           jmp finish

;           dec i4
;           bpl jmploop
;           jmp finish
jmploop
           jmp loop


finish
          ldy #5
          sty $d020

          ldx #0
lp1:
          ldy #5
          lda $0400,x
          cmp reference,x
          beq sk1
          ldy #10
          sty $d020
sk1:
          tya
          sta $d800,x
          inx
          cpx #40*6
          bne lp1

          ldx #0
lp2:
          ldy #5
          lda $0400+(7*40),x
          cmp reference+(7*40),x
          beq sk2
          ldy #10
          sty $d020
sk2:
          tya
          sta $d800+(7*40),x
          inx
          cpx #40*6
          bne lp2

          ldx #0
lp3:
          ldy #5
          lda $0400+(14*40),x
          cmp reference+(14*40),x
          beq sk3
          ldy #10
          sty $d020
sk3:
          tya
          sta $d800+(14*40),x
          inx
          cpx #40*6
          bne lp3

    lda $d020
    and #$0f
    ldx #0 ; success
    cmp #10
    bne nofail
    ldx #$ff ; failure
nofail:
    stx $d7ff

           jmp *

reference:
           !binary "reload0a.ref",,2

