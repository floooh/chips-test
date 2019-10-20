;Clock-Test #2f: 50Hz counter sync test (PAL only). Source for 64TASS (Cold Sewers)

;test the 50Hz counter to find out
;-if it is reset by write to hour register, but still running, or
;-if it is reset and stopped until the clock is restarted

;slight code improvement: first result should be valid now

frame           = $02           ;frame counter
color           = $fb

* = $801
                .word eol,2192
                .byte $9e,$32,$30,$34,$39,0
eol             .word 0

                lda #<text      ;print what this test is all about
                ldy #>text
                jsr $ab1e

                lda #$7f        ;disable any CIA interrupts
                sta $dc0d
                sta $dd0d

                lda #$80
                sta $dd0e       ;input freq = 50Hzish
                lda #$00
                sta $dd0f       ;writes = ToD
                sei             ;just for safety's sake

                ldy #5
                sty bcol+1

                ldx #$00        ;init test pass counter

again           stx $dd08       ;to stabilize 1st pass: start clock
                cpx $dd08
                bne again       ;in case of latch bug
                cpx $dd08       ;else just wait on first .1 tick
                beq *-3

set             lda #$01
                sta color
                lda #$00
                jsr sync
                sta $dd0b       ;at raster #$100, stop Tod at 0:xx:xx.x
                sta $dd08
                sta $dd0b
                ora $dd0b
                ora $dd08
                beq latchgood   ;if time read = time written

                asl color       ;else signal latch error
                lda #0

latchgood       sta frame       ;reset frame counter
                jsr sync        ;wait for 3 frames, clock still stopped
                jsr sync
                jsr sync

                sta $dd08       ;start Tod at 0:xx:xx.0
                lda $dd08
                beq wait4change ;if time read = time written

                asl color       ;else signal latch error

wait4change     inc frame       ;repeat frame count++
                jsr sync        ;wait another frame
                lda $dd08
                beq wait4change ;until .1 secs change

                lda frame       ;current frame #
                ora #$30        ;+ offset screencode "0"
                sta $400,x      ;put on screen:
                sta $400+(18*40),x      ;put on screen:

                ldy #5
                cmp #$35
                beq ok1
                cmp #$36
                beq ok1
                ldy #10
                sty bcol+1
ok1
                tya
                sta $d800+(18*40),x

                lda color       ;white if valid result, red if latch bug struck
                sta $d800,x

                inx             ;test 256 times in a row
                bne set

                cli             ;reenable timer1 irqs
                lda #$81
                sta $dc0d

bcol:           lda #0
                sta $d020

                lda $d020
                and #$0f
                ldx #0 ; success
                cmp #5
                beq nofail
                ldx #$ff ; failure
nofail:
                stx $d7ff

                jmp *

sync            ldy #$ff        ;wait til raster #$100
                cpy $d012
                bne *-3
                cpy $d012
                beq *-3
                rts

text            .byte 147,13,13,13,13,13,13,13
                      ;1234567890123456789012345678901234567890
                .text "hzsync5",13,13
                .text "tod input frequency counter test:",13
                .text "1-3 = reset on hr write, running",13
                .text "4-6 = reset&stopped or reset by .1 secs",13,13
                .text "expected: counter is either not running",13
                .text "while clock is stopped or reset when",13
                .text "clock is restarted. (5s)"
                .byte 0