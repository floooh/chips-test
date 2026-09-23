
!cpu 6510
!ct pet

VSPSEQ=2
!if (DEBUG = 0) {
DEFXSTART=$28
DEFXOFFS=$17
}

!src "dmadelay.s"

screendata2:
             ;1234567890123456789012345678901234567890
        !scr "........................................"
        !scr ".............................dmadelay.2."
