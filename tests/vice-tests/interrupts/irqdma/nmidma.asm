; --- Defines

reference_data = $2000


; --- Code

*=$0801
basic:
; BASIC stub: "1 SYS 2061"
!by $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00

start:
    ldx #0
clp:
    lda #$20
    sta $0400,x
    sta $0500,x
    sta $0600,x
    sta $06e8,x
    lda #$01
    sta $d800+(24*40),x
    inx
    bne clp
    ldx #0
cl:
    lda testtxt,x
    beq sk
    sta $0400+(24*40),x
    inx
    bne cl
sk:
    jmp entrypoint

testtxt:
    !scr "nmi"
    !byte $30+TESTNUM
    !if (B_MODE = 1) {
        !scr "b"
    }
    !byte 0

* = $0900
entrypoint:
    sei
    lda #4 ; start with 4 so we will see green (=5) on success
    sta $d020
    lda #$7f
    sta $dc0d
    sta $dd0d
    lda #$00
    sta $dc0e
    sta $dc0f
    lda $dc0d
    lda $dd0d
    lda #$35
    sta $01
    lda #<irq_handler
    sta $fffe
    lda #>irq_handler
    sta $ffff
    lda #$1b
    sta $d011
    lda #$46
    sta $d012
    lda #$01
    sta $d01a
    sta $d019
    lda #$64
    sta $d000
    sta $d002
    sta $d004
    sta $d006
    sta $d008
    sta $d00a
    sta $d00c
    sta $d00e
    lda #$4a
    sta $d001
    sta $d003
    sta $d005
    sta $d007
    sta $d009
    sta $d00b
    sta $d00d
    sta $d00f
    lda #$00
    sta $d010
    lda #$00
    sta $f5

!if B_MODE = 1 {
    lda #$40
    sta $f7
    lda #$04
} else {
    lda #$00
}
    sta $f8

    lda #<reference_data
    sta $fa
    ldy #$03
    jsr printhex
    lda #>reference_data
    sta $fb
    ldy #$01
    jsr printhex
    cli
entry_loop:
    jmp entry_loop

irq_handler:
    lda #<irq_handler_2
    sta $fffe
    lda #>irq_handler_2
    sta $ffff
    lda #$01
    sta $d019
    ldx $d012
    inx
    stx $d012
    cli
    ror $02
    ror $02
    ror $02
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
    rti

irq_handler_2:
    lda #$01
    sta $d019
    ldx #$07
irq_handler_2_wait_1:
    dex
    bne irq_handler_2_wait_1
    nop
    nop
    lda $d012
    cmp $d012
    beq irq_handler_2_skip_1
irq_handler_2_skip_1:
    nop
    nop
    lda $f5
    bne cia_ok
    jsr testcia
    jmp irq_handler_2_finish_test

cia_ok:    
    ldx #$04
irq_handler_2_wait_2:
    dex
    bne irq_handler_2_wait_2
    inc $d021
    dec $d021
    lda #<nmi_handler_3
    sta $fffa
    lda #>nmi_handler_3
    sta $fffb
    lda #$46
    sta $d012
    cli
    lda $f8
    sta $d015
    lda #$ff
    sta $dc04
    lda #$00
    sta $dc05
    lda #$ff
    sta $dc06
    lda #$00
    sta $dc07
    lda $f9
    sta $dd04
    lda #$00
    sta $dd05
    lda #$81
    sta $dd0d
    lda #$19
    sta $dc0e
    sta $dc0f
    sta $dd0e

!if B_MODE = 1 {
    lda $f7
    jsr delay
}

!if TESTNUM = 1 {
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
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
}

!if TESTNUM = 2 {
    ldx #$00
    lda ($00,x)
    lda ($00,x)
    lda ($00,x)
    lda ($00,x)
    lda ($00,x)
    lda ($00,x)
    lda ($00,x)
    lda ($00,x)
    lda ($00,x)
    lda ($00,x)
    lda ($00,x)
    lda ($00,x)
    lda ($00,x)
    lda ($00,x)
    lda ($00,x)
    lda ($00,x)
    lda ($00,x)
    lda ($00,x)
    lda ($00,x)
}

