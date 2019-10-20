;TOD-clock test #1b v0.1 (Cold Sewers)
;this one repeatedly starts tod at 0:00:00.0 with alarm-nmi enabled
;when either the alarm is triggerd or tod ran for 1 second it will print out
;whether an alarm-nmi occured or not.
;this version will set the clock to the power up state of 1:00:00.0 after every
;test that didn't trigger an alarm, to see if the weird behaviour of the 2nd pass
;can be repeated.

                * = $801
                .word eol,2192
                .byte $9e,$32,$30,$34,$39,0
eol             .word 0

                lda #0
                sta try         ;proper init for cleaner output after reset
                lda #20
                sta wrap
                ldx #160
                ldy #4
                stx row
                sty row+1
                lda #<text      ;instructions
                ldy #>text
                jsr $ab1e

start           lda #$7f        ;disable cia interrupts
                sta $dc0d
                sta $dd0d
                ldx #$ff        ;clear up stack mess
                txs
                ldx #<alarmnmi  ;set up nmi via ram vector
                ldy #>alarmnmi
                stx $fffa
                sty $fffb
                sei             ;just to be 100% safe of IRQs :)
                jsr gettod
                ldy #20         ;show state of tod before test-run
                jsr showtod
                lda #$35        ;roms off
                sta $01
bcol:           ldx #0
                stx $d020
                ldx #0
                stx $d021       ;set colors
                stx alarm       ;flag: 1=alarm occured, 0=clock ran up to 0:00:01.0
                lda #$84        ;enable alarm nmi
                sta $dd0d,x
                jsr settod      ;start tod at 0:00:00.0

main            jsr sync        ;repeat once per frame:
                jsr gettod      ;read out tod registers and copy to $fb-$fe
                ldy #0
                jsr showtod     ;and show them at first screenrow
                lda $fc
                beq main        ;until clock seconds = 1

                jsr resettod    ;if no alarm occured, restore time at powerup

update          sed             ;add 1 to number of tests
                lda try
                clc
                adc #1
                sta try
                cld
                lda row         ;print result in the next lower screenrow

update3         clc
                adc #40
                sta row
                sta $5a
                lda row+1
                adc #0
                sta row+1
                sta $5b
                dec wrap        ;if not at bottom of screen
                bne update2     ;then

                lda #20         ;else start at 1st output pos again
                sta wrap
                lda #4
                sta row+1
                lda #160
                bne update3

update2         ldy #0          ;column0
                lda try
                jsr hexout      ;print number of current test
                ldy #10
                ldx #50
                lda alarm       ;print alarm state (0=no alarm occured, 1=occured)
                beq *+4
                ldx #100        ;delay 1 or 2 seconds, depending on how long the
                jsr hexout      ;clock was running
wait            jsr sync
                dex
                bne wait

                lda try
                cmp #$7
                beq checkok
                jmp start
checkok:
                ldy #5
                ldx #0
checklp:
                lda $0400+(5*40),x
                cmp reftext,x
                beq iseq
                ldy #10
iseq:
                inx
                cpx #6*40
                bne checklp
                sty bcol+1
                sty $d020

                lda $d020
                and #$0f
                ldx #0 ; success
                cmp #5
                beq nofail
                ldx #$ff ; failure
nofail:
                stx $d7ff

                jmp start

reftext:
    .enc screen ;screen code mode
    .text "01        01                            "
    .text "02        00   expected:                "
    .text "03        01   alarm on first run,      "
    .text "04        00   no alarm on second run,  "
    .text "05        01   alarm on third run, etc. "
    .text "06        00                            "
    .enc none

;nmi will set show tod-state at alarm-time and set alarm flag:

alarmnmi        pha
                lda $dd0d
                and #$04
                beq nottod      ;restore key or other timers (should never happen)

doalarm         jsr gettod
                ldy #0
                jsr showtod
                lda #1
                sta alarm
                jmp update

nottod          pla
                rti


;set tod to 0:00:00.0

settod          lda $dd0f       ;writes to tod = time
                and #$7f
                sta $dd0f
                ldx #3

settod2         lda #0
                sta $dd08,x
                dex
                bpl settod2

                rts

;try to get back to 1:00:00.0, like when the machine is powered up

resettod        lda #$7f        ;ensure reset doesn't accidentally trigger nmi
                sta $dd0d
                and $dd0f
                sta $dd0f
                ldy poweron+3
                ldx #3

resettod2       lda poweron,x
                sta $dd08,x
                sty $dd0b
                dex
                bpl resettod2
                rts

;read out tod and copy to zp:

gettod          ldx #3
                lda $dd08,x
                sta $fb,x
                dex
                bpl gettod+2
                rts


;print copied tod values on screen:

showtod         lda $5b
                pha
                lda $5a
                pha
                lda #0
                sta $5a
                lda #4
                sta $5b
                ldx #3

showtod2        lda $fb,x       ;show hours:minutes:seconds
                jsr hexout
                lda #$3a        ;":"-screencode
                sta ($5a),y
                iny
                dex
                bne showtod2

                lda $fb         ;.1 seconds register is only a nibble
                jsr nibout
                pla
                sta $5a
                pla
                sta $5b
                rts

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
                sta ($5a),y
                iny
                rts


;wait for raster beam #$100

sync            lda #$ff
                cmp $d012
                bne *-3
                cmp $d012
                beq *-3
                rts

try             .byte 0
alarm           .byte 0
row             .byte 160,4
wrap            .byte 20
column          .byte 0                         ;0 = sec, 1 = min, 2 = hrs, .1 sec unused
coffset         .byte 9,6,3,0                   ; display offset for .1sec, sec, min, hrs
maxval          .byte $09,$59,$59,$24           ; .1sec, sec, min, hrs in bcd!
dfttime         .byte $00,$00,$00,$00           ; dito
poweron         .byte $00,$00,$00,$01           ;ToD-time after power-up

text            .byte 147,13,13,5
                      ;1234567890123456789012345678901234567890
                .text "continous alarm test:",13,13
                .text "test#:    alarm (1=occured):",13,13
                .text 29,29,29,29,29,29,29,29,29,29,29,29,29,29,29
                .text "expected:",13
                .text 29,29,29,29,29,29,29,29,29,29,29,29,29,29,29
                .text "alarm on first run,",13
                .text 29,29,29,29,29,29,29,29,29,29,29,29,29,29,29
                .text "no alarm on second run,",13
                .text 29,29,29,29,29,29,29,29,29,29,29,29,29,29,29
                .text "alarm on third run, etc.",13
                .byte 31,19,0
                .byte 0

