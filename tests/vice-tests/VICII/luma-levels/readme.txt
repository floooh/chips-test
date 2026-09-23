
Luma test

This test is for measuring the luminance levels on real hardware. This is
achieved by displaying vertical colour bars of decreasing brightness. This
gives a nice step-like graph on an oscilloscope where the last step is the
sync level.

While absolute readings are interesting it's more important how each
level relates to others.

The luminance signal may contain traces of chrominance and AEC which
makes it ~0.1V wide. Readings should aim somewhere in the middle.

8565r2
------

Voltages of LUMA signal:
~0.2V  - sync
~0.9V  - black
~1.3V  - blue/brown
~1.4V  - red/dark gray
~1.55V - purple/orange
~1.75V - light blue/middle gray
~1.85V - green/light red
~2.1V  - cyan/light gray
~2.5V  - yellow/light green
~3.3V  - white

Pixel counting on the scaled measurement picture after scaling to
0-255 gives roughly:

0 42 55 73 93 101 130 173 255

