
zpage = $02

viabase = $9110 ; pb7 goes to userport


charsperline = 22

!if (expanded=1) {
basicstart = $1201      ; expanded
screenmem = $1000
colormem = $9400
} else {
basicstart = $1001      ; unexpanded
screenmem = $1e00
colormem = $9600
}

                * = basicstart
                !word basicstart + $0c
                !byte $0a, $00
                !byte $9e
!if (expanded=1) {
                !byte $34, $36, $32, $32
} else {
                !byte $34, $31, $31, $30
}
                !byte 0,0,0

                * = basicstart + $0d
start:
                sei
                ldx #$ff
                txs

                lda #0
                sta $900f
                jsr initscreen

            lda #$00    ; ACR start
            sta zpage+0
            jsr dotest2
            lda #$40    ; ACR start
            sta zpage+0
            jsr dotest2
            lda #$80    ; ACR start
            sta zpage+0
            jsr dotest2
            lda #$c0    ; ACR start
            sta zpage+0
            jsr dotest2

                jsr comparescreen

                jmp *

dotest2:
            lda #$00    ; ACR end
            sta zpage+1
            jsr dotest1

            lda #$40    ; ACR end
            sta zpage+1
            jsr dotest1

            lda #$80    ; ACR end
            sta zpage+1
            jsr dotest1

            lda #$c0    ; ACR end
            sta zpage+1
            jsr dotest1

            clc
            lda scrp1+1
            adc #2+(charsperline*2)
            sta scrp1+1
            lda scrp1+2
            adc #0
            sta scrp1+2

            clc
            lda scrp2+1
            adc #2+(charsperline*2)
            sta scrp2+1
            lda scrp2+2
            adc #0
            sta scrp2+2

            rts

dotest1:
            lda #$00    ; DATA PB
            sta zpage+2
            jsr dotest

            ldx #3
.scrlp1:
            lda zpage + 10,x
            lsr
            lsr
            lsr
            lsr
            lsr
            lsr
scrp1:      sta screenmem + (charsperline * 0),x
            dex
            bpl .scrlp1

            clc
            lda scrp1+1
            adc #5
            sta scrp1+1
            lda scrp1+2
            adc #0
            sta scrp1+2

            lda #$ff    ; DATA PB
            sta zpage+2
            jsr dotest

            ldx #3
.scrlp2:
            lda zpage + 10,x
            lsr
            lsr
            lsr
            lsr
            lsr
            lsr
scrp2:      sta screenmem + (charsperline * 1),x
            dex
            bpl .scrlp2

            clc
            lda scrp2+1
            adc #5
            sta scrp2+1
            lda scrp2+2
            adc #0
            sta scrp2+2
            rts


dotest:
            ; clear all registers twice to make sure they are all 0
            jsr resetvia
            jsr resetvia

            lda #$ff            ; DDR PB = output
            sta viabase + 2

            ; All tests should run once with PB data to 00 and once with PB data to FF.
            lda zpage + 2                 ; DATA PB
            sta viabase + 0

; Set ACR(reg B) bits 7 and 6 to certain value.
; Read value of PB.
; Start timer with write 0x01 to timer-counter-H (reg 5).
; Read value of PB again.
; Change ACR/reg B to new test pattern.
; Read PB again.
; Wait until timer overflows
; Read value of PB again

            lda zpage + 0   ; first testpattern
            sta viabase + $b    ; ACR

            lda viabase + 0     ; PB
            sta zpage + 10

            lda #1
            sta viabase + 5     ; timer 1 H

            lda viabase + 0     ; PB
            sta zpage + 11

            lda zpage + 1    ; second testpattern
            sta viabase + $b    ; ACR

            lda viabase + 0     ; PB
            sta zpage + 12

            ; wait until timer overflows
            !for n, 128 {
                nop
            }

            lda viabase + 0     ; PB
            sta zpage + 13

            rts
resetvia:
            lda #0
            ldx #$0f
.lp1
            sta viabase,x
            dex
            bpl .lp1
            rts

initscreen:
                ldx #0
lp1:
                lda #32
                sta screenmem,x
                sta screenmem+$100,x
                lda #1
                sta colormem,x
                sta colormem+$100,x
                inx
                bne lp1
                rts

comparescreen:
                lda #5 ; green
                sta failed+1

                ldx #0
lp2:
                !for cnt, 2 {
                ldy #5 ; green
                lda screenmem+((cnt-1)*$100),x
                cmp refdata+((cnt-1)*$100),x
                beq +
                ldy #2 ; red
                sty failed+1
+
                tya
                sta colormem+((cnt-1)*$100),x
                }

                inx
                bne lp2
                
failed:         lda #0
                sta $900f
                
                ; store value to "debug cart"
                ldy #0 ; success
                cmp #5 ; green
                beq +
                ldy #$ff ; failure
+
                sty $910f

                rts

refdata:
                !binary "dump.bin", $200, 2
