
; powerup state check
; this program shows the following:
; - at powerup the clock is not running
; - the time value read from the latch is 01:00:00.0 (may also be 11:01:00.0)
; - the am/pm flag is random
; - the value read is always the time, regardless of CRB bit7
; - reading the clock (hour to tenths) does not start it
; - the alarm is set to 00:00:00.0 by default, and because of that does not
;   trigger unless the time is forced to 00:00:00.0 too

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

            ;===================================================================

            ; wait one second to make sure that IF the clock would be running,
            ; we will get other values than expected
            ldx #50*1
            jsr waitframes

            ; first read the time, starting with tenth secs to avoid latching
            lda #$00    ; set clock
            sta $dd0f

            jsr readtime_nolatch
            jsr check01000000
            jsr ackicr00
            jsr printtime
            lda icrbuf
            jsr printhex
            jsr nextline
            ;-------------------------------------------------------------------

            jsr readtime
            jsr check01000000
            jsr ackicr00
            jsr printtime
            lda icrbuf
            jsr printhex
            jsr nextline
            ;-------------------------------------------------------------------

            ; wait one second again to make sure the above procedure did not
            ; start the clock
            ldx #50*1
            jsr waitframes

            ; set to alarm and read again, we should get the same value(s)
            ; since reading always gives the time

            lda #$80    ; set alarm
            sta $dd0f

            ; first read the time, starting with tenth secs to avoid latching
            jsr readtime_nolatch
            jsr check01000000
            jsr ackicr00
            jsr printtime
            lda icrbuf
            jsr printhex
            jsr nextline
            ;-------------------------------------------------------------------

            jsr readtime
            jsr check01000000
            jsr ackicr00
            jsr printtime
            lda icrbuf
            jsr printhex
            jsr nextline

            jsr nextline
            ;-------------------------------------------------------------------

            lda #$00    ; set clock
            sta $dd0f

            ldx #50*1
            jsr waitframes

            ; read clock again to show its not running
            jsr readtime
            jsr check01000000
            jsr ackicr00
            jsr printtime
            lda icrbuf
            jsr printhex
            jsr nextline

            jsr nextline
            ;-------------------------------------------------------------------

            ; now set the time to 92:59:59.1 and wait for 01:00:00.0, for which
            ; we expect the alarm to occur

            lda #$92
            sta $dd0b
            lda #$59
            sta $dd0a
            lda #$59
            sta $dd09
            lda #$01
            sta $dd08

            lda $dd0d

-
            jsr thisline        ; CR

            jsr readtime
            lda $dd0d
            sta icrbuf

            jsr chkicr00
            jsr printtime

            lda icrbuf
            jsr printhex

            lda timebuf+2       ; skip after 1 second
            cmp #1
            beq +

            lda icrbuf
            and #$04
            beq -
+
            jsr nextline
            ;-------------------------------------------------------------------

            jsr readtime
            lda $dd0d
            sta icrbuf

            jsr check01000100
            jsr chkicr00
            jsr printtime

            lda icrbuf
            jsr printhex
            jsr nextline
            ;-------------------------------------------------------------------

            ; now set the time to 12:59:59.1 and wait for 91:00:00.0, for which
            ; we expect the alarm to occur

            lda #$12
            sta $dd0b
            lda #$59
            sta $dd0a
            lda #$59
            sta $dd09
            lda #$01
            sta $dd08

            lda $dd0d

-
            jsr thisline        ; CR

            jsr readtime
            lda $dd0d
            sta icrbuf

            jsr chkicr00
            jsr printtime

            lda icrbuf
            jsr printhex

            lda timebuf+2       ; skip after 1 second
            cmp #1
            beq +

            lda icrbuf
            and #$04
            beq -
+
            jsr nextline
            ;-------------------------------------------------------------------

            jsr readtime
            lda $dd0d
            sta icrbuf

            jsr check81000100
            jsr chkicr00
            jsr printtime

            lda icrbuf
            jsr printhex
            jsr nextline
            ;-------------------------------------------------------------------
            ; last we set the time to 00:00:00.0 and see if an alarm triggers
            ; 00 00:00:00.00 04

            lda $dd00

            lda #$00
            sta $dd0b
            lda #$00
            sta $dd0a
            lda #$00
            sta $dd09
            lda #$00
            sta $dd08

-
            jsr thisline        ; CR
            
            jsr readtime
            lda $dd0d
            sta icrbuf

            jsr check00000000
            jsr printtime

            lda icrbuf
            jsr printhex
            
            lda timebuf+2       ; skip after 1 second
            cmp #1
            beq +

            lda icrbuf
            and #$04
            beq -
+

            lda icrbuf
            and #$04
            cmp #$04
            beq +
            jsr fail
+

            jsr nextline
            ;-------------------------------------------------------------------
            ; 00 00:00:00.00 00
            jsr readtime
            lda $dd0d
            sta icrbuf

            jsr check00000000
            jsr chkicr00
            jsr printtime

            lda icrbuf
            jsr printhex
            jsr nextline
            ;-------------------------------------------------------------------

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

;-------------------------------------------------------------------------------

fail:
            lda #$2
            sta bcol+1
            rts

ackicr00:
            lda $dd0d
            sta icrbuf
chkicr00:
            lda icrbuf
            beq +
            jsr fail
+
            rts
            
check00000000:
            lda timebuf
            cmp #$00
            bne fail
            jmp check000000
            
check01000000:
            lda timebuf
            and #$7f
            cmp #$01
            bne fail
check000000:
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

check81000100:
            lda timebuf
            cmp #$81
            bne fail
            jmp check000100
check01000100:
            lda timebuf
            cmp #$01
            bne fail
check000100:
            lda timebuf+1
            cmp #$00
            bne fail
            lda timebuf+2
            cmp #$01
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
icrbuf:
            !byte $00

printtime:
            lda timebuf+0
            and #$80
            jsr printhex
            lda #' '
            jsr printchr

            lda timebuf+0
            and #$7f
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

            clc
            lda scrptr
            adc #2
            sta scrptr
            lda scrptr+1
            adc #0
            sta scrptr+1

            rts

hextab:     !text "0123456789abcdef"

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
