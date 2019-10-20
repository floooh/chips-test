;Clock-Test #2: 50Hz counter sync test (PAL only). Source for 64TASS (Cold Sewers)


;find out if the 50/60Hz counter is running freely
;or somehow synchronized with writes to TOD-time:
;in the latter case .1secs should change roughly 5 frames after last rewrite
;in the former case .1secs should change about 2 frames after last rewrite

;slight variations of +/-1 frames can be expected as neither frame rate nor ToD input
;are exactly 50 Hz.

;note: when RUNning the test multiple times, VICE 2.4 (r28019) often shows quite
;erratic results on the very first test pass.


frame           = $02           ;frame counter

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
                sty $d020

                ldx #$00        ;init test pass counter

set             lda $d020
                pha
set2
                lda #$00
                jsr sync
                sta $d020       ;black border to indicate write/re-read phase of test pass
                sta $dd0b       ;at raster #$100, Tod := 0:00:00.0
                sta $dd0a
                sta $dd09
                sta $dd08
                ora $dd0b
                ora $dd0a
                ora $dd09
                ora $dd08
                bne set2         ;try again if time read != time written

                sta frame       ;reset frame counter
                dec $d020
                jsr sync        ;wait for 3 frames
                jsr sync        ;what should be roughly 2 frames
                jsr sync        ;before the .1 secs get inc'ed again

                sta $dd0b       ;reset Tod to 0:00:00.0
                sta $dd0a
                sta $dd09
                sta $dd08
                ora $dd0b
                ora $dd0a
                ora $dd09
                ora $dd08
                bne set2        ;repeat the whole procedure if time read != time written

                pla
                sta $d020

wait4change     inc frame       ;repeat frame count++
                jsr sync        ;wait another frame
                lda $dd08
                beq wait4change ;until .1 secs change

                lda frame       ;current frame #
                ora #$30        ;+ offset screencode "0"
                sta $400,x      ;put on screen in white

                ldy #5
                cmp #$35
                beq ok1
                cmp #$36
                beq ok1
                ldy #10
                sty $d020
ok1
                tya
                sta $d800,x

                inx             ;test 256 times in a row
                bne set

                cli             ;reenable timer1 irqs
                lda #$81
                sta $dc0d

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
                .text "hzsync0",13,13
                      ;1234567890123456789012345678901234567890
                .text "tod input frequency counter test:",13
                .text "1-3 = free running",13
                .text "4-6 = synced to tod writes",13,13
                .text "expected: all synced (some 5s, some 6s)"
                .byte 0
