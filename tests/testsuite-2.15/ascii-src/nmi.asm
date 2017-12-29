
           *= $0801
           .byte $4c,$14,$08,$00,$97
turboass   = 780
           .text "780"
           .byte $2c,$30,$3a,$9e,$32,$30
           .byte $37,$33,$00,$00,$00
           lda #1
           sta turboass
           jmp main


print
           .block
           pla
           sta print0+1
           pla
           sta print0+2
           ldx #1
print0
           lda !*,x
           beq print1
           jsr $ffd2
           inx
           bne print0
print1
           sec
           txa
           adc print0+1
           sta print2+1
           lda #0
           adc print0+2
           sta print2+2
print2
           jmp !*
           .bend


printhb
           .block
           pha
           lsr a
           lsr a
           lsr a
           lsr a
           jsr printhn
           pla
           and #$0f
printhn
           ora #$30
           cmp #$3a
           bcc printhn0
           adc #6
printhn0
           jsr $ffd2
           rts
           .bend

starttimer brk
stoptimer  brk

intlow     .byte 0
inthigh    .byte 0
flagsbefore .byte 0
flagsafter .byte 0

setint
           .block
           pha
           lda #0
           sta $dd0e
           lda $dd0d
           lda #127
           sta $dc0d
           lda #129
           sta $dd0d
           clc
           pla
           adc #3
           sta $dd04
           lda #0
           sta $dd05
           lda #<nmi
           sta $fffa
           lda #>nmi
           sta $fffb
           lda #$35
           sta 1
           lda $d011
           bmi ok
wait
           lda $d012
           cmp #30
           bcs wait
ok
           lda #$19
           sta $dd0e
           rts
           .bend
nmi
           .block
           php
           bit $dd0d
           sta saveda+1
           stx savedx+1
           pla
           sta flagsafter
           tsx
           inx
           lda $0100,x
           sta flagsbefore
           inx
           lda $0100,x
           sta intlow
           inx
           lda $0100,x
           sta inthigh
savedx
           ldx #$11
saveda
           lda #$11
           rti
           .bend


restoreint
           lda #$37
           sta 1
           lda #129
           sta $dc0d
           rts


gsavesp    .byte 0
gastack    *= *+256

savestack
           .block
           tsx
           stx gsavesp
           ldx #0
save
           lda $0100,x
           sta gastack,x
           inx
           bne save
           rts
           .bend

restorestack
           .block
           pla
           sta return2+1
           pla
           sta return1+1
           ldx gsavesp
           inx
           inx
           txs
           ldx #0
restore
           lda gastack,x
           sta $0100,x
           inx
           bne restore
return1
           lda #$11
           pha
return2
           lda #$11
           pha
           rts
           .bend

