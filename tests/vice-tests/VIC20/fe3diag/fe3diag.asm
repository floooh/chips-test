; VIC 20 Final Expansion Cartridge - Diagnose Programm  r002
; Thomas Winkler - 2009


; Testet die FE3/FE1 Hardware durch
; Testbedingung:
;  - Firmware aus (starten mit Commodore Taste)
;  - Firmware ab $6 geflashed
;  - Rest des Flashspeicher leer ($ff)


  processor 6502                         ;VIC20


FLGCOM  = $08


CHRPTR  = $7a                        ;Char Pointer
PT1     = $22                        ;Pointer
PT2     = $24                        ;Pointer
PT3     = $14                        ;Pointer

FAC     = $61

C_LINE  = $d1                           ;pointer current line char RAM
C_COLP  = $f3                           ;pointer current line color RAM

C_ROW   = $d6                           ;cursor row
C_COL   = $d3                           ;cursor column
C_CHR   = $d7                           ;cuurent char

KEYANZ  = $c6

CHRGET  = $0073                         ;GET NEXT CHAR
CHRGOT  = $0079                         ;GET LAST CHAR

BIP     = $0200                         ;BASIC Input Buffer 88 Bytes
BIP_LEN = 88

CAS_BUF = 828                           ;Kassetten Buffer

IO_FINAL = $9c02                        ;FINAL EXPANSION REGISTER 1 (39938,39939)


FEMOD_START = $00                       ;MODE START
FEMOD_ROM   = $40                       ;MODE EEPROM (READ EEPROM, WRITE RAM)
FEMOD_FLASH = $20                       ;MODE FLASH EEPROM (READ EEPROM, WRITE EEPROM)
FEMOD_RAM_1 = $80                       ;MODE RAM 1 (READ::BANK 1,WRITE::BANK 1/2)
FEMOD_RAM_2 = $C0                       ;MODE RAM 2 (WRITE::BANK 1,READ::BANK 1/2)
FEMOD_SRAM  = $A0                       ;MODE BIG SRAM (SRAM 512KB, BANK 0 TO 15)


LOADPTR = $c3
LOADEND = $ae


SOFT_RESET = 64802                      ;SOFT RESET
CURSOR_POS = $e50a

BSOUT      = $ffd2
GETIN      = $ffe4



PRNINT  = $ddcd     ; print integer in X/A



START_ADR  = $1200



	if	0
  org START_ADR -1                   ;

  byte <(START_ADR +1),>(START_ADR +1)
	else
	org	START_ADR+1
	endif
loader_start:
  byte $1b,$10,$d9,$07,$9e,$c2,"(44)",$ac,"256",$aa,$c2,"(43)",$aa,"26",0,0,0     ; 2009 SYSPEEK(44)*256+PEEK(43)+28



; ==============================================================
; START OF CODE
; ==============================================================

PRG_LEN    = END_ADR - START_ADR

START
  lda 44
  cmp #>(START_ADR)
  beq START_2

; MOVE CODE
  ldx 43
  dex
  clc
  adc #>(PRG_LEN)
  sta PT2 +1                            ; REAL END ADDRESS
  stx PT2

  ldx #<(START_ADR)
  lda #(>START_ADR + >PRG_LEN)
  stx PT1
  sta PT1 +1                            ; TARGET END ADDRESS

  ldx #>PRG_LEN +1
  ldy #0
START_0
  lda (PT2),y
  sta (PT1),y
  iny
  bne START_0
  dec PT1 +1
  dec PT2 +1
  dex
  bne START_0

  ldx #>(START_ADR)
  stx 44

START_2
  jmp TEST_PROGGI




; ==============================================================
; START OF TEST CODE
; ==============================================================

BANK    = FAC                           ; BANK  (8b)
BLOCK   = FAC +1                        ; BLOCK (16b)
PASS    = FAC +3                        ; PASS  (8b)
EC      = FAC +4                        ; ERROR CODE


FE1     = LOADPTR                       ; FE1 MODE



COUNT   = FLGCOM


