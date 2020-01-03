; VIC-20 test program for interrupts firing without enabling them
; TESTVIA = 2 fails on VICE 3.0 xvic, works on VICE 2.4
; (untested on HW by author, hopefully by submitter)
; for +8k

!if TESTVIA = 1 {
VIA = $9110
} else {
VIA = $9120
}

COLOR_WHITE = 1
COLOR_RED = 2
COLOR_GREEN = 5

vmatrix = $1000

int_counter = vmatrix + 0
int_flag = vmatrix + 1

b_strout_ay = $cb1e

* = $1201
basic_start:
!wo basic_stop
!wo 0
!pet $9e, "4621", 0
basic_stop:
!wo 0

mlentry:
    ; setup screen
    lda #<text
    ldy #>text
    jsr b_strout_ay
    jmp phase0_init


int_handler:
    pha
    lda VIA + $4   ; ACK interrupt
    inc int_counter
    lda #$ff
    sta int_flag
    pla
int_exit:
!if TESTVIA = 2 {
    pla
    tay
    pla
    tax
    pla
}
    rti


main:
    ; wait for int to happen
    ldx #2
    lda #100
--  cmp $9004
    beq --
-   cmp $9004
    bne -
    dex
    bne --
    ; check if interrupt happened and int_handler set the flag
    lda #(COLOR_GREEN | 8) | (COLOR_WHITE << 4)
    bit int_flag
    bpl +
    lda #(COLOR_RED | 8) | (COLOR_WHITE << 4)
+   sta $900f
test_done:

    ;lda $900f
    ; store value to "debug cart"
    ldy #0 ; success
    cmp #(COLOR_GREEN | 8) | (COLOR_WHITE << 4)
    beq +
    ldy #$ff ; failure
+
    sty $910f

-   jmp -


phase0_init:
    sei
    ldx #$ff
    txs
    cld
    ; disable interrupts
    lda #$7f
    sta $913d
    sta $913e

    ; initialize VIA
    lda #0
    ldx #$a
-   sta $9132,x
    dex
    bpl -

    ; clear test data
    lda #0
    sta int_counter
    sta int_flag

    ; detect pal/ntsc
    lda $ede4
    and #$01
    tax ; 0 -> pal, 1 -> ntsc

    ; enable Timer A free run on VIA $(TESTVIA)
    lda #$40
    sta VIA + $b

    lda #<int_handler
    ldy #>int_handler
!if TESTVIA = 1 {
    sta $0318
    sty $0319
} else {
    sta $0314
    sty $0315
}

    lda tv_tbl_raster,x
    pha
    lda tv_tbl_timer_l,x
    tay
    lda tv_tbl_timer_h,x
    tax
    ; synchronise with the screen
    pla
-   cmp $9004
    beq -
-   cmp $9004
    bne -

    sty VIA + $6
    stx VIA + $5

    lda #$c0
    cli
    ; enable interrupt
    ; NOTE: even as this is commented out, xvic from VICE 3.0 fires the interrupts! (for VIA2)
    ; sta VIA + $e
    jmp main


; tbl: index 0 -> pal, 1 -> ntsc
tv_tbl_raster:
!by 131, 118
tv_tbl_timer_l:
!by <(312 * 71 - 2), <(261 * 65 - 2)
tv_tbl_timer_h:
!by >(312 * 71 - 2), >(261 * 65 - 2)


text:
!by $93 ; clear
;     0123456789012345678901
!pet "   <- int counter&flag"
!pet "no ints should happen "
!pet "border green if ok    "
!if TESTVIA = 1 {
!pet "test nmi via1 $9110"
} else {
!pet "test irq via2 $9120"
}
!by 0