!if TESTNUM = 3 {
    ldx #$00
    inc $ff00,x
    inc $ff00,x
    inc $ff00,x
    inc $ff00,x
    inc $ff00,x
    inc $ff00,x
    inc $ff00,x
    inc $ff00,x
    inc $ff00,x
    inc $ff00,x
    inc $ff00,x
    inc $ff00,x
    inc $ff00,x
    inc $ff00,x
    inc $ff00,x
    inc $ff00,x
    inc $ff00,x
    inc $ff00,x
    inc $ff00,x
}

!if TESTNUM = 4 {
    ldx #$00
    jsr dummy_rts
    jsr dummy_rts
    jsr dummy_rts
    jsr dummy_rts
    jsr dummy_rts
    jsr dummy_rts
    jsr dummy_rts
    jsr dummy_rts
    jsr dummy_rts
    jsr dummy_rts
    jsr dummy_rts
}

!if TESTNUM = 5 {
    ldx #$00
    beq lc113
lc113:
    beq lc115
lc115:
    beq lc117
lc117:
    beq lc119
lc119:
    beq lc11b
lc11b:
    beq lc11d
lc11d:
    beq lc11f
lc11f:
    beq lc121
lc121:
    beq lc123
lc123:
    beq lc125
lc125:
    beq lc127
lc127:
    beq lc129
lc129:
    beq lc12b
lc12b:
    beq lc12d
lc12d:
    beq lc12f
lc12f:
    beq lc131
lc131:
    beq lc133
lc133:
    beq lc135
lc135:
    beq lc137
lc137:
    beq lc139
lc139:
    beq lc13b
lc13b:
    beq lc13d
lc13d:
    beq lc13f
lc13f:
    beq lc141
lc141:
    beq lc143
lc143:
    beq lc145
lc145:
    beq lc147
lc147:
    beq lc149
lc149:
    beq lc14b
lc14b:
    beq lc14d
lc14d:
    beq lc14f
lc14f:
    beq lc151
lc151:
    beq lc153
lc153:
    beq lc155
lc155:
    beq lc157
lc157:
    beq lc159
lc159:
    beq lc15b
lc15b:
    beq lc15d
lc15d:
    beq lc15f
lc15f:
    beq lc161
lc161:
    beq lc163
lc163:
    beq lc165
lc165:
    beq lc167
lc167:
    beq lc169
lc169:
}

!if TESTNUM = 6 {
    ldx #$00
    sei
    php
    cli
    plp
    cli
    sei
    php
    cli
    plp
    cli
    sei
    php
    cli
    plp
    cli
    sei
    php
    cli
    plp
    cli
    sei
    php
    cli
    plp
    cli
    sei
    php
    cli
    plp
    cli
    sei
    php
    cli
    plp
    cli
    sei
    php
    cli
    plp
    cli
}

    lda $dc06
    pha
    tya
    pha
    ldy #$00

    ; 1 = test, 0 = record
    lda #TEST_MODE
    cmp #$01
    beq irq_handler_2_test_mode

    ; store values
    pla
    sta ($fa),y
    iny
    pla
    sta ($fa),y
    jmp irq_handler_2_next

irq_handler_2_test_mode:

    ; compare values with reference data
    pla
    cmp ($fa),y
    bne irq_handler_2_test_failed
    iny
    pla
    cmp ($fa),y
    bne irq_handler_2_test_failed
    jmp irq_handler_2_next

irq_handler_2_test_failed:
    sei
    inc $d020
    jmp failed

irq_handler_2_next:
    lda $fa
    clc
    adc #$02
    sta $fa
    lda $fb
    adc #$00
    sta $fb
    ; show current addr
    ldy #$01
    jsr printhex
    lda $fa
    ldy #$03
    jsr printhex

!if B_MODE = 1 {
    dec $f7
    lda $f7
    cmp #$38
    bne irq_handler_2_finish_test

    lda #$40
    sta $f7
}

    inc $f9
    lda $f9
    cmp $f6
    bne irq_handler_2_finish_test

    lda $f5
    sta $f9
    inc $f8
    lda $f8

!if B_MODE = 1 {
    cmp #$14
} else {
    cmp #$80
}
    bne irq_handler_2_finish_test

    sei
    inc $d020
irq_handler_2_all_tests_successful:

    lda $d020
    and #$0f
    ldx #0 ; success
    cmp #5
    beq nofail
    ldx #$ff ; failure
nofail:
    stx $d7ff
    
    jmp irq_handler_2_all_tests_successful

irq_handler_2_finish_test:
    lda #$00
    sta $dc0e
    lda #$7f
    sta $dc0d
    lda $dc0d
    lda #<irq_handler
    sta $fffe
    lda #>irq_handler
    sta $ffff
    rti

