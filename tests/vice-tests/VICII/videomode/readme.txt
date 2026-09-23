these tests show various mid-line splits between graphic modes. these splits
do not become effective at character boundaries (as one would expect) but they
are delayed by a varying amount of pixels - caused by propagation delays and
analog side effects. the amount of delay may depend on the type of VICII, and
the temperature of the chip.

see the screenshots directory for captures from real hardware, compared to the
reference pictures used for the test suite.

what kind of splits do actually vary between setups (and temperature) is still
yet to be determined, more captures from real hardware are needed.


The reference "screenshots" were crafted to match the supposed expected results.

CAUTION: since this is a tedious and error prone process, there might still be
errors left. Corrections welcome!


NOTE: it may be notable that, for 8565, the references kept here are all very
similar (identical expect for some pixels) to the output of hoxs64.


videomode1.prg.png (PAL 6569)
- matched against my breadbox(m1) (gpz,17/12/2025)
videomode1.prg-8565.png
- matched against my C64C(m7) (gpz,18/12/2025)

videomode2.prg.png (PAL 6569)
- matched against my breadbox(m1) (gpz,17/12/2025)
videomode2.prg-8565.png
- matched against my C64C(m7) (gpz,18/12/2025)

videomode-v.prg.png (PAL 6569)
- matched against my breadbox(m1) (gpz,17/12/2025)
videomode-v.prg-8565.png
- matched against my C64C(m7) (gpz,18/12/2025)

videomode-w.prg.png (PAL 6569)
- matched against my breadbox(m1) (gpz,17/12/2025)
videomode-w.prg-8565.png
- matched against my C64C(m7) (gpz,18/12/2025)

videomode-x.prg.png (PAL 6569)
- matched against my breadbox(m1) (gpz,17/12/2025)
videomode-x.prg-8565.png
- matched against my C64C(m7) (gpz,18/12/2025)

videomode-y.prg.png (PAL 6569)
- matched against my breadbox(m1) (gpz,17/12/2025)
videomode-y.prg-8565.png
- matched against my C64C(m7) (gpz,18/12/2025)

videomode-z.prg.png (PAL 6569)
- matched against my breadbox(m1) (gpz,17/12/2025)
- matches screenshot (zerox 6569r1,r5)
videomode-z.prg-8565.png
- matched against my C64C(m7) (gpz,18/12/2025)

--------------------------------------------------------------------------------

videomode-v:
- matches: unseen-nogreydot, unseen-greydot
- with zerox-8565r2 shows a one pixel difference in the bottom white dotted line

videomode-x:
- shows a one pixel difference in the transition of the bottom red line to the
  red/cyan pattern. hard to tell what is right
- zerox-6569r5_1886_s does not match the reference, it seems to match the 8565
  reference (?)

videomode-z:
- matches: zerox-8565r2
- shows a one pixel difference in the transition of the bottom black line to the
  red/cyan pattern. hard to tell what is right

videomode2:
- shows some one pixel differences. hard to tell what is right

TODO:

- we need more, and more detailed screenshots so the references can be fixed
  and verified
- additional reference (and testsuite support) for 6569r1 might be needed

-------------------------------------------------------------------------------

videomode1.prg

(Hires Text) (->Multicolor Text) (->illegal)
(->ECM Text) (->Hires Text)


videomode2.prg

(Hires Text) (->Multicolor Text) (->Multicolor Bitmap) (->illegal)
(->Multicolor Bitmap) (->Hires Bitmap) (->Hires Text)


videomode-v.prg

(Hires Text) (->Multicolor Text) (->illegal) (->Multicolor Bitmap)
(->Multicolor Bitmap) (->Multicolor Text) (->Hires Text)


videomode-w.prg

(Hires Text) (->Hires Bitmap) (->illegal) (->ECM Text)
(->Hires Text) (->Multicolor Text) (->Hires Text)


videomode-x.prg

(Hires Text) (->Multicolor Text) (->Multicolor Bitmap) (->illegal)
(->Multicolor Bitmap) (->Hires Bitmap) (->Hires Text)


videomode-y.prg

(Hires Text) (->Hires Bitmap) (->Multicolor Bitmap) (->Multicolor Text)
(->illegal) (->Multicolor Text) (->Hires Text)


videomode-z.prg

(Hires Text) (->ECM Text) (->illegal) (->ECM Text)
(->illegal) (->Hires Bitmap) (->illegal) (->Hires Bitmap) (->Hires Text)

