;----------------------------------------------------------------
; This program tests cycle exact reading of memory address $DD0D.
; It checks the values depending on the detected CIA model (old or new).
;
; Programmed in June 2007 by Wilfred Bos
; Extended in April 2016
;
; Use 64tass to assemble the code.
;----------------------------------------------------------------
              SYS = $9e
              ENDBASIC = $0000

              DUMMY = $AA

              *=$0801
              .word (+), 1234
              .null SYS, ^start
+             .word ENDBASIC
;---------------------------------------

start         jsr saveSettings
              jsr disableInterrupts
              jsr screenOff

              php
              pla
              sta oldstatus   ; save processor status register (SR)
              tsx
              stx oldstack    ; save stack pointer (SP)

              jsr acktst1
              lda nmicounter
              cmp #$80
              beq newCIAModel

oldCIAModel   jsr printText
              .null "OLD CIA DETECTED"

              lda #$0d      ; carriage return
              jsr $ffd2

              lda #<expected1
              sta $fa
              lda #>expected1
              sta $fb
              jmp initTest

newCIAModel   jsr printText
              .null "NEW CIA DETECTED"

              lda #$0d      ; carriage return
              jsr $ffd2

              lda #<expected2
              sta $fa
              lda #>expected2
              sta $fb

initTest      lda #$00
              sta erroroccured

              ldy #$00
              sty currentTest

              jsr acktst1         ; test 01
              jsr outputResult2

              jsr delayOrBreak

              jsr screenOff

              inc currentTest
              inc currentTest

              jsr acktst2         ; test 02
              jsr outputResult2

              jsr delayOrBreak

              inc currentTest
              inc currentTest

testloop      jsr screenOff

              ldy currentTest
              lda tests,y
              sta testtorun
              iny
              lda tests,y
              sta testtorun+1
              iny
              sty currentTest
              cmp #$ff
              beq otherTests

              jsr dd0dtestrunner
              jsr outputResult

              jsr delayOrBreak

              jmp testLoop

delayOrBreak  ldx #$10    ; wait 10 * 20 ms and check for run-stop key

-             lda #$7f    ; check for run-stop key
              sta $dc00
              lda $dc01
              and #$80
              beq runtstopPressed

              lda #$80
-             cmp $d012
              bne -
-             lda $d011
              bpl -
              dex
              bpl ---
              rts
runtstopPressed
              pla
              pla
              jmp stoptests

otherTests    jsr screenOff

              jsr dd0dzero0delaytst0      ; test 17
              jsr outputResult

              jsr delayOrBreak

              inc currentTest
              inc currentTest

              jsr screenOff

              jsr dd0dzero0delaytst1      ; test 18
              jsr outputResult

              jsr delayOrBreak

              inc currentTest
              inc currentTest

              jsr screenOff

              jsr dd0dzero0delaytst2      ; test 19
              jsr outputResult

stoptests     lda oldstatus
              pha
              plp             ; restore SP
              ldx oldstack
              txs             ; restore SR

endCalc       lda #$1b        ; enable video
              sta $d011

              lda #$5
              sta $d020
              lda #0            ; success
              sta $d7ff

              jsr restoreSettings
              rts

screenOff     lda $d011
              and #$ef        ;disable video to avoid bad line scan
              sta $d011

-             lda $d011
              bmi -
-             lda $d011
              bpl -
-             lda $d011
              bmi -
              rts

saveSettings  lda $fffa
              sta oldnmivectlo
              lda $fffb
              sta oldnmivecthi
              lda $fffe
              sta oldbrkvectlo
              lda $ffff
              sta oldbrkvecthi
              lda $d01a
              sta oldirqmask
              rts

restoreSettings
              lda oldnmivectlo
              sta $fffa
              lda oldnmivecthi
              sta $fffb

              lda oldbrkvectlo
              sta $fffe
              lda oldbrkvecthi
              sta $ffff

              lda oldirqmask
              sta $d01a

              lda #$37
              sta $01
              lda #$2f
              sta $00

              jsr $fd15      ; init IO vectors
              jsr $fda3      ; init CIA timers

              lda erroroccured
              beq exit

              lda #2
              sta $d020
              lda #$ff       ; failure
              sta $d7ff

              jsr generateSound