nmi_handler_3:
    bit $dd0d
    ldy $dc04
    lda #$19
    sta $dc0f
    rti

!if TESTNUM = 4 {
dummy_rts:
    rts
}


testcia:

    lda #<nmi_handler_testcia
    sta $fffa
    lda #>nmi_handler_testcia
    sta $fffb
    lda #$46
    sta $d012
    cli
    lda #$2b
    sta $dc04
    lda #$00
    sta $dc05
    lda #$10
    sta $dd04
    lda #$00
    sta $dd05
    lda #$81
    sta $dd0d
    lda #$19
    sta $dc0e
    sta $dc0f
    sta $dd0e

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

nmi_handler_testcia:
    bit $dd0d
    ldy $dc04
    lda #$19
    sta $dc0f
    tya
    lsr
    clc
!if B_MODE = 1 {
    adc #$30
} else {
    adc #$10
}
    sta $f5
    sta $f9
    clc
    adc #$80
    sta $f6
    
    tya
    lsr
    asl
    asl
    asl
    tay
    ldx #$00

loopoutput:    
    lda ciatype,y
    sta $0420,x
    lda #$01
    sta $d820,x
    iny
    inx
    cpx #$08
    bne loopoutput
    rti
    
ciatype:
    !scr "old cia "
    !scr "new cia "

printhex:
    pha
    ; mask lower
    and #$0f
    ; lookup
    tax
    lda hex_lut,x
    ; print
    sta $0400,y
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
    sta $0400,y
    rts

; hex lookup table
hex_lut: 
!scr "0123456789abcdef"

failed:
  cpy #$01
  bne failvalue
  inc $0403
failvalue:
  pha
  lda ($fa),y
  ldy #$09
  jsr printhex
  pla
  ldy #$06
  jsr printhex
failloop:
  inc $d020
  lda #$ff
  sta $d7ff
  jmp failloop  
  
!if B_MODE = 1 {
* = $0850
delay:              ;delay 80-accu cycles, 0<=accu<=64
    lsr             ;2 cycles akku=akku/2 carry=1 if accu was odd, 0 otherwise
    bcc waste1cycle ;2/3 cycles, depending on lowest bit, same operation for both
waste1cycle:
    sta smod+1      ;4 cycles selfmodifies the argument of branch
    clc             ;2 cycles
;now we have burned 10/11 cycles.. and jumping into a nopfield 
smod:
    bcc *+10
!by $EA,$EA,$EA,$EA,$EA,$EA,$EA,$EA
!by $EA,$EA,$EA,$EA,$EA,$EA,$EA,$EA
!by $EA,$EA,$EA,$EA,$EA,$EA,$EA,$EA
!by $EA,$EA,$EA,$EA,$EA,$EA,$EA,$EA
    rts             ;6 cycles
}

!if TEST_MODE = 1 {
    * = $2000
    !if B_MODE = 0 {
        !if TESTNUM = 1 {
            !bin "dumps/nmi1.dump",,2
        }
        !if TESTNUM = 2 {
            !bin "dumps/nmi2.dump",,2
        }
        !if TESTNUM = 3 {
            !bin "dumps/nmi3.dump",,2
        }
        !if TESTNUM = 4 {
            !bin "dumps/nmi4.dump",,2
        }
        !if TESTNUM = 5 {
            !bin "dumps/nmi5.dump",,2
        }
        !if TESTNUM = 6 {
            !bin "dumps/nmi6.dump",,2
        }
        !if TESTNUM = 7 {
            !bin "dumps/nmi7.dump",,2
        }
    }
    !if B_MODE = 1 {
        !if TESTNUM = 1 {
            !bin "dumps/nmi1b.dump",,2
        }
        !if TESTNUM = 2 {
            !bin "dumps/nmi2b.dump",,2
        }
        !if TESTNUM = 3 {
            !bin "dumps/nmi3b.dump",,2
        }
        !if TESTNUM = 4 {
            !bin "dumps/nmi4b.dump",,2
        }
        !if TESTNUM = 5 {
            !bin "dumps/nmi5b.dump",,2
        }
        !if TESTNUM = 6 {
            !bin "dumps/nmi6b.dump",,2
        }
        !if TESTNUM = 7 {
            !bin "dumps/nmi7b.dump",,2
        }
    }
}

