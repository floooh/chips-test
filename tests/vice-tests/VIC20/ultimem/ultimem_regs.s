;;; @file ultimem.s
;;; Vic UltiMem registers
;;; @author Marko Mäkelä (marko.makela@iki.fi)

;;; This file is in the public domain.

ultimem	= $9ff0			; UltiMem registers
ultimem_cfg = ultimem		; UltiMem configuration register
ultimem_ioram = ultimem + 1	; 00|I/O3|I/O2|RAM123 config
ultimem_blk = ultimem + 2	; BLK5|BLK3|BLK2|BLK1 config
ultimem_id = ultimem + 3	; ID register (read-only)
ultimem_ram = ultimem + 4	; RAM123 address register (lo/hi)
ultimem_io = ultimem + 6	; I/O2 and I/O3 address register (lo/hi)
ultimem_blk1 = ultimem + 8	; BLK1 address register (lo/hi)
ultimem_blk2 = ultimem + 10	; BLK2 address register (lo/hi)
ultimem_blk3 = ultimem + 12	; BLK3 address register (lo/hi)
ultimem_blk5 = ultimem + 14	; BLK5 address register (lo/hi)

;;; values for the ultimem_cfg register
ultimem_cfg_dis = $80		; disable the UltiMem registers
ultimem_cfg_reset = $40		; assert RESET for one clock cycle
ultimem_cfg_led = 1		; UltiMem LED
ultimem_cfg_noled = 0		; UltiMem LED off
;;; values for the ultimem_blk register
ultimem_blk_5_0k = $40		; BLK5 ROM, nothing at BLK1..BLK3
ultimem_blk_5_24k = $7f		; BLK5 ROM, BLK1..BLK3 RAM
ultimem_blk_24k = $3f		; nothing at BLK5; RAM at BLK1..BLK3
ultimem_blk_3_5_0k = $50	; BLK3 ROM, BLK5 ROM; nothing at BLK1, BLK2
;;; values for the read-only ultimem_id register
ultimem_id_8m = $11		; 8MiB flash + 1MiB RAM
ultimem_id_512k = $12		; 512KiB flash + 128KiB or 256KiB or 512KiB RAM
;;; values for the ultimem_ioram register
ultimem_ioram_0k = 0		; nothing at RAM123, I/O2, I/O3
ultimem_ioram_3k = 3		; 3k RAM, nothing at I/O2, I/O3
ultimem_ioram_0ki= $30		; RAM at I/O3
ultimem_ioram_3ki= $33		; RAM at I/O3 and RAM123