addressing
           .word brkn
           .word rix
           .word hltn
           .word mix
           .word rz
           .word rz
           .word mz
           .word mz
           .word phpn
           .word b
           .word n
           .word b
           .word ra
           .word ra
           .word ma
           .word ma
           .word r
           .word riy
           .word hltn
           .word miy
           .word rzx
           .word rzx
           .word mzx
           .word mzx
           .word n
           .word ray
           .word n
           .word may
           .word rax
           .word rax
           .word max
           .word max
           .word jsrw
           .word rix
           .word hltn
           .word mix
           .word rz
           .word rz
           .word mz
           .word mz
           .word plpn
           .word b
           .word n
           .word b
           .word ra
           .word ra
           .word ma
           .word ma
           .word r
           .word riy
           .word hltn
           .word miy
           .word rzx
           .word rzx
           .word mzx
           .word mzx
           .word n
           .word ray
           .word n
           .word may
           .word rax
           .word rax
           .word max
           .word max
           .word rtin
           .word rix
           .word hltn
           .word mix
           .word rz
           .word rz
           .word mz
           .word mz
           .word phan
           .word b
           .word n
           .word b
           .word jmpw
           .word ra
           .word ma
           .word ma
           .word r
           .word riy
           .word hltn
           .word miy
           .word rzx
           .word rzx
           .word mzx
           .word mzx
           .word n
           .word ray
           .word n
           .word may
           .word rax
           .word rax
           .word max
           .word max
           .word rtsn
           .word rix
           .word hltn
           .word mix
           .word rz
           .word rz
           .word mz
           .word mz
           .word plan
           .word b
           .word n
           .word b
           .word jmpi
           .word ra
           .word ma
           .word ma
           .word r
           .word riy
           .word hltn
           .word miy
           .word rzx
           .word rzx
           .word mzx
           .word mzx
           .word sein
           .word ray
           .word n
           .word may
           .word rax
           .word rax
           .word max
           .word max
           .word b
           .word wix
           .word b
           .word rix
           .word wz
           .word wz
           .word wz
           .word rz
           .word n
           .word b
           .word n
           .word b
           .word wa
           .word wa
           .word wa
           .word ra
           .word r
           .word wiy
           .word hltn
           .word wiy
           .word wzx
           .word wzx
           .word wzy
           .word rzy
           .word n
           .word way
           .word n
           .word way
           .word wax
           .word wax
           .word way
           .word way
           .word b
           .word rix
           .word b
           .word rix
           .word rz
           .word rz
           .word rz
           .word rz
           .word n
           .word b
           .word n
           .word b
           .word ra
           .word ra
           .word ra
           .word ra
           .word r
           .word riy
           .word hltn
           .word riy
           .word rzx
           .word rzx
           .word rzy
           .word rzy
           .word n
           .word ray
           .word n
           .word ray
           .word rax
           .word rax
           .word ray
           .word ray
           .word b
           .word rix
           .word b
           .word mix
           .word rz
           .word rz
           .word mz
           .word mz
           .word n
           .word b
           .word n
           .word b
           .word ra
           .word ra
           .word ma
           .word ma
           .word r
           .word riy
           .word hltn
           .word miy
           .word rzx
           .word rzx
           .word mzx
           .word mzx
           .word n
           .word ray
           .word n
           .word may
           .word rax
           .word rax
           .word max
           .word max
           .word b
           .word rix
           .word b
           .word mix
           .word rz
           .word rz
           .word mz
           .word mz
           .word n
           .word b
           .word n
           .word b
           .word ra
           .word ra
           .word ma
           .word ma
           .word r
           .word riy
           .word hltn
           .word miy
           .word rzx
           .word rzx
           .word mzx
           .word mzx
           .word n
           .word ray
           .word n
           .word may
           .word rax
           .word rax
           .word max
           .word max

cmd        .byte 0

wrong
           jsr restoreint
           jsr print
           .byte 13
           .text "wrong $dd0d"
           .byte 0
           jmp waitk

main
           jsr print
           .byte 13
           .text "(up)nmi"
           .byte 0

           tsx
           stx error+1
           lda #0
           sta cmd

           lda #5
           jsr setint
           lda $dd0d
           cmp #$00
           bne wrong
           lda #4
           jsr setint
           lda $dd0d
           cmp #$01
           bne wrong
           lda #3
           jsr setint
           lda $dd0d
           cmp #$81
           bne wrong
           jsr restoreint

loop
           .block
           lda cmd
           cmp #$ff
           jmp ok
           lda #13
           jsr $ffd2
           lda cmd
           jsr printhb
waitkey
           jsr $ffe4
           beq waitkey
           cmp #3
           bne ok
           jmp $8000
ok
           .bend

           lda #<addressing
           ldx #>addressing
           clc
           adc cmd
           bcc noinc1
           inx
noinc1
           clc
           adc cmd
           bcc noinc2
           inx
noinc2
           sta 172
           stx 173
           ldy #0
           lda (172),y
           sta jump+1
           iny
           lda (172),y
           sta jump+2
jump
           jsr $1111
noerror
           inc cmd
           bne loop

           jmp ok

clock      .byte 0
rightlow   .byte 0
righthigh  .byte 0

compare
           stx addlow+1
           clc
addlow
           adc #$11
           sta rightlow
           bcc noiny
           iny
noiny
           sty righthigh
           cmp intlow
           bne error
           cpy inthigh
           bne error
           lda flagsbefore
           and #$14
           bne error
           lda flagsafter
           and #$14
           cmp #$14
           bne error
           rts

error
           ldx #$11
           txs
           lda #13
           jsr $ffd2
           lda cmd
           jsr printhb
           lda #"/"
           jsr $ffd2
           ldx clock
           lda #0
           jsr $bdcd
           jsr print
           .byte 13
           .text "stack  "
           .byte 0
           lda inthigh
           jsr printhb
           lda intlow
           jsr printhb
           lda #32
           jsr $ffd2
           lda flagsbefore
           and #$14
           jsr printhb
           lda #32
           jsr $ffd2
           lda flagsafter
           and #$14
           jsr printhb
           jsr print
           .byte 13
           .text "right  "
           .byte 0
           lda righthigh
           jsr printhb
           lda rightlow
           jsr printhb
           lda #32
           jsr $ffd2
           lda #$00
           jsr printhb
           lda #32
           jsr $ffd2
           lda #$14
           jsr printhb
