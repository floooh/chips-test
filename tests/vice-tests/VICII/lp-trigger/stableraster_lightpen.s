
        .export rastersync_lp

        .align 256

; the lightpen is usually connected to pin 6 of joystick port 1, which is the
; same as used for fire on a regular joystick (and space on the keybpard)
; this line is then directly connected to both the cia (bit 4 of cia1 port B)
; and the lightpen input of the vic, which means that the lightpen line of the
; vic can be artificially written to by toggling said cia port bit.

rastersync_lp:

.scope
         ; acknowledge vic irq
;         lda $d019
;         sta $d019

         ldx #$ff
         ldy #$00
         ; prepare cia ports
         stx $dc00     ; port A = $ff (inactive)
         sty $dc02     ; ddr A = $00  (all input)
         stx $dc03     ; ddr B = $ff  (all output)
         stx $dc01     ; port B = $ff (inactive)
         ; now trigger the lp latch
         sty $dc01     ; port B = $00 (active)
         stx $dc01     ; port B = $ff (inactive)
         lda $d013     ; get x-position (pixels, divided by two)
         ; restore cia setup
         stx $dc02     ; ddr A = $ff  (all output)
         sty $dc03     ; ddr B = $00  (all input)
         stx $dc01     ; port B = $ff (inactive)
         ldx #$7f
         stx $dc00     ; port A = $7f

         ; divide x-pos by 4 to get x-position in cycles (0..62)
         lsr a
         lsr a
         ; delay 62 - n cycles
         lsr a
         sta timeout+1
         bcc timing   ; 2, +1 extra cycle if even
timing:  clv
timeout: bvc timeout
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

         rts

.endscope