exit          rts

disableInterrupts
              sei
              lda #$7f
              sta $dc0d
              sta $dd0d
              lda #$00
              sta $dc0e
              sta $dd0e
              sta $d01a
              lda $dc0d
              lda $dd0d
              cli
              rts

              .align $0100
;---------------------------------------
dd0dtestrunner
              sei
              lda #$35
              sta $01

              lda #$ff
              sta output+0
              sta output+1
              sta output+2
              sta output+3
              sta output+4
              sta output+5

              tsx
              txa
              sec
              sbc #$08    ; preserve some stack space for instructions that pull data from the stack
              tax
              txs
              sta sp

              lda #<nmi
              sta $fffa
              lda #>nmi
              sta $fffb

              lda #$04    ; generate interrupt after x cycles when timer is started
              clc
              adc #$11
              sta $dd04
              lda #$00
              sta $dd05

              lda #$81    ; enable CIA B, Timer A
              sta $dd0d
              lda #$11    ; start Timer A, Force Load
              sta $dd0e

              ldx #$00
              ldy #$00
              lda #$00
              cli
              bit $ea
              jmp (testtorun)

test03        bit $ea
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              bit $ea
              lda $dd0d,x
              sta output+2
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test04        bit $ea
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              bit $ea
              lda $dd0d,x
              sta output+2
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test05        nop
              nop
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              bit $ea
              lda $dd0d,x
              sta output+2
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test06        nop
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              bit $ea
              lda $dd0d
              sta output+2
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test07        nop
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              bit $ea
              lda $dd0d,x
              sta output+2
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test08        nop
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              bit $ea
              lda $dd0d
              nop
              sta output+2
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test09        ldx #$10
              nop
              lda $dcfd,x
              sta output+0
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              bit $ea
              lda $dd0d
              sta output+2
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test0a        nop
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              ldx #$10
              bit $ea
              lda $dcfd,x
              nop
              sta output+1
              nop
              nop
              nop
              nop
              nop
              bit $ea
              lda $dd0d
              sta output+2
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test0b        nop
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              inc $dd0d
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              bit $ea
              lda $dd0d
              sta output+2
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test0c        nop
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              inc $dd0d,x
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              bit $ea
              lda $dd0d
              sta output+2
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest


test0d        nop
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              nop
              nop
              bit $ea
              inc $dd0d,x
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              bit $ea
              lda $dd0d
              sta output+2
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test0e        nop
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              inc $dd0d,x
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              bit $ea
              lda $dd0d
              sta output+2
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test0f        nop
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              nop
              bit $ea
              inc $dd0d,x
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              bit $ea
              lda $dd0d
              sta output+2
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test10        nop
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              nop
              nop
              inc $dd0d,x
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+2
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test11        nop
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              bit $ea
              inc $dd0d,x
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+2
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test12        nop
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              inc $dd0d,x
              sta output+1
              sta output+2
              nop
              nop
              bit $ea
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test13        nop
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              inc $dd0d,x
              sta output+1
              sta output+2
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test14        nop
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              nop
              bit $ea
              inc $dd0d,x
              sta output+1
              sta output+2
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test15        nop
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              ldx #$10
              lda $ddfd,x   ; page crossing, read value in T3 cycle from $dd0d and through NMI away, load accumulator with value of memory $de0d
              lda #$00      ; write value $00 since value of memory $de0d is not part of this test
              sta output+2
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

test16        nop
              nop
              lda $dd0d
              sta output+0
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              sta $dd0d,x
              sta output+2
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              lda $dd0d
              sta output+3
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp endtest

; this code should only be reached for some test cases
endtest       jsr *+3         ; push PC+3 on stack
              php
              sei
              jmp nmi

;---------------------------------------
acktst1       sei
              lda #$35
              sta $01

              lda #<nmiacktst1
              sta $fffa
              lda #>nmiacktst1
              sta $fffb
              jmp acktstrunner

