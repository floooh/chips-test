LOAD"C64.DATA",8,1
LOAD"CMP-B-COUNTS-A",8
RUN

After some time the border turns green (success) or an error message is written
to top of screen and the program freezes into a border decrement loop.

Errors take two forms:

"<byte> CMP <pointer>"

Meaning that the value read from TB after triggering counting did not match
the expected. The byte is the index of the access which failed with respect to
the address, which is the source address of the failed value. The comparision
value is stored at address+$1000 bytes.

"<byte> NMI <address>"

The byte is the value read from TA with the NMI handler, and address is where
the correct value is stored.


CIA-B-COUNTS-A.PRG was used to create the test data.

Data files:
C64.DATA             Created with original C64 (6526)
C64NEWCIA.DATA       Created with C64C with modified CIA (6526A / 8521)
VICE122.DATA         Created with buggy VICE-1.22
CCS.DATA             Created with buggy CCS3.4
HOXS.DATA            Created with HOXS-1.0.5, equal to C64.DATA

dump-oldcia.bin      Created with original C64 (6526)
dump-newcia.bin      Created with C64C with modified CIA (6526A / 8521)
