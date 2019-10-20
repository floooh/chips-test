          *=$0801             ; basic start
          !WORD +
          !WORD 10
          !BYTE $9E
          !TEXT "2061"
          !BYTE 0
+         !WORD 0

          lda #$93
          jsr $ffd2

          ldx #0
tlp
          lda text,x
          beq tsk
          sta $0400+(24*40),x
          inx
          bne tlp
tsk

         sei

          LDA #5
          STA $D020           ; test ok: border = green
loop:
-         lda $d011
          bmi -
-         lda $d011
          bpl -

          inc $d020
         ; first loop
         ldx #0
-
         lda #0
         sta $dc0e      ; CIA1 TimerA stop
         stx $dc04      ; CIA1 TimerA Lo
         sta $dc05      ; CIA1 TimerA Hi
         ; TimerA value = 0000
         lda #$7f
         sta $dc0d      ; disable all CIA1 IRQs
         bit $dc0d      ; ACK pending CIA1 IRQs
          !if (ONESHOT=1) {
         lda #%00011001
          } else {
         lda #%00010001
          }
         sta $dc0e      ; (4) CIA1 TimerA start, oneshot, force load
         lda #$81       ; (2)
         sta $dc0d      ; (4) enable CIA1 TimerA IRQ
         ; IRQs should happen now. if we would read from $dc0d, we would read $81
         lda #$7f       ; (2)
         sta $dc0d      ; (4) disable all CIA1 IRQs
         ; although IMR was cleared (and no more IRQs can occur), the value that
         ; will get read from $dc0d still has the respective bits set, it must
         ; be read once to ACK pending IRQs
         lda $dc0d      ; read IRQ mask and ACK IRQs
         sta $0400+(0*40),x

          inx
          cpx #40
          bne -
          dec $d020

-         lda $d011
          bmi -
-         lda $d011
          bpl -


          ; second loop
          inc $d020
         ldx #0
-
         lda #0
         sta $dc0e      ; CIA1 TimerA stop
         stx $dc04      ; CIA1 TimerA Lo
         sta $dc05      ; CIA1 TimerA Hi
         ; TimerA value = 0000
         lda #$7f
         sta $dc0d      ; disable all CIA1 IRQs
         bit $dc0d      ; ACK pending CIA1 IRQs
          !if (ONESHOT=1) {
         lda #%00011001
          } else {
         lda #%00010001
          }
         sta $dc0e      ; (4) CIA1 TimerA start, oneshot, force load
         lda #$81       ; (2)
         sta $dc0d      ; (4) enable CIA1 TimerA IRQ
         ; IRQs should happen now. if we would read from $dc0d, we would read $81
         nop
         lda #$7f
         sta $dc0d      ; disable all CIA1 IRQs
         ; although IMR was cleared (and no more IRQs can occur), the value that
         ; will get read from $dc0d still has the respective bits set, it must
         ; be read once to ACK pending IRQs
         lda $dc0d      ; read IRQ mask and ACK IRQs
         sta $0400+(8*40),x

          inx
          cpx #40
          bne -
          dec $d020

-         lda $d011
          bmi -
-         lda $d011
          bpl -

          ; third loop
          inc $d020
         ldx #0
-
         lda #0
         sta $dc0e      ; CIA1 TimerA stop
         stx $dc04      ; CIA1 TimerA Lo
         sta $dc05      ; CIA1 TimerA Hi
         ; TimerA value = 0000
         lda #$7f
         sta $dc0d      ; disable all CIA1 IRQs
         bit $dc0d      ; ACK pending CIA1 IRQs
          !if (ONESHOT=1) {
         lda #%00011001
          } else {
         lda #%00010001
          }
         sta $dc0e      ; (4) CIA1 TimerA start, oneshot, force load
         nop            ; (2)
         lda #$81       ; (2)
         sta $dc0d      ; (4) enable CIA1 TimerA IRQ
         ; IRQs should happen now. if we would read from $dc0d, we would read $81
         nop            ; (2)
         lda #$7f       ; (2)
         sta $dc0d      ; (4) disable all CIA1 IRQs
         ; although IMR was cleared (and no more IRQs can occur), the value that
         ; will get read from $dc0d still has the respective bits set, it must
         ; be read once to ACK pending IRQs
         lda $dc0d      ; read IRQ mask and ACK IRQs
         sta $0400+(16*40),x

          inx
          cpx #40
          bne -
         dec $d020


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
          cpx #40
          bne lp1

          ldx #0
lp2:
          ldy #5
          lda $0400+(8*40),x
          cmp reference+(8*40),x
          beq sk2
          ldy #10
          sty $d020
sk2:
          tya
          sta $d800+(8*40),x
          inx
          cpx #40
          bne lp2

          ldx #0
lp3:
          ldy #5
          lda $0400+(16*40),x
          cmp reference+(16*40),x
          beq sk3
          ldy #10
          sty $d020
sk3:
          tya
          sta $d800+(16*40),x
          inx
          cpx #40
          bne lp3

    lda $d020
    and #$0f
    ldx #0 ; success
    cmp #10
    bne nofail
    ldx #$ff ; failure
nofail:
    stx $d7ff

          JMP loop

text:
        !scr "cia icr test2 - "
          !if (ONESHOT=1) {
            !scr "oneshot "
          } else {
            !scr "continues "
          }
        !byte 0

reference:
            !if (ONESHOT=1) {
                !binary "icr2-oneshot.ref",,2
            } else {
                !binary "icr2-oneshot.ref",,2
            }
