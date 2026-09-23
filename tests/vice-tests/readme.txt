This is a subset of the VICE emulator tests collection.

The original tests can be found here:

https://sourceforge.net/p/vice-emu/code/HEAD/tree/testprogs/

The VICII/ subdirectory was mirrored from /testprogs/VICII at SVN revision
46247. Two kinds of file were left out to keep the repository small, neither
of them usable for automated comparison:

  - the screenshots/ directories, which hold photos of real hardware output
    (161 MB of JPG/PNG/GIF/APNG)
  - the remaining *.jpg files, also photos of real hardware (23 MB)

Everything else was kept: the .prg test programs, their sources, Makefiles,
readmes, .tas input scripts, and the references/ directories with the small
384x272 emulator screenshots that tests/c64-vicetest.c compares against.

To re-mirror (there is no svn client requirement, the SourceForge SVN HTTP
interface serves browsable directory listings):

  curl -sS https://svn.code.sf.net/p/vice-emu/code/testprogs/VICII/