TEST_PROGGI subroutine
  sei
  lda #0
  sta FE1
  jsr TEST_REGISTER                     ; TEST FE3 REGISTER
  bcs .E
  jsr TEST_ROM                          ; ROM READ MODE
  jsr TEST_RAM                          ; RAM MODE (FE3.1 RAM-1)
  bcs .E
  jsr TEST_PROT                         ; TEST BLOCK WRITE PROTZECT
  jsr TEST_DISABLE                      ; TEST BLOCK DISABLE
  jsr TEST_SRAM                         ; SUPER RAM MODE
  bcs .E
  jsr TEST_FE32
  bcs .31
  jsr TEST_RAM1                         ; RAM-1 MODE FE3.2
  bcs .E
.31
  jsr FE_OK
.E
  ldx #FEMOD_ROM                        ; ROM MODE
  stx IO_FINAL
  ldx #0
  stx IO_FINAL +1

;DEBUG
  ldx #FEMOD_SRAM +1                    ; SUPER RAM MODE
  stx IO_FINAL

  cli
  rts





; ==============================================================
; IO REGISTER TEST
; ==============================================================

TEST_REGISTER subroutine
  lda #1
  sta EC                                ;ERROR CODE

  lda #<MSG_REGISTER
  ldy #>MSG_REGISTER
  jsr STROUT

  sei
  ldx #128
  lda #0
  sta $a000                             ;UNLOCK IO
  sta IO_FINAL                          ;START MODE!
  sta IO_FINAL +1                       ;ENABLE ALL BLOCKS

  ldy $a000                             ;LOCK IO
  stx IO_FINAL
  cpx IO_FINAL
  bne TERE_1                            ;ok
  sta IO_FINAL
  cmp IO_FINAL
  bne TERE_1                            ;ok
TERE_ERR
;  cli
  jmp PRINT_ERR

TERE_1
  sta $a000                             ;UNLOCK IO

  inc EC                                ;EC#=2
; TEST REGISTER 1
  sta IO_FINAL
  cmp IO_FINAL
  bne TERE_ERR
  stx IO_FINAL
  cpx IO_FINAL
  bne TERE_ERR

  inc EC                                ;EC#=3
; TEST REGISTER 2
  dex                                   ;$7f
  stx IO_FINAL +1
  cpx IO_FINAL +1
  bne TERE_ERR
  sta IO_FINAL +1
  cmp IO_FINAL +1
  bne TERE_ERR

  inc EC                                ;EC#=5
; TEST CPLD A0
  inx                                   ;$80
  cpx IO_FINAL
  bne TERE_ERR
  cmp IO_FINAL +1
  bne TERE_ERR

  inc EC                                ;EC#=4
; TEST CPLD A1
  dex                                   ;$7f
  stx IO_FINAL -1
  cpx IO_FINAL -1
  bne TERE_2
  sta IO_FINAL -1
  cmp IO_FINAL -1
  beq TERE_ERR2
TERE_2
;  cli
  jmp PRINT_OK


TERE_ERR2
  jmp NO_RAM2



; ==============================================================
; ROM TEST
; ==============================================================

TEST_ROM
  lda #0
  sta EC                                ;ERROR CODE

  lda #FEMOD_ROM                        ;ROM READ MODE
  sta IO_FINAL

  lda #<MSG_ROMTEST
  ldy #>MSG_ROMTEST
  jsr STROUT

  ; COPY PART OF ROM AS SIG
  ldx #BIP_LEN -1
TERO_2
  lda $A000,x
  sta BIP,x
  dex
  bpl TERO_2

  lda #4
TERO_4
  sta BLOCK
  lda #$00                              ;A13 / A14
  jsr CHECK_ROM
  bcs TERO_ERR
  lda #$20                              ;!A13 / A14
  jsr CHECK_ROM
  bcs TERO_ERR
  lda #$40                              ;A13 / !A14
  jsr CHECK_ROM
  bcs TERO_ERR
  lda #$60                              ;!A13 / !A14
  jsr CHECK_ROM
  bcs TERO_ERR
  lda #0
  sta IO_FINAL +1

  lda #16
  ldx BLOCK
  beq TERO_6                            ;PASS1? -->
  jmp PRINT_OK

