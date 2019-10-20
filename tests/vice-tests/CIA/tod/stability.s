            *=$07ff
            !word $0801
            !word bend
            !word 10
            !byte $9e
            !text "2061", 0
bend:       !word 0
            jmp start
;-------------------------------------------------------------------------------

diffbuf = $0800         ; differences (16 bit)
timebuf = $0c00         ; measured timer values (32 bit)

linepos = $02
scrptr = $03

            * = $1000

alarmirq:
            pha
            txa
            pha
            tya
            pha

            ldx atime1+1
            stx $d020

            ; stop timer
            lda #%10000000
            sta $dc0e

            ; read timer values and save them
            lda $dc04           ;4
            ldx $dc05           ;4
            ldy $dc06           ;4
curroff1:   sta timebuf         ;4
            lda $dc07           ;4
curroff4:   sta timebuf+$0300   ;4

            ; start timer
    !if SYNC = 0 {
            lda #%10000001      ;2
    }
    !if SYNC > 0 {
            lda #%00000001      ;2
    }
            sta $dc0e           ;4
                                ;-> 30

timererrorcycles = 30

curroff2:   stx timebuf+$0100
curroff3:   sty timebuf+$0200

            inc curroff1+1
            inc curroff2+1
            inc curroff3+1
            inc curroff4+1
            bne skip
            lda #$a9
            sta cont
skip:
            ; increment BCD counter
            inc atime1+1
            ldx atime1+1
            cpx #10
            bne sk1
            ldx #0
            stx atime1+1

            inc atime2+1
            lda atime2+1
            cmp #10
            bne sk2
            lda #$10
            sta atime2+1
sk2:
            cmp #$1a
            bne sk3
            lda #$20
            sta atime2+1
sk3:

            cmp #$2a
            bne sk4
            lda #$a9
            sta cont
sk4:

sk1:
            ; set new alarm time
            lda #%11000001      ; set alarm
            sta $dc0f

            lda #$00
            sta $dc0b
            ;lda #$00
            sta $dc0a
atime2:     ldx #$00
            stx $dc09
atime1:     lda #$01
            sta $dc08

            stx $d020

            lda $dc0d

            pla
            tay
            pla
            tax
            pla
            rti

;-------------------------------------------------------------------------------
start:
            ; disable screen
            lda #$0b
            sta $d011
            
            jsr makescreen

            sei
            lda #$35
            sta $01
            lda #>alarmirq
            sta $ffff
            lda #<alarmirq
            sta $fffe
            lda #$7f
            sta $dc0d

            ; stop timer
    !if SYNC = 0 {
            lda #%10000000  ; PAL
    }
    !if SYNC > 0 {
            lda #%00000000  ; NTSC
    }
            sta $dc0e
            lda #%01000000
            sta $dc0f

            lda #$ff
            sta $dc04
            sta $dc05
            sta $dc06
            sta $dc07

            ; start timer
    !if SYNC = 0 {
            lda #%10010001  ; PAL
    }
    !if SYNC > 0 {
            lda #%00010001  ; NTSC
    }
            sta $dc0e
            lda #%01010001      ; set clock
            sta $dc0f


            lda #$00
            sta $dc0b
            ;lda #$00
            sta $dc0a
            ;lda #$00
            sta $dc09
            ;lda #$00
            sta $dc08

            lda #%11000001      ; set alarm
            sta $dc0f

            lda #$00
            sta $dc0b
            ;lda #$00
            sta $dc0a
            ;lda #$00
            sta $dc09
            sta atime2+1
            lda #$01
            sta $dc08
            sta atime1+1

            ; enable alarm irq
            lda #$84
            sta $dc0d

            lda $dc0d
            cli

            clc
cont:       bcc *       ; wait, 3 cycles latency

            lda #$90
            sta cont

            ; we are done measuring, now show the results

            ; stop CIA irq
            lda #$7f
            sta $dc0d
            ; enable screen
            lda #$1b
            sta $d011

            lda #0
            sta $d020

            jsr calcresults

            jsr showresults
