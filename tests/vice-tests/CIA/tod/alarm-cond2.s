
; test when the time=alarm condition is checked
; (set alarm time to current time)

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

            ; disable alarm irq
            lda #$04
            sta $dd0d
            lda $dd0d
loop:
            lda #$80    ; set alarm
            sta $dd0f

            lda #$ff
            sta $dd0b
            ;lda #$00
            sta $dd0a
            ;lda #$02    ; 2 seconds
            sta $dd09
            ;lda #$00
            sta $dd08


            lda #$00    ; set clock
            sta $dd0f

            lda #$00
            sta $dd0b
            ;lda #$00
            sta $dd0a
            ;lda #$00
            sta $dd09
            lda #$01
            sta $dd08

            lda $dd0d

            ; wait for 10th sec change
            lda $dd08
wait:
            cmp $dd08
            beq wait
            sta $0400+0
            tay
            iny

            lda $dd0d
            sta $0400+1

            lda #$80    ; set alarm
            sta $dd0f

            ldx #$00
            stx $dd0b

            lda $dd0d
            sta $0400+2
            and #$04 
            bne fail

            ;ldx #$00
            stx $dd0a

            lda $dd0d
            sta $0400+3
            and #$04 
            bne fail

            ;ldx #$02    ; 2 seconds
            stx $dd09

            lda $dd0d
            sta $0400+4
            and #$04 
            bne fail

            ;ldx #$03	; 1 tenth
            sty $dd08

;            ldx #6 ; .1 sec
;            jsr waitframes

            lda #5
            sta $d020

            lda #0      ; success
            sta $d7ff

            inc $07e7

            lda $dd0d
            sta $0400+5

            and #$04 
            beq fail
            jmp loop
fail:
            lda #$2
            sta $d020
            lda #$ff    ; failure
            sta $d7ff
            jmp *