TERO_ERR
  jmp PRINT_ERR

TERO_6
  ldx #FEMOD_ROM +1                     ;ROM READ MODE, BANK 1
  stx IO_FINAL
  bne TERO_4




  ;CHECK BLK 1,2,3,5
CHECK_ROM
  sta IO_FINAL +1

  ldx #4
  stx PASS
CHRO_2
  inc EC                                ;EC#=1,5,9,13 :: 17,21,25,29
  cpx BLOCK
  php
  jsr SET_BLK_X
  jsr CHECK_SIG
  beq CHRO_4

  ;NO ROM!
  plp
  bne CHRO_6                            ;ok

CHRO_ERR
  sec
  rts

  ;IS ROM!
CHRO_4
  plp
  bne CHRO_ERR                          ;ok

CHRO_6
  dec PASS
  ldx PASS
  bne CHRO_2
  dec BLOCK
  clc
  rts


  ; CHECK ROM SIG AT (PT1)  ZF=1:OK
CHECK_SIG
  ldy #BIP_LEN -1
CHSI_2
  lda BIP,y
  cmp (PT1),y
  bne CHSI_E
  dey
  bpl CHSI_2
  iny
CHSI_E
  rts



; ==============================================================
; RAM 2 TEST (FE3.2 MODE)
; ==============================================================

TEST_FE32 subroutine
  lda #0
  sta EC                                ;ERROR CODE

  lda #FEMOD_RAM_2                      ;READ BANK 1
  ldx #1
  jsr .do                               ;VERIFY BANK 1
  bcs .rts

  lda #FEMOD_RAM_2 + $1e                ;READ BANK 2 FROM BLK 1,2,3,5
  ldx #2
  jsr .do                               ;VERIFY BANK 2
  bcs .rts

  lda #<MSG_RAMTEST2
  ldy #>MSG_RAMTEST2
  jsr STROUT
  jsr PRINT_OK
  clc
.rts
  rts


; ==============================================================
; RAM 2 TEST (FE3.2 MODE)
; ==============================================================

TEST_RAM1
  lda #0
  sta EC                                ;ERROR CODE

  lda #<MSG_FE32TEST
  ldy #>MSG_FE32TEST
  jsr STROUT

  lda #FEMOD_RAM_1 + $1e                ;READ BANK 1, WRITE BANK 2
  ldx #1
  jsr .do                               ;VERIFY BANK 1
  bcs .err

  lda #$55
  sta $2000
  ldx #FEMOD_RAM_2 + $1e                ;READ BANK 2, WRITE BANK 1
  stx IO_FINAL
  sta $2001
  cmp $2000
  bne .err
  ldx #FEMOD_RAM_1 + $1e                ;READ BANK 1, WRITE BANK 2
  stx IO_FINAL
  cmp $2001
  bne .err

  jmp PRINT_OK


.err
  jmp PRINT_ERR



.do
  sta IO_FINAL
  lda #1                                ;ONLY SECOND PASS (TEST)
  sta PASS
  jsr SET_BANK_BLOCK
  jmp TEST_BLOCKS_2



  ;XR=BANK
SET_BANK_BLOCK subroutine
  stx BANK
  lda #0
  tay
.1
  dex
  bmi .e
  clc
  adc #$80
  bcc .1
  iny
  bne .1
.e
  sta BLOCK
  sty BLOCK +1
  rts



; ==============================================================
; RAM TEST (SUPERRAM)
; ==============================================================

TEST_SRAM
  lda #0
  sta EC                                ;ERROR CODE

  lda #<MSG_SRAMTEST
  ldy #>MSG_SRAMTEST
  jsr STROUT

  lda #2
  sta PASS

TERA_20
  lda #0
  sta BANK
  sta BLOCK
  sta BLOCK +1

TERA_21
  lda BANK
  ora #FEMOD_SRAM                       ;SUPER RAM MODE
  sta IO_FINAL

  jsr TEST_BLOCKS_1
  bcc TERA_25

  lda EC
  cmp #33
  bne TERA_ERR
  jmp NO_RAM2