waitk
           jsr $ffe4
           beq waitk
           cmp #3
           beq stop
           jmp noerror
stop
           lda turboass
           beq basic
           jmp $8000
basic
           jmp $a474


ok
           jsr print
           .text " - ok"
           .byte 13,0
           lda turboass
           beq load
waitkey    jsr $ffe4
           beq waitkey
           jmp $8000

load
           lda #47
           sta 0
           jsr print
name       .text "cia1tb123"
namelen    = *-name
           .byte 0
           lda #0
           sta $0a
           sta $b9
           lda #namelen
           sta $b7
           lda #<name
           sta $bb
           lda #>name
           sta $bc
           pla
           pla
           jmp $e16f


n
           .block
           jsr savestack
           lda cmd
           sta command
           lda #1
           sta clock
loop
           lda clock
           jsr setint
command
           nop
           cli
           cld
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           jsr restorestack
           rts
right      .byte 1,2
           .bend


sein
           .block
           jsr savestack
           lda cmd
           sta command
           lda #1
           sta clock
loop
           lda clock
           jsr setint
command
           nop
           cli
           cld
           jsr restoreint
           ldx clock
           bne not0
           lda flagsbefore
           eor #$04
           sta flagsbefore
not0
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           jsr restorestack
           rts
right      .byte 1,2
           .bend


b
           .block
           lda cmd
           sta command
           lda #1
           sta clock
loop
           lda clock
           jsr setint
command
           lda #0
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           rts
right      .byte 2,3
           .bend


rz
wz
           .block
           lda cmd
           sta command
           lda #2
           sta clock
loop
           lda clock
           jsr setint
command
           lda 2
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           rts
right      .byte 2,2,3
           .bend


mz
           .block
           lda cmd
           sta command
           lda #4
           sta clock
loop
           lda clock
           jsr setint
command
           lda 2
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           rts
right      .byte 2,2,2,2,3
           .bend


rzx
rzy
wzx
wzy
           .block
           lda cmd
           sta command
           lda #3
           sta clock
loop
           ldx #0
           ldy #0
           lda clock
           jsr setint
command
           lda 2,x
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           rts
right      .byte 2,2,2,3
           .bend



mzx
mzy
           .block
           lda cmd
           sta command
           lda #5
           sta clock
loop
           ldx #0
           ldy #0
           lda clock
           jsr setint
command
           lda 2,x
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           rts
right      .byte 2,2,2,2,2,3
           .bend


ra
wa
           .block
           lda cmd
           sta command
           lda #3
           sta clock
loop
           lda clock
           jsr setint
command
           lda $fff0
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           rts
right      .byte 3,3,3,4
           .bend


ma
           .block
           lda cmd
           sta command
           lda #5
           sta clock
loop
           lda clock
           jsr setint
command
           lda $fff0
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           rts
right      .byte 3,3,3,3,3,4
           .bend


rax
ray
           .block
           jsr savestack
           lda cmd
           sta command
           lda #3
           sta clock
loop
           ldx #0
           ldy #0
           lda clock
           jsr setint
command
           lda $fff0,x
           nop
           jsr restoreint
           ldx clock
           lda right1,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           lda cmd
           sta command2
           lda #4
           sta clock
loop2
           ldx #$12
           ldy #$12
           lda clock
           jsr setint
command2
           lda $fff0,x
           nop
           jsr restoreint
           ldx clock
           lda right2,x
           ldx #<command2
           ldy #>command2
           jsr compare
           dec clock
           bpl loop2
           jsr restorestack
           rts
right1     .byte 3,3,3,4
right2     .byte 3,3,3,3,4
           .bend


wax
way
           .block
           jsr savestack
           lda cmd
           sta command
           lda #4
           sta clock
loop
           ldx #0
           ldy #0
           lda clock
           jsr setint
command
           sta $fff0,x
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           jsr restorestack
           rts
right      .byte 3,3,3,3,4
           .bend


max
may
           .block
           jsr savestack
           lda cmd
           sta command
           lda #6
           sta clock
loop
           ldx #0
           ldy #0
           lda clock
           jsr setint
command
           sta $fff0,x
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           jsr restorestack
           rts