wkey:
            lda $dc01
            cmp #$ef
            bne wkey

            jmp start

;-------------------------------------------------------------------------------
makescreen:
            ; clear screen
            ldx #0
lp:
            lda #$20
            sta $0400,x
            sta $0500,x
            sta $0600,x
            lda #$a0
            sta $0700,x
            lda #$01
            sta $da00,x
            lda #$00
            sta $db00,x
            inx
            bne lp
            
            ldx #3
vlp:
            lda videosystem,x
            eor #$80
            sta $0400+40*19+35,x
            dex
            bpl vlp
            
            ldx #3
vlp3:
            lda powerfreq,x
            eor #$80
            sta $0400+40*19+30,x
            dex
            bpl vlp3
            
            ldx #39
vlp2:
            lda descr,x
            eor #$80
            sta $0400+40*21,x
            lda space,x
            eor #$80
            sta $0400+40*23,x
            dex
            bpl vlp2
            
            lda #12
            sta $d020
            sta $d021
            rts
;-------------------------------------------------------------------------------
calcresults:
            ; 1s complement of timer values
            ldx #0
lp3:
            lda timebuf,x
            eor #$ff
            sta timebuf,x
            lda timebuf+$0100,x
            eor #$ff
            sta timebuf+$0100,x
            lda timebuf+$0200,x
            eor #$ff
            sta timebuf+$0200,x
            lda timebuf+$0300,x
            eor #$ff
            sta timebuf+$0300,x
            inx
            bne lp3

            ; note: the first measured value is considered invalid

            ; calculate differences
            ldx #1
            ldy #0
lp2:
            sec
            lda timebuf+$0000,x
            sbc timebuf+$0000,y
            sta diffbuf+$0000,x
            lda timebuf+$0100,x
            sbc timebuf+$0100,y
            sta diffbuf+$0100,x
            lda timebuf+$0200,x
            sbc timebuf+$0200,y
            sta diffbuf+$0200,x
            lda timebuf+$0300,x
            sbc timebuf+$0300,y
            sta diffbuf+$0300,x
            iny
            inx
            cpx #$ff
            bne lp2

            ; compensate the error introduced by stopping the timer in the irq
            ldx #1
lp8:
            clc
            lda diffbuf,x
            adc #timererrorcycles - 2 ; sub 2 to compensate for IRQ latency
            sta diffbuf,x
            lda diffbuf+$0100,x
            adc #0
            sta diffbuf+$0100,x
            lda diffbuf+$0200,x
            adc #0
            sta diffbuf+$0200,x
            lda diffbuf+$0300,x
            adc #0
            sta diffbuf+$0300,x
            inx
            cpx #$ff
            bne lp8

            ; note: the first and last calculated difference is invalid

            ; find min/max and average in diffbuff
            lda #$ff
            sta diffbuf+$0000
            sta diffbuf+$0100
            sta diffbuf+$0200
            sta diffbuf+$0300
            
            lda #0
            sta diffbuf+$00ff
            sta diffbuf+$01ff
            sta diffbuf+$02ff
            sta diffbuf+$03ff
            
            sta avgtime+0
            sta avgtime+1
            sta avgtime+2
            sta avgtime+3
            
            ldx #1
lp5:
            ; (diffbuf - currmin) ... C=0 if currmin > diffbuf
            sec
            lda diffbuf+$0000,x
            sbc diffbuf+$0000
            lda diffbuf+$0100,x
            sbc diffbuf+$0100
            lda diffbuf+$0200,x
            sbc diffbuf+$0200
            lda diffbuf+$0300,x
            sbc diffbuf+$0300

            bcs notmin

            lda diffbuf+$0000,x
            sta diffbuf+$0000
            lda diffbuf+$0100,x
            sta diffbuf+$0100
            lda diffbuf+$0200,x
            sta diffbuf+$0200
            lda diffbuf+$0300,x
            sta diffbuf+$0300
