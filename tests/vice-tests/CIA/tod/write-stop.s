
; "TOD is automatically stopped whenever a write to the Hours register 
;  occurs. The clock will not start again until after a write to the 10ths of 
;  seconds register."

;-------------------------------------------------------------------------------
            *=$07ff
            !word $0801
            !word bend
            !word 10
            !byte $9e
            !text "2061", 0
bend:       !word 0
;-------------------------------------------------------------------------------
            jmp start

scrptr = $02
linenr = $04
loopcnt = $06

waitframes:
-
            lda #$f0
            cmp $d012
            bne *-3
            cmp $d012
            beq *-3
            dex
            bne -
            rts

start:
            sei

            lda #$5
            sta bcol+1

            ldx #0
-
            lda #$20
            sta $0400,x
            sta $0500,x
            sta $0600,x
            sta $0700,x
            lda #$01
            sta $d800,x
            sta $d900,x
            sta $da00,x
            sta $db00,x
            inx
            bne -

            ldx #0
            stx loopcnt

            ; disable alarm irq
            lda #$04
            sta $dd0d

            lda #$80    ; set 50hz
            sta $dd0e

            lda #0
            sta linenr
            lda #>($0400+0*40)
            sta scrptr+1
            lda #<($0400+0*40)
            sta scrptr+0

            lda #$00    ; set clock
            sta $dd0f

            jsr inittod

            ; first write tsec, then hour... that means clock is stopped after
            ; the write
            lda #$01
            sta $dd08   ; tsec (start clock)
            lda #$01
            sta $dd0b   ; hour (stop clock)
            lda #$59
            sta $dd0a   ; min
            lda #$59
            sta $dd09   ; sec

            ; now read the time 3 times to prove it is really stopped
            jsr waitandprint3s

            ; now write to sec, min, hour to show clock will not start
            jsr nextline

            lda #$12
            sta $dd09   ; sec

            jsr waitandprint3s

            lda #$34
            sta $dd0a   ; min

            jsr waitandprint3s

            lda #$05
            sta $dd0b   ; hour (stop clock)

            jsr waitandprint3s

            ; finally start the clock by storing the 10th seconds
            jsr nextline

            lda #$01
            sta $dd08   ; tsec (start clock)

            jsr waitandprint
            jsr waitandprint
            jsr waitandprint
            jsr nextline

            ; and check that writing sec and min will not stop it
            
            lda #$34
            sta $dd09   ; sec

            jsr waitandprint
            jsr waitandprint
            jsr waitandprint
            jsr nextline

            lda #$12
            sta $dd0a   ; min

            jsr waitandprint
            jsr waitandprint
            jsr waitandprint
            jsr nextline

            ; now check that writing to any time registers will not stop
            ; the clock when alarm is selected
            jsr nextline
            
            lda #$80    ; set alarm
            sta $dd0f

            lda #$01
            sta $dd0b   ; hour (stop clock)
            
            jsr waitandprint3

            lda #$00
            sta $dd0a   ; min

            jsr waitandprint3

            lda #$00
            sta $dd09   ; sec

            jsr waitandprint3
            
            lda #$00
            sta $dd08   ; tsec (start clock)

            jsr waitandprint3

            ; last not least first stop the clock, then check that writing to
            ; any registers does not start the clock when alarm is selected
            jsr nextline

            lda #$00    ; set clock
            sta $dd0f

            lda #$02
            sta $dd0b   ; hour (stop clock)

            lda #$80    ; set alarm
            sta $dd0f

            jsr waitandprint3s

            lda #$00
            sta $dd0a   ; min

            jsr waitandprint3s

            lda #$00
            sta $dd09   ; sec

            jsr waitandprint3s
            
            lda #$00
            sta $dd08   ; tsec (start clock)

            jsr waitandprint3s

bcol:       lda #5
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

waitandprint3s:
            jsr readtime
            jsr printtime

            ; wait about 0.2 seconds
            ldx #20*1
            jsr waitframes

            jsr readtime
            jsr printtime

            ; wait about 0.2 seconds
            ldx #20*1
            jsr waitframes

            jsr readtime
            jsr printtime

            jsr nextline
            rts
            
waitandprint3:
            jsr waitandprint
            jsr waitandprint
            jsr waitandprint
            jsr nextline
            rts

waitandprint:
            lda $dd08
-           cmp $dd08
            beq -

            jsr readtime
            jsr printtime
            rts

