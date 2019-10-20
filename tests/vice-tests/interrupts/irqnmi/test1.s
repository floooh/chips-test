    .macpack longbranch
    .export Start

tmp = $02
tmp2 = $04

Start:
                sei
                ldx #$ff
                txs
                
                lda #0
                sta $d020
                sta $d021
                jsr initscreen
    
    
                ldx #0
lpx:
                ldy #0
lpy:
                jsr dotest
                iny
                cpy #19
                bne lpy

                inx
                cpx #19
                bne lpx

                jsr comparescreen

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
                
;-------------------------------------------------------------------------------

dotest:
                sei
                
                stx timeirq+1
                sty timenmi+1
                
                lda lineptrh,x
                sta irqout+2
                sta nmiout+2
                lda lineptrl,x
                sta irqout+1
                sta nmiout+1

                tya
                clc
                adc irqout+1
                sta irqout+1
                lda #0
                adc irqout+2
                sta irqout+2

                tya
                clc
                adc nmiout+1
                sta nmiout+1
                lda #0
                adc nmiout+2
                sta nmiout+2

                lda #20
                clc
                adc nmiout+1
                sta nmiout+1
                lda #0
                adc nmiout+2
                sta nmiout+2

                ; screen off
                lda #$0b
                sta $d011

                lda #$35
                sta $01

                ; stop timers
                lda #%00000000
                sta $dc0e
                sta $dd0e
                sta $dc0f
                sta $dd0f

                ; disable timer irqs
                lda #$7f
                sta $dc0d
                sta $dd0d

                ; clear pending irqs
                lda $dc0d
                lda $dd0d

                lda $d011
                bpl *-3
                lda $d011
                bmi *-3

                ; Set NMI vector
                lda #<nmihandler
                sta $fffa
                lda #>nmihandler
                sta $fffb

                ; Set IRQ vector
                lda #<irqhandler
                sta $fffe
                lda #>irqhandler
                sta $ffff

                
                ; setup timer for NMI
                lda #1
                clc
timenmi:        adc #0
                sta $dd04
                lda #0
                sta $dd05
                
                ; setup timer for IRQ
                lda #1+4
                clc
timeirq:        adc #0
                sta $dc04
                lda #0
                sta $dc05
                
                ; setup reference timer
                lda #8+128+70
                sta $dc06
                lda #0
                sta $dc07

                ; enable timer irqs
                lda #$81
                sta $dc0d
                sta $dd0d
                
                ; start reference timer
                lda #%00010001 
                sta $dc0f
                ; Fire 1-shot timer
                lda #%10011001  ; 2
                sta $dc0e       ; 4
                sta $dd0e       ; 4
                
                nop
                nop
                cli
                
                ; run 512 cycles NOPs
                .repeat 256
                nop
                .endrepeat
                
                ; screen on
                lda #$1b
                sta $d011

                rts
                
;-------------------------------------------------------------------------------
nmihandler:
                pha
                lda $dc06
nmiout:         sta $0400 + (1*40) + 21
                lda $dd0d
                pla
                rti
;-------------------------------------------------------------------------------
irqhandler:
                pha
                lda $dc06
irqout:         sta $0400 + (1*40) + 1
                lda $dc0d
                pla
                rti
                
;-------------------------------------------------------------------------------
initscreen:
                lda #32
                ldx #0
lp1:
                sta $0400,x
                sta $0500,x
                sta $0600,x
                sta $0700,x
                inx
                bne lp1
                rts
;-------------------------------------------------------------------------------
comparescreen:
                ldy #5
                sty $d020

                ldx #0
lp2:
                .repeat 3,cnt
                ldy #5
                lda $0400+(cnt*$100),x
                cmp refdata+(cnt*$100),x
                beq *+(2+2+3)
                ldy #2
                sty $d020

                tya
                sta $d800+(cnt*$100),x
                .endrepeat

                inx
                bne lp2
                rts
                
;-------------------------------------------------------------------------------
lineptrh:
                .repeat 25, count
                .byte > ($0400 + (count * 40))
                .endrepeat
lineptrl:
                .repeat 25, count
                .byte < ($0400 + (count * 40))
                .endrepeat
                
;-------------------------------------------------------------------------------

refdata:
.if CIAOLD = 0
    .incbin "dumpnew.bin",2
.else
    .incbin "dumpold.bin",2
.endif