acktst2       sei
              lda #$35
              sta $01

              lda #<nmiacktst2
              sta $fffa
              lda #>nmiacktst2
              sta $fffb
              jmp acktstrunner

acktstrunner  tsx
              txa
              sec
              sbc #$18    ; preserve some stack space for instructions that pull data from the stack
              tax
              txs
              sta sp

              lda #$01    ; generate interrupt after 1 cycle when timer is started
              sta $dd04
              lda #$00
              sta $dd05

              lda #$00
              sta nmicounter

              lda #$81    ; enable CIA B, Timer A
              sta $dd0d
              lda #$01    ; start Timer A
              sta $dd0e

              lda #$00
              ldx #$00
              ldy #$00
              cli
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp stopacktst

nmiacktst1    sta ac
              stx xr
              sty yr

              lda nmicounter
              cmp #$00
              bne stopacktst

              lda #$80
              sta nmicounter

              lda $dd0d
              bit $ea
              inc nmicounter
              inc nmicounter
              inc nmicounter
              inc nmicounter
              jmp stopacktst

nmiacktst2    sta ac
              stx xr
              sty yr

              lda nmicounter
              cmp #$00
              bne stopacktst

              lda #$80
              sta nmicounter

              lda $dd0d
              nop
              nop
              inc nmicounter
              inc nmicounter
              inc nmicounter
              inc nmicounter
              jmp stopacktst


stopacktst    lda oldstatus
              ora #$04        ; set interrupt flag
              pha
              plp             ; restore status register

              lda sp
              clc
              adc #$18
              tax
              txs             ; restore stackpointer to exit interrupt nicely
              lda #$37
              sta $01
              lda #$7f        ; disable CIA B
              sta $dd0d
              lda #$00
              sta $dd0e
              sta $dd0f
              lda $dd0d       ; acknowledge interrupt
              lda ac
              ldx xr
              ldy yr
              cli
              ; end NMI with RTS since registers are pulled from the stack already
              rts

;---------------------------------------

dd0dzero0delaytst0
              sei
              lda #$35
              sta $01

              lda #<nmizerotst1
              sta $fffa
              lda #>nmizerotst1
              sta $fffb

              tsx
              txa
              sec
              sbc #$18    ; preserve some stack space for instructions that pull data from the stack
              tax
              txs
              sta sp

              lda #$20
              sta $dd04
              lda #$00
              sta $dd05

              lda #$30
              sta $dd06
              lda #$00
              sta $dd07

              lda #$ff
              sta output+0
              sta output+1
              sta output+2
              sta output+3
              sta output+4
              sta output+5

              lda #$81    ; enable CIA B, Timer A
              sta $dd0d
              lda #$11    ; start Timer B, Force Load
              sta $dd0f
              lda #$19    ; start Timer A, Force Load, One shot
              sta $dd0e
              nop
              nop

              lda #$00
              ldx #$00
              ldy #$00
              cli
              nop
              nop
              nop
              nop
              nop
              bit $ea
              bit $ea
              ldx #$00
              inc $dd0d,x       ; kill Timer A, enable Timer B
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp stopacktst

dd0dzero0delaytst1
              sei
              lda #$35
              sta $01

              lda #<nmizerotst1
              sta $fffa
              lda #>nmizerotst1
              sta $fffb

              tsx
              txa
              sec
              sbc #$18    ; preserve some stack space for instructions that pull data from the stack
              tax
              txs
              sta sp

              lda #$20
              sta $dd04
              lda #$00
              sta $dd05

              lda #$30
              sta $dd06
              lda #$00
              sta $dd07

              lda #$ff
              sta output+0
              sta output+1
              sta output+2
              sta output+3
              sta output+4
              sta output+5

              lda #$81    ; enable CIA B, Timer A
              sta $dd0d
              lda #$11    ; start Timer B, Force Load
              sta $dd0f
              lda #$19    ; start Timer A, Force Load, One shot
              sta $dd0e
              nop
              nop

              lda #$00
              ldx #$00
              ldy #$00
              cli
              nop
              nop
              nop
              nop
              nop
              bit $ea
              nop
              nop
              ldx #$00
              inc $dd0d,x       ; kill Timer A, enable Timer B
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp stopacktst