TERA_25
  inc BANK
  lda BANK
  cmp #16
  bcc TERA_21

  dec PASS
  bne TERA_20

  beq TERA_OK


; ==============================================================
; RAM TEST (FE3.1 RAM-1)
; ==============================================================

TEST_RAM subroutine
  lda #0
  sta EC                                ;ERROR CODE

  lda #FEMOD_RAM_1                      ;NORMAL RAM MODE
  sta IO_FINAL

  lda #<MSG_RAMTEST
  ldy #>MSG_RAMTEST
  jsr STROUT

  lda #2
  sta PASS
.0
  ldx #0
  jsr SET_BANK_BLOCK

.1
  jsr TEST_BLOCKS_0
  bcs TERA_ERR

  lda #FEMOD_RAM_1 + $1f                ;RAM MODE, PROTECT ALL BLOCKS
  sta IO_FINAL

  dec PASS
  bne .0

TERA_OK
  jmp PRINT_OK

TERA_ERR
  jmp PRINT_ERR




  ; PRINT BANK & TEST SRAM BLOCKS (Blk 1-3,5)
TEST_BLOCKS_1 subroutine
  lda BANK
  clc
  adc #65
  jsr BSOUT
  lda #157
  jsr BSOUT
  jmp TEST_BLOCKS_2


  ; TEST SRAM BLOCKS (Blk 0-3,5)
TEST_BLOCKS_0
  inc EC                                ;EC#=1,4
  jsr SET_LORAM
  jsr TEST_BLOCK
  bcs TEBLS_ERR

  ; TEST SRAM BLOCKS (Blk 1-3,5)
TEST_BLOCKS_2
  inc EC                                ;EC#=2,5
  jsr SET_BLK_2
  jsr TEST_BLOCK
  bcs TEBLS_ERR

  inc EC                                ;EC#=3,6
  jsr SET_BLK_A
  jsr TEST_BLOCK
;  bcs TEBLS_ERR
TEBLS_ERR
  rts


  ; TEST SRAM BLOCKS
TEST_BLOCK
  lda PASS
  cmp #2
  bne TEBL_5                            ;PASS CNT (2=first, 1=second)

  ldx BLOCK
  jsr TEST_SECTOR                       ;TEST 256 BYTE BLOCK
  bcs TEBL_ERR

  ; WRITE BLOCK ID
  lda BLOCK
  sta (PT1),y
  lda BLOCK +1
  iny
  sta (PT1),y
  lda BANK
  iny
  sta (PT1),y

  jsr INC_BLOCK
  inc PT1 +1

  dec COUNT
  bne TEST_BLOCK
  clc
  rts

TEBL_ERR
  sec
  rts

  ;TEST ONLY (PASS 2)
TEBL_5
  ;lda #"."
  ;jsr BSOUT
  ldy #0
  lda BLOCK
  cmp (PT1),y
  bne TEBL_ERR
  lda BLOCK +1
  iny
  cmp (PT1),y
  bne TEBL_ERR
  lda BANK
  iny
  cmp (PT1),y
  bne TEBL_ERR

  jsr INC_BLOCK
  inc PT1 +1

  dec COUNT
  bne TEST_BLOCK
  clc
  rts


  ;TEST MEMORY SECTOR (256 BYTES)
  ;TEST FOR RAM, CHECK EACH BIT
TEST_SECTOR
  ldy #0
TESE_1
  inx
  txa
  sta (PT1),y
  cmp (PT1),y
  bne TEBL_ERR
  eor #$ff
  sta (PT1),y
  cmp (PT1),y
  bne TEBL_ERR
  iny
  bne TESE_1
  clc
  rts


  ;TEST MEMORY SECTOR (256 BYTES)
TEST_SECTOR_RD
  ldy #0
TESE_RD_1
  inx
  txa
  eor #$ff
  cmp (PT1),y
  bne TEBL_ERR
  iny
  bne TESE_RD_1
  clc
  rts



; ==============================================================
; TEST PROTECTED RAM
; ==============================================================

TEST_PROT
  lda #0
  sta EC                                ;ERROR CODE