right      .byte 3,3,3,3,3,3,4
           .bend


rix
wix
           .block
           jsr savestack
           lda cmd
           sta command
           lda #$02
           sta 172
           lda #$00
           sta 173
           lda #5
           sta clock
loop
           ldx #0
           lda clock
           jsr setint
command
           lda (172,x)
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           jsr restorestack
           rts
right      .byte 2,2,2,2,2,3
           .bend


mix
           .block
           jsr savestack
           lda cmd
           sta command
           lda #$02
           sta 172
           lda #$00
           sta 173
           lda #7
           sta clock
loop
           ldx #0
           lda clock
           jsr setint
command
           lda (172,x)
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           jsr restorestack
           rts
right      .byte 2,2,2,2,2,2,2,3
           .bend


riy
           .block
           jsr savestack
           lda cmd
           sta command
           sta command1
           lda #$f0
           sta 172
           lda #$ff
           sta 173
           lda #4
           sta clock
loop
           ldy #$00
           lda clock
           jsr setint
command
           lda (172),y
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           lda #5
           sta clock
loop1
           ldy #$12
           lda clock
           jsr setint
command1
           lda (172),y
           nop
           jsr restoreint
           ldx clock
           lda right1,x
           ldx #<command1
           ldy #>command1
           jsr compare
           dec clock
           bpl loop1
           jsr restorestack
           rts
right      .byte 2,2,2,2,3
right1     .byte 2,2,2,2,2,3
           .bend


wiy
           .block
           jsr savestack
           lda cmd
           sta command
           lda #$f0
           sta 172
           lda #$ff
           sta 173
           lda #5
           sta clock
loop
           ldy #$00
           lda clock
           jsr setint
command
           lda (172),y
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           jsr restorestack
           rts
right      .byte 2,2,2,2,2,3
           .bend


miy
           .block
           jsr savestack
           lda cmd
           sta command
           lda #$f0
           sta 172
           lda #$ff
           sta 173
           lda #7
           sta clock
loop
           ldy #$00
           lda clock
           jsr setint
command
           lda (172),y
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           jsr restorestack
           rts
right      .byte 2,2,2,2,2,2,2,3
           .bend


r
           .block
from       = $2000
to         = $2000
           lda cmd
           sta from-2
           ldx #$f3
           and #$20
           beq clear
           ldx #$30
clear
           txa
           pha
           lda #to-from&$ff
           sta from-1
           lda #$ea
           sta to
           lda #$60
           sta to+1
           lda #0
           sta clock
           lda #10
           jsr setint
           plp
           jsr from-2
           jsr restoreint
           ldx #<to
           ldy #>to
           lda #0
           jsr compare
           .bend

           .block
from       = $2000
to         = $2000
           lda cmd
           sta from-2
           ldx #$f3
           and #$20
           beq clear
           ldx #$30
clear
           txa
           pha
           lda #to-from&$ff
           sta from-1
           lda #$ea
           sta to
           lda #$60
           sta to+1
           lda #1
           sta clock
           lda #11
           jsr setint
           plp
           jsr from-2
           jsr restoreint
           ldx #<to
           ldy #>to
           lda #1
           jsr compare
           .bend

           .block
from       = $2000
to         = $2010
           lda cmd
           sta from-2
           ldx #$f3
           and #$20
           bne clear
           ldx #$30
clear
           txa
           pha
           lda #to-from&$ff
           sta from-1
           lda #$ea
           sta to
           lda #$60
           sta to+1
           lda #0
           sta clock
           lda #10
           jsr setint
           plp
           jsr from-2
           jsr restoreint
           ldx #<to
           ldy #>to
           lda #0
           jsr compare
           .bend

           .block
from       = $2000
to         = $2010
           lda cmd
           sta from-2
           ldx #$f3
           and #$20
           bne clear
           ldx #$30
clear
           txa
           pha
           lda #to-from&$ff
           sta from-1
           lda #$ea
           sta to
           lda #$60
           sta to+1
           lda #1
           sta clock
           lda #11
           jsr setint
           plp
           jsr from-2
           jsr restoreint
           ldx #<to
           ldy #>to
           lda #1
           jsr compare
           .bend

           .block
from       = $2000
to         = $2010
           lda cmd
           sta from-2
           ldx #$f3
           and #$20
           bne clear
           ldx #$30