dd0dzero0delaytst2
              sei
              lda #$35
              sta $01

              lda #<nmizerotst1
              sta $fffa
              lda #>nmizerotst1
              sta $fffb

              tsx
              txa
              sec
              sbc #$18    ; preserve some stack space for instructions that pull data from the stack
              tax
              txs
              sta sp

              lda #$20
              sta $dd04
              lda #$00
              sta $dd05

              lda #$30
              sta $dd06
              lda #$00
              sta $dd07

              lda #$ff
              sta output+0
              sta output+1
              sta output+2
              sta output+3
              sta output+4
              sta output+5

              lda #$81    ; enable CIA B, Timer A
              sta $dd0d
              lda #$11    ; start Timer B, Force Load
              sta $dd0f
              lda #$19    ; start Timer A, Force Load, One shot
              sta $dd0e
              nop
              nop

              lda #$00
              ldx #$00
              ldy #$00
              cli
              nop
              nop
              nop
              nop
              nop
              bit $ea
              nop
              bit $ea
              ldx #$00
              inc $dd0d,x       ; kill Timer A, enable Timer B
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp stopacktst

dd0dzero0delaytst3
              sei
              lda #$35
              sta $01

              lda #<nmizerotst1
              sta $fffa
              lda #>nmizerotst1
              sta $fffb

              tsx
              txa
              sec
              sbc #$18    ; preserve some stack space for instructions that pull data from the stack
              tax
              txs
              sta sp

              lda #$20
              sta $dd04
              lda #$00
              sta $dd05

              lda #$30
              sta $dd06
              lda #$00
              sta $dd07

              lda #$ff
              sta output+0
              sta output+1
              sta output+2
              sta output+3
              sta output+4
              sta output+5

              lda #$81    ; enable CIA B, Timer A
              sta $dd0d
              lda #$11    ; start Timer B, Force Load
              sta $dd0f
              lda #$19    ; start Timer A, Force Load, One shot
              sta $dd0e
              nop
              nop

              lda #$00
              ldx #$00
              ldy #$00
              cli
              nop
              nop
              nop
              nop
              nop
              bit $ea
              bit $ea
              bit $ea
              ldx #$00
              inc $dd0d,x       ; kill Timer A, enable Timer B
              sta output+1
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              nop
              jmp stopacktst

nmizerotst1   sta ac
              stx xr
              sty yr

              bit $ea
              bit $ea

              lda $dd0d         ; should read $82 for new CIA model
              sta output+0

              lda #$00
              sta $dd0f
              sta $dd0e

              jmp stopacktst

;---------------------------------------

nmi           sta output+4
              sta ac
              stx xr
              sty yr

              lda oldstatus
              ora #$04        ; set interrupt flag
              pha
              plp             ; restore status register

              lda sp
              clc
              adc #$08
              tax
              txs             ; restore stackpointer to exit interrupt nicely
              lda #$37
              sta $01
              lda #$7f        ; disable CIA B
              sta $dd0d
              lda #$00
              sta $dd0e
              lda $dd0d       ; acknowledge interrupt
              sta output+5
              lda ac
              ldx xr
              ldy yr
              cli
              ; end NMI with RTS since registers are pulled from the stack already
              rts

;---------------------------------------
outputResult2
              lda #$1b
              sta $d011

              jsr printText
              .null "TEST "

              lda currentTest
              lsr
              clc
              adc #$01
              jsr printByte

              jsr printText
              .null " ACK   "

              lda currentTest
              asl
              asl
              tax           ; x reg is index of expected table
              tay

              lda nmicounter
              jsr checkError

              jsr printByte

              lda #154      ; lt. blue color
              jsr $ffd2

              lda errorFound
              beq printOk2

              jsr printText
              .null " FAILED"

              lda #$0d      ; carriage return
              jsr $ffd2

              lda #$00
              sta errorFound
              rts

