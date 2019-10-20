;write to ToD like Slurpy, Source for 64Tass
;this version writes just the alarm time hour, like Slurpycode does.

;apparently, setting tod-time = tod-alarm-time will trigger an nmi,
;but setting tod-alarm-time = tod-time won't. Well done, MOS tech! :P

passes          = $fb
alarms          = $fd

* = $801
                .word eol,2192
                .byte $9e,$32,$30,$34,$39,0
eol             .word 0

                jsr alarm0      ;start with sys2064 to skip this!

                lda #<text      ;for better understanding
                ldy #>text
                jsr $ab1e

start           lda #$7f
                sta $dd0d
                ldx #$ff        ;reset stackptr
                txs
                sei
                ldx #<nmi       ;set up nmi-vector in ram
                ldy #>nmi
                stx $fffa
                sty $fffb
                lda #$35        ;bank out basic and kernal
                sta $01

                lda #0
                sta passes
                sta passes+1
                sta alarms
                sta alarms+1

                lda #$80        ;50Hz input
                sta $dd0e
                lda #$84        ;alarm nmi on
                sta $dd0d
                lda $dd0d

main            ldx #50
                ldy #$f0
                cpy $d012
                bne *-3
                dex
                bne main+2      ;to slow down to about 1 test per second

                jsr time0alarm0 ;first set time to 0, then alarm immediately after
                inc passes      ;count passes
                bne *+4
                inc passes+1

                ldx #10
                lda passes+1    ;print pass #
                jsr hexout
                lda passes
                jsr hexout

                ldx #10+40
                lda alarms+1
                jsr hexout
                lda alarms
                jsr hexout

                lda passes
                cmp #$40
                beq checkok
                jmp main

checkok:
                ldy #2
                lda alarms
                cmp passes
                lda alarms+1
                sbc passes+1
                bcc noteq
                ldy #5
noteq:
                sty $d020

                lda $d020
                and #$0f
                ldx #0 ; success
                cmp #5
                beq nofail
                ldx #$ff ; failure
nofail:
                stx $d7ff

                jmp main

time0alarm0     ldx #$00        ;write to tod time
                stx $dd0f
                stx $dd0b       ;hrs = 0
                stx $dd0a       ;min = 0
                stx $dd09       ;sec = 0
                stx $dd08       ;.1s = 0
                lda #$80        ;write to tod alarm
                sta $dd0f
                stx $dd0b       ;hrs = 0
                rts

;count the various nmis:

nmi             pha
                lda $dd0d
                and #$04
                beq nottod

                inc alarms
                bne *+4
                inc alarms+1

                pla
                rti

nottod          jmp start       ;restore to retest

alarm0          lda #$7f
                sta $dd0d
                lda #$80
                sta $dd0f       ;write alarm
                lda #$00
                sta $dd0b       ;set alarm to 0:00:00.0
                sta $dd0a
                sta $dd09
                sta $dd08
                lda #$00
                sta $dd0f       ;write time
                lda $dd0d       ;can't hurt
                rts

text            .text 147,"# tests:  xxxx",13
                .text "# alarms: xxxx",13,13
                .text "more alarms than test passes:",13
                .text "there's something wrong!",0

hexout          pha
                lsr
                lsr
                lsr
                lsr
                jsr nibout
                pla

nibout          and #$0f
                ora #$30         ;+screencode "0"
                cmp #$3a         ;if > "9"
                bcc *+4
                sbc #$39         ;then use screencodes for "a"-"f"
                sta $400,x
                inx
                rts

