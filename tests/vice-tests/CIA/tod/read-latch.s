
; "All four TOD registers latch on a read of Hours and remain latched until 
;  after a read of 10ths of seconds. The TOD clock continues to count when the 
;  output registers are latched."
;
; "If only one register is to be read, there is no carry problem and the register 
;  can be read "on the fly," provided that any read of Hours is followed by a read 
;  of 10ths of seconds to disable the latching."

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

;            ; wait a bit
;            ldx #20*1
;            jsr waitframes

            ; sync to the TOD. this "unlatches" and waits for tsec to change
            lda $dd08
-           cmp $dd08
            beq -
            lda $dd08
            sta timebuf+3

            ; now read the time hour->secs so it will latch, and NOT get
            ; "unlatched" after that (because tsec is not read)
            jsr waitandprint3s

            ; wait about 1 second
            ldx #50*1
            jsr waitframes

            jsr waitandprint3s

            ; wait about 1 second
            ldx #50*1
            jsr waitframes

            jsr waitandprint3s

            ; "unlatch" by reading tsec
            jsr nextline
            
            ; wait about 1 second
            ldx #50*1
            jsr waitframes

            jsr waitandprint4s

            ; read time min->tsec "on the fly"
            jsr nextline

            ; wait about 1 second
            ldx #50*1
            jsr waitframes

            ; sync to the TOD
            lda $dd08
-           cmp $dd08
            beq -

            jsr waitandprint3m
            jsr waitandprint3m
            jsr waitandprint3m

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
            jsr readtime3
            jsr printtime

            ; wait about 0.2 seconds
            ldx #20*1
            jsr waitframes

            jsr readtime3
            jsr printtime

            ; wait about 0.2 seconds
            ldx #20*1
            jsr waitframes

            jsr readtime3
            jsr printtime

            jsr nextline
            rts
            
waitandprint3m:
            jsr readtime3m
            jsr printtime

            ; wait about 0.2 seconds
            ldx #20*1
            jsr waitframes

            jsr readtime3m
            jsr printtime

            ; wait about 0.2 seconds
            ldx #20*1
            jsr waitframes

            jsr readtime3m
            jsr printtime

            jsr nextline
            rts
            
waitandprint4s:
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


cmptab:

            !byte $01,$00,$00,$01, $01,$00,$00,$01, $01,$00,$00,$01
            !byte $01,$00,$00,$01, $01,$00,$00,$01, $01,$00,$00,$01
            !byte $01,$00,$00,$01, $01,$00,$00,$01, $01,$00,$00,$01

            !byte $01,$00,$00,$01, $01,$00,$05,$08, $01,$00,$06,$02

            !byte $01,$00,$07,$03, $01,$00,$07,$06, $01,$00,$08,$00
            !byte $01,$00,$08,$00, $01,$00,$08,$04, $01,$00,$08,$08
            !byte $01,$00,$08,$08, $01,$00,$09,$02, $01,$00,$09,$06

inittod:
            lda $dd08 ; unlatch
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
            ldx #20 * 1
            jsr waitframes

            ; wait until 10th seconds == 0
-           lda $dd08
            bne -

            lda $dd0b ; copy time to latch
            lda $dd08 ; unlatch
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

readtime3:
            lda $dd0b
            sta timebuf+0
            lda $dd0a
            sta timebuf+1
            lda $dd09
            sta timebuf+2
;            lda #0
;            sta timebuf+3
            rts

readtime3m:
;            lda $dd0b
;            sta timebuf+0
            lda $dd0a
            sta timebuf+1
            lda $dd09
            sta timebuf+2
            lda $dd08
            sta timebuf+3
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
