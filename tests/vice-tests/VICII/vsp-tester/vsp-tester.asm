         * = $20;
atemp	.byte ?

        test_row = 2
        test_col = 39
        vsp_irq_d012 = $30+test_row
        open_border_d012 = $fa
        den_test = 0

        hex_out_loc = $424

        ;; PRG header
        *= $0801

*       = $0801
        .word (+), 2005  ;pointer, line number
        .null $9e, format("%4d", start);will be sys 4096
+       .word 0          ;basic line end
vsp_irq
        sta atemp
        lda $dd04        ; read timer
        eor #$3f        ; invert to determine required delay
        sta mod_int_delay+1
mod_int_delay
        bne *
        .fill $3d, $80	; NOP #
        .byte $04	; NOP zp
.if NTSC = 1
        nop
.endif
        nop
        lda #$18+test_row
        sta $d011
        inc $d011

        lda #<open_border
        sta $fffe
        lda #open_border_d012
        sta $d012
        dec $d019
        lda atemp
        rti
open_border
        sta atemp
        lda #<vsp_irq
        sta $fffe
        lda #vsp_irq_d012
        sta $d012
	.if den_test
        lda #0
	.else
        lda #$10+test_row+1
	.fi
        sta $d011
        dec $d019
        inc frame
        lda atemp
        rti

start
;        jsr $e544
        sei ; kill interrupts while we fuck with shit
sync
        lax $dc04   ; routine to sync with raster x
        sbx #51
        sta $2
        cpx $dc04
        bne sync    ; when this drops through we are at x = $1e0

.if NTSC = 1
        lda #64        	; 65 cycles to match NTSC VIC
.else
        lda #62        	; 63 cycles to match PAL VIC
.endif
        sta $dd04
        lda #0
        sta $dd05

        ldx #3        	; delay here shifts sync to where we want
-        dex
        bne -
        nop
        nop
        lda #$11
        sta $dd0e        ; start timer

        lda #1        	; change text colour to white
        ldx #0
-        sta $d800,x
        sta $d900,x
        sta $da00,x
        sta $db00,x
        inx
        bne -

        lda #$f        	; set background to lt grey
        sta $d021
        
        lda #test_sprite>>6
        ldx #7
-        sta $7f8,x        ; set sprite pointers to single pixel
        dex
        bpl-

        lda #$ff
        sta $d01b        ; sprites behind gfx
        sta $d010        ; sprites X MSB
        ldx #$48        ; sprites X LSB
        ldy #14
-        txa
        sta $d000,y
        lda #$2f+test_row	; sprites Y
        sta $d001,y
        inx
        dey
        dey
        bpl -

        lda #%00110101         ; BASIC and KERNAL rom off
        sta $01        	
        lda #%01111111 	
        sta $DC0D        ;"Switch off" interrupts signals from CIA-1
        sta $DD0D        ;"Switch off" interrupts signals from CIA-2
        lda $DC0D        ;Clear any pending IRQs
        lda $DD0D        ;

        lda #<open_border
        sta $fffe
        lda #>open_border
        sta $ffff
        lda #open_border_d012	
        sta $d012
        lda #$1b
        sta $d011        ;Clear IRQ raster MSB

        lda #1
        sta $D01A 	;Enable raster interrupt signals from VIC
        dec $d019	;Clear any pending VIC IRQ

        cli
        
        lda #$ff	;Enable sprites early, so MCBASE is corrected
        sta $d015
        jsr fill_h	;Fill VIC bank with MSB
        lda #0        ;Disable sprites
        sta $d015	
-        bit $d011	;Wait for bottom of screen
        bpl -
-        bit $d011	;Wait for top of screen
        bmi -
        lda #$ff	;Enable sprites
        sta $d015
        lda $d01f	;Clear collision register
-        bit $d011	;Wait for bottom of screen
        bpl -

        lda $d01f	;Read collision register to obtain MSB
        ldy #0
        sta mod_animate+2
        jsr print_byte	;Print MSB on screen

        lda #0        ;Disable sprites
        sta $d015	
        jsr fill_l	;Fill VIC bank with LSB
-        bit $d011	;Wait for bottom of screen
        bpl -
-        bit $d011	;Wait for top of screen
        bmi -
        lda #$ff	;Enable sprites
        sta $d015	
        lda $d01f	;Clear collision register
-        bit $d011	;Wait for bottom of screen
        bpl -

        lda $d01f	;Read collision register to obtain LSB
        sta mod_animate+1
        ldy #2
        jsr print_byte	;Print LSB on screen

        lda #0        ;Disable sprites
        sta $d015
animate
        dex        ;Kill some time
        bne animate
        dey
        bne animate
mod_animate
        inc $3fff	;This address is modified by code above
        lda #1
        sta $3fff	;Clear out the normal idle byte, so it is never animated

        lda frame
        and #$f0
        cmp #$40
        bne +
        jsr checkresult
+
        jmp animate

fill_h
        ldx #0        ; Fill memory up to $3fff with the MSB of the address
-        lda mod_h+2
mod_h
        sta test_area,x
        inx
        bne mod_h
        inc mod_h+2
        cmp #$3f
        bne -
        rts

checkresult:
        ; 33 06 06 06   $3fff   $38ff $3807 $38c7 $38d7
        ldx #$ff    ; failure
        ldy #10     ; red

        lda hex_out_loc
        cmp #$33    ; 3
        bne notok
        lda hex_out_loc+1
        cmp #$38    ; 8
        bne notok

        lda hex_out_loc+2
        cmp #$06    ; f
        bne +
        lda hex_out_loc+3
        cmp #$06    ; f
        bne +
        jmp isok
+
        lda hex_out_loc+2
        cmp #$30    ; 0
        bne +
        lda hex_out_loc+3
        cmp #$37    ; 7
        bne +
        jmp isok
+
        lda hex_out_loc+2
        cmp #$33    ; c
        bne +
        lda hex_out_loc+3
        cmp #$37    ; 7
        bne +
        jmp isok
+
        lda hex_out_loc+2
        cmp #$04    ; d
        bne +
        lda hex_out_loc+3
        cmp #$37    ; 7
        bne +
        jmp isok
+       jmp notok

isok
        ldx #$00    ; ok
        ldy #$0d    ; green
notok
        stx $d7ff
        sty $d020
        rts

waitframe:
        lda frame
-       cmp frame
        beq -
        rts
frame:  .byte 0
;------------------------------------------------------------------------------
fill_l
        ldx #0        ; Fill memory up to $3fff with the LSB of the address
-
        txa
mod_l
        sta test_area,x
        inx
        bne -
        inc mod_l+2
        lda mod_l+2
        cmp #$40
        bne -
        rts

print_byte        	; Print a byte on screen indented by Y
        tax
        lsr
        lsr
        lsr
        lsr
        jsr print_digit
        iny
        txa
        and #$f
print_digit        	; Print a hex digit on screen indented by Y
        clc
        adc #48
        cmp #58
        bcc +
        sbc #57
+
        sta hex_out_loc,y
        rts
;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
.align 64
test_sprite
        .byte $80	; Single set pixel
        .fill 62,0
end
test_area = ((end+255)/256)*256	; Round up to next page