notmin:

            ; (diffbuf - currmax) ... C=0 if currmax > diffbuf
            sec
            lda diffbuf+$0000,x
            sbc diffbuf+$00ff
            lda diffbuf+$0100,x
            sbc diffbuf+$01ff
            lda diffbuf+$0200,x
            sbc diffbuf+$02ff
            lda diffbuf+$0300,x
            sbc diffbuf+$03ff

            bcc notmax

            lda diffbuf+$0000,x
            sta diffbuf+$00ff
            lda diffbuf+$0100,x
            sta diffbuf+$01ff
            lda diffbuf+$0200,x
            sta diffbuf+$02ff
            lda diffbuf+$0300,x
            sta diffbuf+$03ff
notmax:
            inx
            cpx #$ff
            bne lp5

            ; diff/range
            sec
            lda diffbuf+$00ff
            sbc diffbuf+$0000
            sta difftime+0
            lda diffbuf+$01ff
            sbc diffbuf+$0100
            sta difftime+1

            ldx #0
lp7:
            ; average
            clc
            lda avgtime+0
            adc diffbuf+$0000,x
            sta avgtime+0
            lda avgtime+1
            adc diffbuf+$0100,x
            sta avgtime+1
            lda avgtime+2
            adc diffbuf+$0200,x
            sta avgtime+2
            lda avgtime+3
            adc diffbuf+$0300,x
            sta avgtime+3

            inx
            bne lp7

            lda avgtime+1
            sta avgtime+0
            lda avgtime+2
            sta avgtime+1
            lda avgtime+3
            sta avgtime+2
            lda #0
            sta avgtime+3
            
            rts
            
;-------------------------------------------------------------------------------
showresults:
            ; show difference buffer
            ldx #0
lp4:
            lda diffbuf,x
            sta $0400,x
            sta $d800,x
            lda diffbuf+$0100,x
            sta $0400+$0100,x
            sta $d800+$0100,x
            inx
            bne lp4

            ; show distributen of values
            ldx #0
lp1:
            lda diffbuf,x
            tay
            lda #'*'
            sta $0600,y

            inx
            bne lp1

            ; min
            ldy diffbuf
            lda #'<'
            sta $0600,y

lp6:
            lda $0600,y
            cmp #' '
            bne *+4
            lda #'-'
            sta $0600,y
            iny
            cpy diffbuf+$00ff
            bne lp6
            
            ; max
            ldy diffbuf+$00ff
            lda #'>'
            sta $0600,y

            ; average
            ldy avgtime+0
            lda #'!' & $3f
            sta $0600,y

            ; ideal
            ldy idealtime+0
            lda #'I' & $3f
            sta $0600,y
            
            ; output some numbers :)
            lda #0
            sta linepos
            
            lda #>($400+40*22)
            sta scrptr+1
            lda #<($400+40*22)
            sta scrptr+0
            
            ; min
            lda diffbuf+$0300
            jsr hexout
            lda diffbuf+$0200
            jsr hexout
            lda diffbuf+$0100
            jsr hexout
            lda diffbuf+$0000
            jsr hexout

            inc linepos

            ; max
            lda diffbuf+$03ff
            jsr hexout
            lda diffbuf+$02ff
            jsr hexout
            lda diffbuf+$01ff
            jsr hexout
            lda diffbuf+$00ff
            jsr hexout

            inc linepos

            ; diff
            lda difftime+1
            jsr hexout
            lda difftime+0
            jsr hexout

            inc linepos

            ; average
            lda avgtime+3
            jsr hexout
            lda avgtime+2
            jsr hexout
            lda avgtime+1
            jsr hexout
            lda avgtime+0
            jsr hexout

            inc linepos

            ; ideal
            lda idealtime+3
            jsr hexout
            lda idealtime+2
            jsr hexout
            lda idealtime+1
            jsr hexout
            lda idealtime+0
            jsr hexout

            lda #0
            sta linepos
            lda #>($400+40*23)
            sta scrptr+1
            lda #<($400+40*23)
            sta scrptr+0
            
            ; min
            lda mintime + 3
            jsr hexout
            lda mintime + 2
            jsr hexout
            lda mintime + 1
            jsr hexout
            lda mintime + 0
            jsr hexout

            inc linepos

            ; max
            lda maxtime + 3
            jsr hexout
            lda maxtime + 2
            jsr hexout
            lda maxtime + 1
            jsr hexout
            lda maxtime + 0
            jsr hexout
            
            ; measured min - min
            ldx #5
            ldy #13
            lda diffbuf + $0000
            cmp mintime + 0
            lda diffbuf + $0100
            sbc mintime + 1
            lda diffbuf + $0200
            sbc mintime + 2
            lda diffbuf + $0300
            sbc mintime + 3
            bcs +
            ldy #10
            ldx #10
