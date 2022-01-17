# chips-test

[![Build Status](https://github.com/floooh/chips-test/workflows/build_and_test/badge.svg)](https://github.com/floooh/chips-test/actions)

Tests and sample emulators for https://github.com/floooh/chips

Live demos of the example emulators: https://floooh.github.io/tiny8bit

To build and run on Windows, OSX or Linux (exact versions of tools shouldn't matter):

Check prerequisites:

```bash
> python --version
Python 2.x or 3.x
> cmake --version
cmake version 3.x
```

NOTE: on Linux, additional dev packages need to be present for X11, GL and ALSA development.

Create a 'workspace' directory (which will be populated with additional dependencies),
clons and cd into ```chips-test```:

```bash
> mkdir workspace
> cd workspace
> git clone https://github.com/floooh/chips-test
> cd chips-test
```

Finally, build and run one of the emulators (for instance the Amstrad CPC):

> ./fips build
> ./fips list targets
> ./fips run cpc
```

To get optimized builds for performance testing:

```bash
# on OSX:
> ./fips set config osx-make-release
> ./fips build
> ./fips run [target]

# on Linux
> ./fips set config linux-make-release
> ./fips build
> ./fips run [target]

# on Windows
> fips set config win64-vstudio-release
> fips build
> fips run [target]
```

To open project in IDE:
```bash
# on OSX with Xcode:
> ./fips set config osx-xcode-debug
> ./fips gen
> ./fips open

# on Windows with Visual Studio:
> ./fips set config win64-vstudio-debug
> ./fips gen
> ./fips open

# experimental VSCode support on Win/OSX/Linux:
> ./fips set config [linux|osx|win64]-vscode-debug
> ./fips gen
> ./fips open
```

To build the WebAssembly demos (Linux or OSX recommended here, Windows
might work too, but this is not well tested).

```bash
# first get ninja (on Windows a ninja.exe comes with the fips build system)
> ninja --version
1.8.2
# now install the emscripten toolchain, this needs a lot of time and memory
> ./fips setup emscripten
...
# from here on as usual...
> ./fips set config wasm-ninja-release
> ./fips build
...
> ./fips list targets
...
> ./fips run c64
...
```

When the above emscripten build steps work, you can also build and test the
entire samples webpage like this:

```bash
> ./fips webpage build
...
> ./fips webpage serve
...
```

## Many Thanks To:

- utest.h: https://github.com/sheredom/utest.h
