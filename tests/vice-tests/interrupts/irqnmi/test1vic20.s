    .macpack longbranch
    .export Start

tmp = $02
tmp2 = $04

buffer = $1b00 ; get the dump from here

.if (expanded=1) 
basicstart = $1201      ; expanded
screenmem = $1000
colormem = $9400
.else
basicstart = $1001      ; unexpanded
screenmem = $1e00
colormem = $9600
.endif

charsperline = 22
screenlines = 23

                .org basicstart - 2
                .word basicstart

                .word basicstart + $0c
                .byte $0a, $00
                .byte $9e
.if (expanded=1)
                .byte $34, $36, $32, $32
.else
                .byte $34, $31, $31, $30
.endif
                .byte 0,0,0

;                * = basicstart + $0d
Start:
                sei
                ldx #$ff
                txs
                
                lda #0
                sta $900f
                jsr initscreen
    
    
                ldx #0
lpx:
                ldy #0
lpy:
                jsr dotest
                iny
                cpy #charsperline
                bne lpy

                inx
                cpx #screenlines
                bne lpx

                jsr copyscreen
                jsr comparescreen

                ldy #0  ; success
                lda $900f
                cmp #5
                beq @skp
                ldy #$ff ; failure
@skp:
                sty $910f
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

                ; disable timer irqs
                lda #$7f
                sta $912e     ; VIA2 IE disable and acknowledge interrupts
                sta $912d     ; VIA2 IF
                sta $911e     ; VIA1 IE disable NMIs (Restore key)
                sta $911d     ; VIA1 IF

                lda #$00      ; enable Timer A one-shot mode
                sta $911b     ; VIA1 AUX
                sta $912b     ; VIA2 AUX

                ; stop timers

                ; clear pending irqs
                bit $9114
                bit $9124
                ;lda $dd0d

                lda $9004
                bne *-3

                ; Set NMI vector
                lda #<nmihandler
                sta $0318
                lda #>nmihandler
                sta $0319

                ; Set IRQ vector
                lda #<irqhandler
                sta $0314
                lda #>irqhandler
                sta $0315

                
                ; setup timer for NMI
                lda #1
                clc
timenmi:        adc #0
                sta $9116 ; VIA1 TA lo
                
                ; setup timer for IRQ
                lda #1+4
                clc
timeirq:        adc #0
                sta $9126 ; VIA2 TA lo
                
                ; setup reference timer
.if SHOW = 0
                lda #8+128+104
.else
                lda #8+128+54
.endif
                sta $9128       ; VIA2 TB lo

                ; enable timer irqs
                lda #$c0
                sta $912e     ;VIA2 IF enable Timer A interrupts
                sta $911e     ;VIA1 IF enable Timer A interrupts
                
                ; start reference timer
                ; Fire 1-shot timers

                lda #0
                sta $9129 ; 4 VIA2 TB hi
                sta $9125 ; 4 VIA2 TA hi+count
                sta $9115 ; 4 VIA1 TA hi+count
                
                nop
                nop
                cli
                
                ; run 512 cycles NOPs
                .repeat 256
                nop
                .endrepeat
                
                rts
                
;-------------------------------------------------------------------------------
nmihandler:
                pha
                lda $9128       ; VIA2 TB lo
nmiout:
.if SHOW = 1
                sta screenmem
.else
                bit screenmem
.endif
                bit $9114       ; VIA1 TA lo latch

                pla
                rti
;-------------------------------------------------------------------------------
irqhandler:
                lda $9128       ; VIA2 TB lo
irqout:
.if SHOW = 0
                sta screenmem
.else
                bit screenmem
.endif
                bit $9124       ; VIA2 TA lo latch

                pla
                tay
                pla
                tax
                pla
                rti
                
;-------------------------------------------------------------------------------
initscreen:
                ldx #0
lp1:
                lda #32
                sta screenmem,x
                sta screenmem+$100,x
                lda #1
                sta colormem,x
                sta colormem+$100,x
                inx
                bne lp1
                rts
copyscreen:
                ldx #0
lp3:
                lda screenmem,x
                sta buffer,x
                lda screenmem+$100,x
                sta buffer+$100,x
                inx
                bne lp3
                rts
;-------------------------------------------------------------------------------
comparescreen:
                lda #$05
                sta $900f

                ldx #0
lp2:
                .repeat 2,cnt
                .scope
                ldy #5
                lda screenmem+(cnt*$100),x
                cmp refdata+(cnt*$100),x
                beq @skp
                lda #$02
                sta $900f
                
                ldy #2
@skp:
                tya
                sta colormem+(cnt*$100),x
                .endscope
                .endrepeat

                inx
                bne lp2
                rts
                
;-------------------------------------------------------------------------------
lineptrh:
                .repeat screenlines, count
                .byte > (screenmem + (count * charsperline))
                .endrepeat
lineptrl:
                .repeat screenlines, count
                .byte < (screenmem + (count * charsperline))
                .endrepeat
                
;-------------------------------------------------------------------------------

refdata:
.if SHOW = 0
    .incbin "dumpvicirq.bin",2
.else
    .incbin "dumpvicnmi.bin",2
.endif