clear
           txa
           pha
           lda #to-from&$ff
           sta from-1
           lda #$ea
           sta to
           lda #$60
           sta to+1
           lda #2
           sta clock
           lda #12
           jsr setint
           plp
           jsr from-2
           jsr restoreint
           ldx #<to
           ldy #>to
           lda #1
           jsr compare
           .bend

           .block
from       = $1ff0
to         = $2010
           lda cmd
           sta from-2
           ldx #$f3
           and #$20
           bne clear
           ldx #$30
clear
           txa
           pha
           lda #to-from&$ff
           sta from-1
           lda #$ea
           sta to
           lda #$60
           sta to+1
           lda #1
           sta clock
           lda #11
           jsr setint
           plp
           jsr from-2
           jsr restoreint
           ldx #<to
           ldy #>to
           lda #0
           jsr compare
           .bend

           .block
from       = $1ff0
to         = $2010
           lda cmd
           sta from-2
           ldx #$f3
           and #$20
           bne clear
           ldx #$30
clear
           txa
           pha
           lda #to-from&$ff
           sta from-1
           lda #$ea
           sta to
           lda #$60
           sta to+1
           lda #2
           sta clock
           lda #12
           jsr setint
           plp
           jsr from-2
           jsr restoreint
           ldx #<to
           ldy #>to
           lda #0
           jsr compare
           .bend

           .block
from       = $1ff0
to         = $2010
           lda cmd
           sta from-2
           ldx #$f3
           and #$20
           bne clear
           ldx #$30
clear
           txa
           pha
           lda #to-from&$ff
           sta from-1
           lda #$ea
           sta to
           lda #$60
           sta to+1
           lda #3
           sta clock
           lda #13
           jsr setint
           plp
           jsr from-2
           jsr restoreint
           ldx #<to
           ldy #>to
           lda #1
           jsr compare
           .bend

           rts


brkn
           .block
           lda #<irq
           sta $fffe
           lda #>irq
           sta $ffff
           lda #9
           sta clock
loop
           lda clock
           jsr setint
command
           brk
           nop
           jsr restoreint
           ldx clock
           lda flagsbefore
           eor flags,x
           sta flagsbefore
           lda table,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           rts
irq
           nop
           rti
table      .byte 2,2,2,2,34
           .byte 34,34,34,2,2
flags      .byte $10,$10,$10,$10,$04
           .byte $04,$04,$04,$00,$00
           .bend


phan
phpn
           .block
           lda cmd
           sta command
           lda #2
           sta clock
loop
           lda clock
           jsr setint
command
           nop
           pla
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           rts
right      .byte 1,1,2
           .bend


plan
plpn
           .block
           lda cmd
           sta command
           lda #3
           sta clock
loop
           lda #0
           pha
           lda clock
           jsr setint
command
           nop
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<command
           ldy #>command
           jsr compare
           dec clock
           bpl loop
           rts
right      .byte 1,1,1,2
           .bend


jmpw
           .block
           lda #2
           sta clock
loop
           lda clock
           jsr setint
           jmp target
           brk
target
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<target
           ldy #>target
           jsr compare
           dec clock
           bpl loop
           rts
right      .byte 0,0,1
           .bend


jmpi
           .block
           lda #<target
           sta $2000
           lda #>target
           sta $2001
           lda #4
           sta clock
loop
           lda clock
           jsr setint
           jmp ($2000)
           brk
target
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<target
           ldy #>target
           jsr compare
           dec clock
           bpl loop
           rts
right      .byte 0,0,0,0,1
           .bend


jsrw
           .block
           lda #5
           sta clock
loop
           lda clock
           jsr setint
           jsr target
           brk
target
           pla
           pla
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<target
           ldy #>target
           jsr compare
           dec clock
           bpl loop
           rts
right      .byte 0,0,0,0,0,1
           .bend


rtsn
           .block
           lda #5
           sta clock
loop
           lda #>continue-1
           pha
           lda #<continue-1
           pha
           lda clock
           jsr setint
           rts
continue
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<continue
           ldy #>continue
           jsr compare
           dec clock
           bpl loop
           rts
right      .byte 0,0,0,0,0,1
           .bend


rtin
           .block
           lda #5
           sta clock
loop
           lda #>continue
           pha
           lda #<continue
           pha
           php
           lda clock
           jsr setint
           rti
continue
           nop
           jsr restoreint
           ldx clock
           lda right,x
           ldx #<continue
           ldy #>continue
           jsr compare
           dec clock
           bpl loop
           rts
right      .byte 0,0,0,0,0,1
           .bend


hltn
           rts