printOk2      jsr printText
              .null " OK"

              lda #$0d      ; carriage return
              jsr $ffd2

              lda #$00
              sta errorFound
              rts
;---------------------------------------
outputResult
              lda #$1b
              sta $d011

              jsr printText
              .null "TEST "

              lda currentTest
              lsr
              jsr printByte

              jsr printText
              .null " READ "

              lda #$20      ; space
              jsr $ffd2

              lda #154      ; lt. blue color
              jsr $ffd2

              lda #$00
              sta errorFound

              lda currentTest
              clc
              sbc #$01
              asl
              asl
              tax           ; x reg is index of expected table
              tay

              lda output+0
              jsr checkError
              jsr printByte

              lda output+1
              jsr checkError
              jsr printByte

              lda output+2
              jsr checkError
              jsr printByte

              lda output+3
              jsr checkError
              jsr printByte

              lda output+4
              jsr checkError
              jsr printByte

              lda output+5
              jsr checkError
              jsr printByte

              lda #154      ; lt. blue color
              jsr $ffd2

              lda errorFound
              beq printOk

              jsr printText
              .null " FAILED"

              lda #$0d      ; carriage return
              jsr $ffd2

              jsr printText
              .null "     EXPECTED "

              lda ($fa),y
              jsr printByte
              iny
              lda ($fa),y
              jsr printByte
              iny
              lda ($fa),y
              jsr printByte
              iny
              lda ($fa),y
              jsr printByte
              iny
              lda ($fa),y
              jsr printByte
              iny
              lda ($fa),y
              jsr printByte

              jmp endOutput

printOk       jsr printText
              .null " OK"

endOutput     lda #$0d      ; carriage return
              jsr $ffd2
              rts

checkError    sty $fc
              pha

              lda #154      ; lt. blue color
              jsr $ffd2

              txa
              tay
              pla
              pha

              cmp ($fa),y
              beq noErrorFound

              lda #$01
              sta errorFound
              sta erroroccured

              lda #150      ; red color
              jsr $ffd2

noErrorFound  inx
              pla
              ldy $fc
              rts

printText     pla
              sta fetchtxt+1
              pla
              sta fetchtxt+2
              jmp inctxtaddr
fetchtxt      lda $ffff
              beq txtend
              jsr $ffd2
inctxtaddr    inc fetchtxt+1
              bne fetchtxt
              inc fetchtxt+2
              bne fetchtxt
txtend        lda fetchtxt+2
              pha
              lda fetchtxt+1
              pha
              rts

printByte     pha
              lsr
              lsr
              lsr
              lsr
              jsr printNibble
              pla
              and #$0f
              jsr printNibble
              rts

printNibble   pha
              and #$0f
              cmp #$0a
              bmi printNum
              clc
              adc #"A"-"0"-$0a
printNum      adc #$30
              jsr $ffd2
              pla
              rts

generateSound lda #$0f      ; generate sound on error
              sta $d418
              lda #$08
              sta $d400
              sta $d401
              lda #$0f
              sta $d405
              lda #$0a
              sta $d406
              lda #$21
              sta $d404
              lda #$20
              sta $d404
              rts

oldbrkvectlo  .byte $00
oldbrkvecthi  .byte $00

oldnmivectlo  .byte $00
oldnmivecthi  .byte $00
oldirqmask    .byte $00

ac            .byte $00
xr            .byte $00
yr            .byte $00
sp            .byte $00

oldstack      .byte $00
oldstatus     .byte $00

testtorun     .word 0
currentTest   .byte $00
errorFound    .byte $00
erroroccured  .byte $00
nmicounter    .byte $00

tests         .byte 0, 0, 0, 0
              .byte <test03, >test03, <test04, >test04, <test05, >test05, <test06, >test06
              .byte <test07, >test07, <test08, >test08, <test09, >test09, <test0a, >test0a
              .byte <test0b, >test0b, <test0c, >test0c, <test0d, >test0d, <test0e, >test0e
              .byte <test0f, >test0f, <test10, >test10, <test11, >test11, <test12, >test12
              .byte <test13, >test13, <test14, >test14, <test15, >test15, <test16, >test16
              .byte $ff, $ff