;  lda #FEMOD_RAM                        ;RAM READ MODE
;  sta IO_FINAL

  lda #<MSG_PROT
  ldy #>MSG_PROT
  jsr STROUT

;PROT_4
  lda #$11                              ;BLK0, BLK5
  jsr CHECK_PROT
  bcs PROT_ERR
  lda #$0e                              ;BLK1, BLK2, BLK3
  jsr CHECK_PROT
  bcs PROT_ERR
  lda #$05                              ;BLK1, BLK3
  jsr CHECK_PROT
  bcs PROT_ERR
  lda #$1a                              ;BLK0, BLK2, BLK5
  jsr CHECK_PROT
  bcs PROT_ERR
  jmp PRINT_OK

PROT_ERR
  jmp PRINT_ERR




  ;CHECK BLK 0,1,2,3,5
CHECK_PROT
  ora #FEMOD_RAM_1                      ;NORMAL RAM MODE
  sta IO_FINAL
  sta PASS

  ldx #0
  stx BLOCK
CHPR_2
  inc EC                                ;ERROR CODE
  lsr PASS
  php
  jsr SET_BLK_X
  jsr TEST_SECTOR
  bcs CHPR_4
  jsr TEST_SECTOR_RD
  bcs CHPR_4

  ;WRITE OK
  plp
  bcc CHPR_6                            ;not protected? --> ok

CHPR_ERR
  sec
  rts

  ;WRITE ERROR
CHPR_4
  plp
  bcc CHPR_ERR                          ;not protected? -->

CHPR_6
  inc BLOCK
  ldx BLOCK
  cpx #5
  bcc CHPR_2
  clc
  rts



; ==============================================================
; TEST BLOCK DISABLE
; ==============================================================

TEST_DISABLE
  lda #0
  sta EC                                ;ERROR CODE

  lda #FEMOD_RAM_1                      ;RAM READ MODE
  sta IO_FINAL

  lda #<MSG_DISABLE
  ldy #>MSG_DISABLE
  jsr STROUT

  lda #$11                              ;BLK0, BLK5
  jsr CHECK_DISA
  bcs DISA_ERR
  lda #$0e                              ;BLK1, BLK2, BLK3
  jsr CHECK_DISA
  bcs DISA_ERR
  lda #$05                              ;BLK1, BLK3
  jsr CHECK_DISA
  bcs DISA_ERR
  lda #$1a                              ;BLK0, BLK2, BLK5
  jsr CHECK_DISA
  bcs DISA_ERR
  jmp PRINT_OK

DISA_ERR
  jmp PRINT_ERR




  ;CHECK BLK 0,1,2,3,5
CHECK_DISA
  sta PASS
  sta PT3

  ldx #0
  stx BLOCK
CHDI_2
  inc EC                                ;ERROR CODE
  lsr PASS
  php
  jsr SET_BLK_X

  lda #0                                ;ENABLE ALL BLOCKS
  sta IO_FINAL +1
  ldx BLOCK
  jsr TEST_SECTOR
  bcs CHDI_ERR

  inc EC                                ;ERROR CODE
  lda PT3
  sta IO_FINAL +1
  ldx BLOCK
  jsr TEST_SECTOR_RD
  bcs CHPR_4

  ;READ OK
  plp
  bcc CHDI_6                            ;not disabled? --> ok

CHDI_ERR
  sec
  rts

  ;READ ERROR
CHDI_4
  plp
  bcc CHDI_ERR                          ;not disabled? --> ERROR

CHDI_6
  inc BLOCK
  ldx BLOCK
  cpx #5
  bcc CHDI_2
  clc
  rts





; ==============================================================
; SUB PROCEDURES
; ==============================================================

INC_BLOCK
  inc BLOCK
  bne INBL_2
  inc BLOCK +1
INBL_2
  rts
  

  ; SET BLK IN XR
SET_BLK_X
  dex
  bmi SET_LORAM
  cpx #3
  bcs SET_BLK_A
;  ldx #3
;SERX_1
  lda SERX_TAB,x
  ldy #0
  ldx #32
  bne SET_PTR


SERX_TAB
  .byte $20,$40,$60


  ; SET BLK 1,2,3
