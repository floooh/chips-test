; irqdummy
; --------
; 2010 Hannu Nuotio
; Test code based on:
;   http://bel.fi/~alankila/irqack/
; ...which is a fixed version of Freshness79's IRQ test in:
;   http://noname.c64.org/csdb/forums/?roomid=11&topicid=79331&showallposts=1

; Checks if interrupt sequence dummy loads are from PC only (not PC+1)

!ct scr


; --- Consts

screen = $0400


; --- Variables

; - zero page

strptr = $fb            ; pointer to string
scrptr = $fd            ; pointer to screen


; --- Code

* = $0801
basic:
; BASIC stub: "1 SYS 2061"
!by $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00

; -- Main

entrypoint:
    lda #4 ; start with 4 so we will see green (=5) on success
    sta $d020
    ; setup
    jsr setup
main_testloop:
    ; setup next test
    jsr test_setup
    beq main_exit
    ; do test
    jsr test_entry
    ; print results
    jsr display_results
    jmp main_testloop

    ; exit
main_exit:
    ; print end text
    lda #>message_end
    sta strptr+1
    lda #<message_end
    sta strptr
    jsr print_text

    inc $d020

    lda $d020
    and #$0f
    ldx #0 ; success
    cmp #5
    beq nofail
    ldx #$ff ; failure
nofail:
    stx $d7ff

-   jmp -


; -- Subroutines

; - test_setup
; parameters:
;  test_num = test to run
; returns:
;  test_num = next test
;  Z = no more tests
;
test_setup:
    lda test_num

    ; point y to testdata
    asl
    asl
    tay

    ; check if no test
    lda testdata,y
    bne test_setup_test_exists
    ; no test, Z = 1
    rts

test_setup_test_exists:
    ; print test #
    lda test_num
    ldx #3
    jsr printhex

    ; setup test params
    iny
    lda testdata,y
    sta test_addr_lsb
    iny
    lda testdata,y
    sta test_addr_msb
    iny
    lda testdata,y
    sta test_setup_ref

    ; print test params
    lda test_addr_msb
    ldx #2
    jsr printhex
    lda test_addr_lsb
    ldx #3
    jsr printhex
test_setup_ref = * + 1
    lda #$00
    ldx #4
    jsr printhex

test_setup_exit:
    ; wait for line 256
-   lda $d011
    bmi -
-   lda $d011
    bpl -

    ; next test (and Z = 0)
    inc test_num
    rts

; - test_entry
; parameters:
;  test_addr_lsb = test address MSB
;  test_addr_msb = test address LSB
; returns:
;  result_icr = ICR value at IRQ handler
;  result_msb = stored PC MSB
;  result_lsb = stored PC LSB
;
test_entry:
    sei
    lda #$7f
    sta $dc0d
    sta $dd0d
    lda #$81
    sta $dc0d
    lda #<test_irq
    sta $0314
    lda #>test_irq
    sta $0315
    lda $dc0d
    lda $dd0d
    lda #$00
    sta $dc0e
    sta $dc0f
    lda #$02
    ldx #$00
    cli
    sta $dc04
    stx $dc05
    lda #$09
test_start:
    sta $dc0e
    sta $ff
    ; IRQ fires right after this jump
test_addr_lsb = * + 1
test_addr_msb = * + 2
    jmp $1234
-   jmp wrong_timing
    jmp -

test_irq:
    ; throw away A, X, Y, status
    pla
    pla
    pla
    pla

test_get_results:
    ; get and store MSB, LSB
    pla
    sta result_lsb
    pla
    sta result_msb
    ; ACK IRQ, store ICR status
    lda $dc0d
    sta result_icr

test_done:
    ; restore kernal IRQ handler
    lda #$31
    sta $0314
    lda #$ea
    sta $0315

    rts

wrong_timing:
    inc $d020
    jmp wrong_timing


; - display_results
;
display_results:
    ; print results
    lda result_msb
    ldx #2
    jsr printhex
    lda result_lsb
    ldx #3
    jsr printhex
    lda result_icr
    ldx #23
    jsr printhex
    rts


; - setup
;
setup:
    lda #0
    sta test_num

    ; set screen color
    lda 646
    ldx #0
-   sta $d800,x
    sta $d900,x
    sta $da00,x
    sta $dae8,x
    inx
    bne -

    ; clear screen
    lda #' '
    ldx #0
-   sta screen,x
    sta screen+$100,x
    sta screen+$200,x
    sta screen+$2e8,x
    inx
    bne -

    ; print some text
    lda #>screen
    sta scrptr+1
    lda #<screen
    sta scrptr
    lda #>message_start
    sta strptr+1
    lda #<message_start
    sta strptr
    jsr print_text
    rts


; - print_text
; parameters:
;  scrptr -> screen location to print to
;  strptr -> string to print
;
print_text:
    ldy #0
-   lda (strptr),y
    beq +++         ; end if zero
    sta (scrptr),y  ; print char
    iny
    bne -           ; loop if not zero
    inc strptr+1
    inc scrptr+1
    bne -
+++
    clc
    tya             ; update scrptr to next char
    adc scrptr
    sta scrptr
    bcc +
    inc scrptr+1
+
    rts             ; return

; - printhex
; parameters:
;  scrptr -> screen location to print to
;  a = value to print
;  x = amount to move scrptr
; returns:
;  scrptr += y
;
printhex:
    sty printhex_y_store
    pha
    stx printhex_move
    pha
    ; mask lower
    and #$0f
    ; lookup
    tax
    lda hex_lut,x
    ; print
    ldy #1
    sta (scrptr),y
    ; lsr x4
    pla
    lsr
    lsr
    lsr
    lsr
    ; lookup
    tax
    lda hex_lut,x
    ; print
    dey
    sta (scrptr),y
    ; scrptr += y
    lda scrptr
    clc
printhex_move = * + 1
    adc #$02
    sta scrptr
    bcc +
    inc scrptr+1
+   pla
printhex_y_store = * + 1
    ldy #$00
    rts


; --- Data

test_num: !by 0

result_lsb: !by 0
result_msb: !by 0
result_icr: !by 0

; hex lookup table
hex_lut: !scr "0123456789abcdef"

; - text

message_start:
;     |---------0---------0---------0--------|
!scr "irqdummy v2                             "
!scr "                                        "
!scr "this testprog checks if the interrupt   "
!scr "sequence dummy loads happen from pc & pc"
!scr "by jumping to the test addresses and    "
!scr "having the cia1 irq trigger there.      "
!scr "                                        "
!scr "the ref/result is the value of cia1 icr "
!scr "as read on the irq handler; if 0, it was"
!scr "already read by one of the dummy loads. "
!scr "                                        "
!scr "00 on dc0d: a dummy load is from pc     "
!scr "81 on dc0c: no dummy load from pc+1     "
!scr "-> dummy loads are from pc & (likely) pc"
!scr "                                        "
!scr "  reference / results                   "
!scr "#  addr icr/addr icr                    "
!by 0

message_end:
;     |---------0---------0---------0--------|
!scr "                                        "
!scr "all tests done.                         "
!by 0


; - tests
; format: nn ll mm rr
;  nn = test exists (0 to end list)
;  ll = LSB
;  mm = MSB
;  rr = expected results
;
testdata:
!by 1, $00, $60, $81
!by 1, $0d, $dc, $00
!by 1, $0c, $dc, $81
!by 0
