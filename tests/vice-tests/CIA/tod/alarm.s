; ACME-Testprogram:
; Interrupt Bit2 von $DD0D testen (Uhrzeit=Alarmzeit)
; Rahmen grün einfärben und Programm beenden 

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

            lda #$80    ; set alarm
            sta $dd0f

            lda #$00
            sta $dd0b
            lda #$00
            sta $dd0a
            lda #$02    ; 2 seconds
            sta $dd09
            lda #$00
            sta $dd08

            lda #$00    ; set clock
            sta $dd0f

            lda #$00
            sta $dd0b
            ;lda #$00
            sta $dd0a
            ;lda #$00
            sta $dd09
            ;lda #$00
            sta $dd08

            ldx #60 * 3 ; 3 seconds
            jsr waitframes

            lda #2
            sta $d020

-           lda $dd0d
            and #$04 
            beq -

            lda #$05
            sta $d020
            lda #0      ; success
            sta $d7ff
            jmp *