expected1     .byte $81, $00, $00, $00, $00, $00, DUMMY, DUMMY    ; test01
              .byte $80, $00, $00, $00, $00, $00, DUMMY, DUMMY    ; test02
              .byte $00, $ff, $ff, $ff, $00, $81, DUMMY, DUMMY    ; test03
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test04
              .byte $ff, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test05
              .byte $01, $01, $81, $ff, $81, $81, DUMMY, DUMMY    ; test06
              .byte $01, $01, $81, $ff, $81, $81, DUMMY, DUMMY    ; test07
              .byte $01, $01, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test08
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test09
              .byte $01, $01, $81, $ff, $81, $81, DUMMY, DUMMY    ; test0a
              .byte $01, $01, $01, $01, $01, $01, DUMMY, DUMMY    ; test0b   ; inc $dd0d
              .byte $01, $01, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test0c   ; inc $dd0d,x
              .byte $01, $ff, $ff, $ff, $01, $81, DUMMY, DUMMY    ; test0d   ; inc $dd0d,x
              .byte $01, $01, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test0e   ; inc $dd0d,x
              .byte $01, $01, $01, $01, $01, $01, DUMMY, DUMMY    ; test0f   ; inc $dd0d,x
              .byte $01, $01, $ff, $ff, $01, $81, DUMMY, DUMMY    ; test10   ; inc $dd0d,x
              .byte $01, $01, $01, $01, $01, $01, DUMMY, DUMMY    ; test11   ; inc $dd0d,x
              .byte $01, $01, $01, $01, $01, $81, DUMMY, DUMMY    ; test12   ; inc $dd0d,x
              .byte $01, $01, $01, $81, $81, $81, DUMMY, DUMMY    ; test13   ; inc $dd0d,x
              .byte $01, $01, $01, $01, $01, $01, DUMMY, DUMMY    ; test14   ; inc $dd0d,x
              .byte $01, $01, $00, $81, $81, $81, DUMMY, DUMMY    ; test15
              .byte $01, $01, $01, $01, $01, $01, DUMMY, DUMMY    ; test16

              .byte $ff, $00, $ff, $ff, $ff, $ff, DUMMY, DUMMY    ; test17
              .byte $ff, $00, $ff, $ff, $ff, $ff, DUMMY, DUMMY    ; test18
              .byte $02, $ff, $ff, $ff, $ff, $ff, DUMMY, DUMMY    ; test19

expected2     .byte $80, $00, $00, $00, $00, $00, DUMMY, DUMMY    ; test01
              .byte $80, $00, $00, $00, $00, $00, DUMMY, DUMMY    ; test02
              .byte $00, $ff, $ff, $ff, $00, $81, DUMMY, DUMMY    ; test03
              .byte $ff, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test04
              .byte $ff, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test05
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test06
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test07
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test08
              .byte $ff, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test09
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test0a
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test0b   ; inc $dd0d
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test0c   ; inc $dd0d,x
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test0d   ; inc $dd0d,x
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test0e   ; inc $dd0d,x
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test0f   ; inc $dd0d,x
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test10   ; inc $dd0d,x
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test11   ; inc $dd0d,x
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test12   ; inc $dd0d,x
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test13   ; inc $dd0d,x
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test14   ; inc $dd0d,x
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test15
              .byte $81, $ff, $ff, $ff, $81, $81, DUMMY, DUMMY    ; test16

              .byte $82, $00, $ff, $ff, $ff, $ff, DUMMY, DUMMY    ; test17
              .byte $82, $ff, $ff, $ff, $ff, $ff, DUMMY, DUMMY    ; test18
              .byte $82, $ff, $ff, $ff, $ff, $ff, DUMMY, DUMMY    ; test19

output        .byte $00, $00, $00, $00, $00