SET_BLK_2
  lda #>$2000
  ldy #<$2000
  ldx #96
  bne SET_PTR

  ; SET BLK 5
SET_BLK_A
  lda #>$A000
  ldy #<$A000
  ldx #32
  bne SET_PTR

  ; SET BLK 0
SET_LORAM
  lda #>$0400
  ldy #<$0400
  ldx #12
SET_PTR
  sta PT1 +1
  sty PT1
  stx COUNT
  rts



; ==============================================================
; MESSAGES
; ==============================================================

NO_RAM2
  lda #<MSG_NO_RAM2
  ldy #>MSG_NO_RAM2
  jsr STROUT
  inc FE1
  clc
  rts


PRINT_ERR
  lda #0                                ;ENABLE ALL BLOCKS
  sta IO_FINAL +1

  lda PT1 +1
  pha
  lda PT1
  pha

  lda #<MSG_ERROR
  ldy #>MSG_ERROR
  jsr STROUT
  ldx EC
  lda #0
  jsr PRNINT
  lda #<MSG_ERROR2
  ldy #>MSG_ERROR2
  jsr STROUT
  pla
  tax
  pla
  jsr PRNINT
  lda #13
  jsr BSOUT
  sec
  rts


PRINT_OK  subroutine
  lda #0                                ;ENABLE ALL BLOCKS
  sta IO_FINAL +1

  lda #<MSG_OK
  ldy #>MSG_OK
  jsr STROUT
  clc
  rts


FE_OK subroutine
  lda #<MSG_FE32OK
  ldy #>MSG_FE32OK
  bcc .ok
.fe3
  lda #<MSG_FE3OK
  ldy #>MSG_FE3OK
  ldx FE1
  beq .ok
.fe1
  lda #<MSG_FE1OK
  ldy #>MSG_FE1OK
.ok
  jmp STROUT



MSG_REGISTER
  dc.b BLUE,13,"FE3 DIAGNOSTICS",13,13,"REGISTER.....",0
MSG_ROMTEST
  dc.b "ROM/XMODE....",0
MSG_RAMTEST
  dc.b "RAM-MODE1....",0
MSG_RAMTEST2
  dc.b "RAM-MODE2....",0
MSG_SRAMTEST
  dc.b "SUPER-RAM....",0
MSG_PROT
  dc.b "BLK-PROTECT..",0
MSG_DISABLE
  dc.b "BLK-DISABLE..",0
MSG_FE32TEST
  dc.b "SPEC.MODES...",0

MSG_ERROR
  dc.b RED,"ERROR#",0
MSG_ERROR2
  dc.b BLUE,13,"HP=",0
MSG_NO_RAM2
  dc.b PURPLE,"FE1??",BLUE,13,0

MSG_HP
  dc.b "HP=",0

MSG_OK
  dc.b GREEN,"OK.",BLUE,13,0

  ;       "----------------------"
MSG_FE32OK
  dc.b 13,"FE-3.2 512K DETECTED.",13,0
MSG_FE3OK
  dc.b 13,"FE-3.1 512K DETECTED.",13,0
MSG_FE1OK
  dc.b 13,"FE1-40KB DETECTED.",13,0


; ==============================================================
; DISPLAY STRING    in AC/YR
; ==============================================================

STROUT
  sta PT1
  sty PT1 +1
  ldy #0
  ;sty 658                         ;Scroll Flag
  ;dey
STOU_1
  lda (PT1),y
  beq STOU_E
_relo0140 = . +1
  jsr BSOUT
  iny
  bne STOU_1
STOU_E
  rts




END_ADR


; ==============================================================
; Define some common PETSCII codes
; http://sta.c64.org/cbm64petkey.html
; ==============================================================

CLRHOME = $93
HOME    = $13
RVSON   = $12
RVSOFF  = $92
CR      = $0D
BLACK   = $90
WHITE   = $05
RED     = $1C
BLUE    = $1F
GREEN   = $1E
PURPLE  = $9C
YELLOW  = $9E
FONT1   = 142               ; BIG LETTERS & GRAFIC
FONT2   = 14                ; BIG AND SMALL LETTERS
AT      = $40
