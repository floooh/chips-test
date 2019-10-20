;Clock-Test #2c: 50Hz counter sync test (PAL only). Source for 64TASS (Cold Sewers)

;find out if the 50/60Hz counter is still reset at clock-restart
;when ommitting the writes to seconds and minutes registers.
;(writing to .1 secs can't be ommitted, else the clock would just stop)

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
                sta $dd0b       ;at raster #$100, Tod := 0:xx:xx.0
                sta $dd08
                ora $dd0b
                ora $dd08
                beq latchgood   ;if time read = time written

                asl color       ;else signal latch error
                lda #0

latchgood       sta frame       ;reset frame counter
                jsr sync        ;wait for 3 frames
                jsr sync        ;what should be roughly 2 frames
                jsr sync        ;before the .1 secs get inc'ed again

                sta $dd0b       ;reset Tod to 0:xx:xx.0
                sta $dd08
                ora $dd0b
                ora $dd08
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
                .text "hzsync2",13,13
                      ;1234567890123456789012345678901234567890
                .text "tod input frequency counter test:",13
                .text "1-3 = free running",13
                .text "4-6 = synced to hr/.1sec writes",13,13
                .text "expected: no reset (5s)",0