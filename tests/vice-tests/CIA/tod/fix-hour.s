
; check how nonsense values are handled
; -> hour register

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

            ldx #0
loop:
            stx loopcnt

            jsr inittod

            ; start clock at nn:59:59.01
            ldx loopcnt
            lda tabhours,x
            ;ora #$80
            pha
            sta $dd0b
            lda #$59
            sta $dd0a
            lda #$59
            sta $dd09
            lda #$01
            sta $dd08

            jsr readtime_nolatch

            lda #1
            sta ccol+1
            
            pla
            jsr printhex
            lda #' '
            jsr printchr

            lda #5
            sta ccol+1
            
            ldx loopcnt
            lda cmptab1,x
            jsr check59
            jsr printtime

            lda #5
            sta ccol+1
            
            jsr readtime
            ldx loopcnt
            lda cmptab2,x
            jsr check59
            jsr printtime

            ; wait some frames to make sure the clock tick occurred at least once
            ldx #10
            jsr waitframes

            ; wait until 10th seconds == 0
-           lda $dd08
            bne -

            lda #5
            sta ccol+1

            jsr readtime
            ldx loopcnt
            lda cmptab3,x
            jsr check00
            jsr printtime
            jsr nextline

            ldx loopcnt
            inx
            cpx #24
            bne loop

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

tabhours:
            !byte $00,$09
            !byte $0a,$0b,$0c,$0d,$0e,$0f,$10,$11,$12
            !byte $13,$14,$15,$16,$17,$18,$19
            !byte $1a,$1b,$1c,$1d,$1e,$1f
cmptab1:
cmptab2:
            !byte $00,$09
            !byte $0a,$0b,$0c,$0d,$0e,$0f,$10,$11,$92
            !byte $13,$14,$15,$16,$17,$18,$19
            !byte $1a,$1b,$1c,$1d,$1e,$1f
cmptab3:
            !byte $01,$10
            !byte $0b,$0c,$0d,$0e,$0f,$00,$11,$92,$81
            !byte $14,$15,$16,$17,$18,$19,$1a
            !byte $1b,$1c,$1d,$1e,$1f,$10
;-------------------------------------------------------------------------------

fail:
            lda #10
            sta bcol+1
            sta ccol+1
            rts

check59:
            cmp timebuf
            bne fail
            lda timebuf+1
            cmp #$59
            bne fail
            lda timebuf+2
            cmp #$59
            bne fail
            lda timebuf+3
            cmp #$01
            bne fail
            rts

check00:
            cmp timebuf
            bne fail
            lda timebuf+1
            cmp #$00
            bne fail
            lda timebuf+2
            cmp #$00
            bne fail
            lda timebuf+3
            cmp #$00
            bne fail
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
