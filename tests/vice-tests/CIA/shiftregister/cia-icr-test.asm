          *=$0801             ; basic start
          !WORD +
          !WORD 10
          !BYTE $9E
          !TEXT "2061"
          !BYTE 0
+         !WORD 0

          lda #$93
          jsr $ffd2

          sei
          ; make sure screen is disabled, so timing is not disturbed by badlines
          lda #$0b
          sta $d011
          lda #$f0
-         cmp $d012
          bne -

          ldx #0
tlp
          lda text,x
          beq tsk
          sta $0400+(24*40),x
          inx
          bne tlp
tsk

          lda #0
          STA $DC0E           ; stop CIA Timer A
          lda #$7f            ; clear all interrupt flags
          sta $dc0d
          LDA $DC0D           ; read and clear the "interrupt control register"

          lda #$81            ; enable timer A interrupts
          sta $dc0d

          LDA #0
          TAX                 ; Counter X = 0
          STA $DC05

          LDA #1
          STA $DC04           ; Timer A value = 1

          LDA #2
          STA $D020           ; border = red

          !if (ONESHOT=1) {
          LDA #%11011001      ; force load Timer A; Run Mode: One-Shot
          STA $DC0E           ; shift register: SP is output; TOD=50Hz 
                              ; set mode and start Timer A
          } else {
          lda #%11010001
          sta $dc0e
          }
          
loop:
          !if (ONESHOT=1) {
          LDA #%11011001      ; force load Timer A, Run Mode: One-Shot
          STA $DC0E           ; shift register: SP is output; TOD=50Hz 
                              ; start Timer A
          }

-         LDA $DC0D           ; read and clear the "interrupt control register"
          sta $0400+(2*40),x
          AND #1              ; Interrupt from Timer A?
          BEQ -               ; no, then try again

          lda $dc0d           ; restore value from "interrupt control register"
          sta $0400+(10*40),x

          inx
          bne loop

OK:       LDA #5
          STA $D020           ; test ok: border = green

          lda #$7f
          sta $dc0d
          lda $dc0d

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
          bne lp1

          ldx #0
lp2:
          ldy #5
          lda $0400+$100,x
          cmp reference+$100,x
          beq sk2
          ldy #10
          sty $d020
sk2:
          tya
          sta $d800+$100,x
          inx
          bne lp2

          ldx #0
lp3:
          ldy #5
          lda $0400+$200,x
          cmp reference+$200,x
          beq sk3
          ldy #10
          sty $d020
sk3:
          tya
          sta $d800+$200,x
          inx
          bne lp3

          lda #$1b
          sta $d011

    lda $d020
    and #$0f
    ldx #0 ; success
    cmp #10
    bne nofail
    ldx #$ff ; failure
nofail:
    stx $d7ff

          JMP *

text:
        !scr "cia icr test - "
          !if (ONESHOT=1) {
            !scr "oneshot "
          } else {
            !scr "continues "
          }
          !if (CIA=1) {
            !scr "(new cia)"
          } else {
            !scr "(old cia)"
          }
        !byte 0

reference:
          !if (CIA=1) {
            !if (ONESHOT=1) {
                !binary "icr-oneshot-new.ref",,2
            } else {
                !binary "icr-continues-new.ref",,2
            }
          } else {
            !if (ONESHOT=1) {
                !binary "icr-oneshot-old.ref",,2
            } else {
                !binary "icr-continues-old.ref",,2
            }
          }