+
            sty $d800 + 40*23
            sty $d801 + 40*23
            sty $d802 + 40*23
            sty $d803 + 40*23
            sty $d804 + 40*23
            sty $d805 + 40*23
            sty $d806 + 40*23
            sty $d807 + 40*23
             
            ; measured max - max
            ldy #13
            lda maxtime + 0
            cmp diffbuf + $00ff
            lda maxtime + 1
            sbc diffbuf + $01ff
            lda maxtime + 2
            sbc diffbuf + $02ff
            lda maxtime + 3
            sbc diffbuf + $03ff
            bcs +
            ldy #10
            ldx #10
+
            sty $d809 + 40*23
            sty $d80a + 40*23
            sty $d80b + 40*23
            sty $d80c + 40*23
            sty $d80d + 40*23
            sty $d80e + 40*23
            sty $d80f + 40*23
            sty $d810 + 40*23

            ldy #0
            stx $d020
            cpx #5
            beq +
            ldy #$ff
+
            sty $d7ff
            rts

avgtime:
            !word 0
            !word 0
difftime:
            !word 0
;-------------------------------------------------------------------------------

hexout:
            pha
            lsr
            lsr
            lsr
            lsr
            tax
            lda hextab,x
            ldy linepos
            eor #$80
            sta (scrptr),y
            iny
            pla
            and #$0f
            tax
            lda hextab,x
            eor #$80
            sta (scrptr),y
            iny
            sty linepos
            rts

hextab:
            !byte $30,$31,$32,$33,$34,$35,$36,$37,$38,$39,$01,$02,$03,$04,$05,$06

videosystem:
    !if SYNC = 0 {
            !scr "pal "
    }
    !if SYNC = 1 {
            !scr "ntsc"
    }
            !scr "ntsc"
            !scr "paln"

powerfreq:
    !if SYNC = 0 {
            !scr "50hz"
    }
    !if SYNC > 0 {
            !scr "60hz"
    }

descr:
            !scr "min      max      diff average  ideal   "
space:
            !scr "                  space to measure again"

;
;timing:
;
; cycles per second:
; PAL       985248
; NTSC     1022730
; NTSCOLD  1022730
; PALN     1023440
;

!if SYNC = 0 {
    ; PAL
    CYCLESPERSEC = 985248
}
!if SYNC = 1 {
    ; NTSC
    CYCLESPERSEC = 1022730
}
!if SYNC = 2 {
    ; NTSCOLD
    CYCLESPERSEC = 1022730
}
!if SYNC = 3 {
    ; PALN
    CYCLESPERSEC = 1023440
}

; we measure time between 1/10th second ticks
CYCLESPER10THSEC = (CYCLESPERSEC / 10)

idealtime:
    !word CYCLESPER10THSEC & $ffff
    !word CYCLESPER10THSEC >> 16
    
; the maximum deviation should be less than 0,1Hz
CYCLESPER10THSECMIN = CYCLESPER10THSEC - (CYCLESPER10THSEC / 100)
CYCLESPER10THSECMAX = CYCLESPER10THSEC + (CYCLESPER10THSEC / 100)

mintime:
    !word CYCLESPER10THSECMIN & $ffff
    !word CYCLESPER10THSECMIN >> 16

maxtime:
    !word CYCLESPER10THSECMAX & $ffff
    !word CYCLESPER10THSECMAX >> 16


