; Select the video timing (processor clock cycles per raster line)
;CYCLES = 65     ; 6567R8 and above, NTSC-M
;CYCLES = 64    ; 6567R5 6A, NTSC-M
;CYCLES = 63    ; 6569 (all revisions), PAL-B

    !src "cmp-b-counts-a.asm"

    ; note: this file must have the load address removed
    *=$4000
!if CYCLES = 63 {
    !bin "dump-newcia.bin"
} else {
    !bin "dump-newcia-ntsc.bin"
}
