
zpage = $02

viabase = $9110


charsperline = 22

!if (expanded=1) {
basicstart = $1201      ; expanded
screenmem = $1000
colormem = $9400
} else {
basicstart = $1001      ; unexpanded
screenmem = $1e00
colormem = $9600
}

                * = basicstart
                !word basicstart + $0c
                !byte $0a, $00
                !byte $9e
!if (expanded=1) {
                !byte $34, $36, $32, $32
} else {
                !byte $34, $31, $31, $30
}
                !byte 0,0,0

                * = basicstart + $0d
start:
                sei
                ldx #$ff
                txs

                lda #0
                sta $900f
                jsr initscreen

            lda #(mode<<2)    ; SR mode
            sta zpage+0
            lda #%00000001    ; SR pattern
            sta zpage+1
            jsr dotest

                jsr comparescreen
lp
            jsr showreg
            lda #0
            sta $9120
            lda $9121
            cmp #$ff
            beq lp

            jsr swapscrn

-           lda $9121
            cmp #$ff
            bne - 
            
            jsr swapscrn
            jmp lp

showreg:
            !for n, charsperline {
                lda viabase + $0a     ; SR
                sta screenmem+(charsperline*19)+n-1
            }
            rts
swapscrn:
            ldx #0
-
            lda screenmem,x
            pha
            lda refdata,x
            sta screenmem,x
            pla
            sta refdata,x
            inx
            bne -
            ldx #0
-
            lda screenmem+$100,x
            pha
            lda refdata+$100,x
            sta screenmem+$100,x
            pla
            sta refdata+$100,x
            inx
            cpx #$84
            bne -
            rts
                
dotest:
            ; clear all registers twice to make sure they are all 0
            jsr resetvia
            jsr resetvia

            !if (dumpreg = 10) {
            lda #7 
            sta viabase + 8     ; timer 2 L
            lda #0
            sta viabase + 9     ; timer 2 H

            lda #7 
            sta viabase + 4     ; timer 1 L
            lda #0
            sta viabase + 5     ; timer 1 H
            }

            !if (dumpreg = 13) {
            lda #$80
            sta viabase + 8     ; timer 2 L
            lda #0
            sta viabase + 9     ; timer 2 H

            lda #$c0
            sta viabase + 4     ; timer 1 L
            lda #0
            sta viabase + 5     ; timer 1 H
            }
            !if (dumpreg = 13) {
            lda #0
            sta viabase + $b    ; IER
            lda viabase + $1    ; ORA
            lda viabase + $0    ; ORB
            ;stx viabase + $A    ; SR
            }
            
            ldx zpage + 1    ; pattern
            stx viabase + $A    ; SR
            
            lda zpage + 0    ; mode
            sta viabase + $b    ; ACR

            
            !for n, 128 {
                lda viabase + dumpreg     ; SR
                sta zpage + n + 10 - 1
            }
                ldx #0
-
                lda viabase + dumpreg     ; SR
                sta screenmem+(6*charsperline),x
                inx
                lda viabase + dumpreg     ; SR
                sta screenmem+(6*charsperline),x
                inx
                bne -

                ldx #0
-
                lda zpage + 10,x
                sta screenmem,x
                inx
                cpx #128
                bne -

            rts
resetvia:
            lda #0
            ldx #$0f
.lp1
            sta viabase,x
            dex
            bpl .lp1
            rts

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
                
                ldx #0
lp1a:
                lda reftitle,x
                sta screenmem+(charsperline*22),x
                lda #2
                sta colormem+(charsperline*22),x
                inx
                cpx #charsperline
                bne lp1a
                
                rts

comparescreen:
                lda #5 ; green
                sta failed+1

                ldx #0
lp2:
                ldy #5 ; green
                lda screenmem,x
                cmp refdata,x
                beq +
                ldy #2 ; red
                sty failed+1
+
                tya
                sta colormem,x

                inx
                bne lp2
                
                ldx #0
lp2a:
                ldy #5 ; green
                lda screenmem+($100),x
                cmp refdata+($100),x
                beq +
                ldy #2 ; red
                sty failed+1
+
                tya
                sta colormem+($100),x

                inx
                cpx #$84
                bne lp2a
                
failed:         lda #0
                sta $900f
                
                ; store value to "debug cart"
                ldy #0 ; success
                cmp #5 ; green
                beq +
                ldy #$ff ; failure
+
                sty $910f

                rts

reftitle:
!if (dumpreg = 10) {
!if (mode = 0) {
    !scr    "viasr00               "
}
!if (mode = 1) {
    !scr    "viasr04               "
}
!if (mode = 2) {
    !scr    "viasr08               "
}
!if (mode = 3) {
    !scr    "viasr0c               "
}
!if (mode = 4) {
    !scr    "viasr10               "
}
!if (mode = 5) {
    !scr    "viasr14               "
}
!if (mode = 6) {
    !scr    "viasr18               "
}
!if (mode = 7) {
    !scr    "viasr1c               "
}
}
!if (dumpreg = 13) {
!if (mode = 0) {
    !scr    "viasr00ifr            "
}
!if (mode = 1) {
    !scr    "viasr04ifr            "
}
!if (mode = 2) {
    !scr    "viasr08ifr            "
}
!if (mode = 3) {
    !scr    "viasr0cifr            "
}
!if (mode = 4) {
    !scr    "viasr10ifr            "
}
!if (mode = 5) {
    !scr    "viasr14ifr            "
}
!if (mode = 6) {
    !scr    "viasr18ifr            "
}
!if (mode = 7) {
    !scr    "viasr1cifr            "
}
}
                
refdata:

!if (dumpreg = 10) {
!if (mode = 0) {
                !binary "dump00.bin",,  ; all $01, nothing is shifted
}
!if (mode = 1) {
                !binary "dump04.bin",,
}
!if (mode = 2) {
                !binary "dump08.bin",,
}
!if (mode = 3) {
                !binary "dump00.bin",,  ; all $01, nothing is shifted
}
!if (mode = 4) {
                !binary "dump10.bin",,
}
!if (mode = 5) {
                !binary "dump14.bin",,
}
!if (mode = 6) {
                !binary "dump18.bin",,
}
!if (mode = 7) {
                !binary "dump00.bin",,  ; all $01, nothing is shifted
}
}

!if (dumpreg = 13) {
!if (mode = 0) {
                !binary "dump00i.bin",, 
}
!if (mode = 1) {
                !binary "dump00i.bin",,
}
!if (mode = 2) {
                !binary "dump00i.bin",,
}
!if (mode = 3) {
                !binary "dump00i.bin",, 
}
!if (mode = 4) {
                !binary "dump14i.bin",,
}
!if (mode = 5) {
                !binary "dump14i.bin",,
}
!if (mode = 6) {
                !binary "dump14i.bin",,
}
!if (mode = 7) {
                !binary "dump14i.bin",, 
}
}