cmptab:

            !byte $01,$59,$59,$01, $01,$59,$59,$01, $01,$59,$59,$01
            
            !byte $01,$59,$12,$01, $01,$59,$12,$01, $01,$59,$12,$01
            !byte $01,$34,$12,$01, $01,$34,$12,$01, $01,$34,$12,$01
            !byte $05,$34,$12,$01, $05,$34,$12,$01, $05,$34,$12,$01

            !byte $05,$34,$12,$02, $05,$34,$12,$03, $05,$34,$12,$04
            !byte $05,$34,$34,$05, $05,$34,$34,$06, $05,$34,$34,$07
            !byte $05,$12,$34,$08, $05,$12,$34,$09, $05,$12,$35,$00
            
            !byte $05,$12,$35,$01, $05,$12,$35,$02, $05,$12,$35,$03
            !byte $05,$12,$35,$04, $05,$12,$35,$05, $05,$12,$35,$06
            !byte $05,$12,$35,$07, $05,$12,$35,$08, $05,$12,$35,$09
            !byte $05,$12,$36,$00, $05,$12,$36,$01, $05,$12,$36,$02
            
            !byte $02,$12,$36,$02, $02,$12,$36,$02, $02,$12,$36,$02
            !byte $02,$12,$36,$02, $02,$12,$36,$02, $02,$12,$36,$02
            !byte $02,$12,$36,$02, $02,$12,$36,$02, $02,$12,$36,$02
            !byte $02,$12,$36,$02, $02,$12,$36,$02, $02,$12,$36,$02

inittod:
            ; make sure the time register and latch is always 01:00:00.0
            ; before the actual measurement
            lda #$00
            sta $dd0b
            lda #$59
            sta $dd0a
            lda #$59
            sta $dd09
            lda #$01
            sta $dd08

            ; wait some frames to make sure the clock tick occurred at least once
            ldx #10
            jsr waitframes

            ; wait until 10th seconds == 0
-           lda $dd08
            bne -
            rts
            
;-------------------------------------------------------------------------------

fail:
            lda #10
            sta bcol+1
            sta ccol+1

            inc loopcnt
            rts

checktime:
            lda loopcnt
            asl
            asl
            tax
            lda timebuf+0
            cmp cmptab+0,x
            bne fail
            lda timebuf+1
            cmp cmptab+1,x
            bne fail
            lda timebuf+2
            cmp cmptab+2,x
            bne fail
            lda timebuf+3
            cmp cmptab+3,x
            bne fail

            inc loopcnt
            rts

readtime:
            ; now we want latching, so start with hours
            lda $dd0b
            sta timebuf+0
            lda $dd0a
            sta timebuf+1
            lda $dd09
            sta timebuf+2
            lda $dd08
            sta timebuf+3
            rts

readtime_nolatch:
            lda $dd08
            sta timebuf+3
            lda $dd09
            sta timebuf+2
            lda $dd0a
            sta timebuf+1
            lda $dd0b
            sta timebuf+0
            rts
            
timebuf:
            !byte $de, $ad, $ba, $be

printtime:
            lda #5
            sta ccol+1

            jsr checktime
            
            lda timebuf+0
            jsr printhex
            lda #':'
            jsr printchr
            lda timebuf+1
            jsr printhex
            lda #':'
            jsr printchr
            lda timebuf+2
            jsr printhex
            lda #'.'
            jsr printchr
            lda timebuf+3
            jsr printhex
            lda #' '
            jsr printchr
            rts

printchr:
            ldy #0
            sta (scrptr),y
            clc
            lda scrptr
            adc #1
            sta scrptr
            lda scrptr+1
            adc #0
            sta scrptr+1
            rts

printhex:
            ldy #0
            pha
            lsr
            lsr
            lsr
            lsr
            tax
            lda hextab,x
            sta (scrptr),y
            iny
            pla
            and #$0f
            tax
            lda hextab,x
            sta (scrptr),y

            lda scrptr+1
            clc
            adc #$d4
            sta scrptr+1

ccol:       lda #0
            sta (scrptr),y
            dey
            sta (scrptr),y

            lda scrptr+1
            sec
            sbc #$d4
            sta scrptr+1
            
            clc
            lda scrptr
            adc #2
            sta scrptr
            lda scrptr+1
            adc #0
            sta scrptr+1

            rts

hextab:     !scr "0123456789abcdef"

nextline:
            inc linenr
thisline:
            lda linenr
            asl
            tax
            lda linetab,x
            sta scrptr+0
            lda linetab+1,x
            sta scrptr+1
            rts

linetab:
        !word ($0400+0*40)
        !word ($0400+1*40)
        !word ($0400+2*40)
        !word ($0400+3*40)
        !word ($0400+4*40)
        !word ($0400+5*40)
        !word ($0400+6*40)
        !word ($0400+7*40)
        !word ($0400+8*40)
        !word ($0400+9*40)
        !word ($0400+10*40)
        !word ($0400+11*40)
        !word ($0400+12*40)
        !word ($0400+13*40)
        !word ($0400+14*40)
        !word ($0400+15*40)
        !word ($0400+16*40)
        !word ($0400+17*40)
        !word ($0400+18*40)
        !word ($0400+19*40)
        !word ($0400+20*40)
        !word ($0400+21*40)
        !word ($0400+22*40)
        !word ($0400+23*40)
        !word ($0400+24*40)
