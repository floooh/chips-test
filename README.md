# chips-test

[![Build Status](https://github.com/floooh/chips-test/workflows/build_and_test/badge.svg)](https://github.com/floooh/chips-test/actions)

Tests and sample emulators for https://github.com/floooh/chips

Live demos of the example emulators: https://floooh.github.io/tiny8bit

To build and run on Windows, OSX or Linux (exact versions of tools shouldn't matter):

Prerequisites:

- deno (https://docs.deno.com/runtime/getting_started/installation/)
- cmake (at least version 3.21)
- your system's C/C++ toolchain (Clang, GCC, MSVC)

NOTE: on Linux, additional dev packages need to be present for X11, GL, ALSA and ncurses development.

Clone, build and run the CPC emulator:

```bash
git clone https://github.com/floooh/chips-test
cd chips-test
./fibs build
./fibs run cpc
```

...if you're on Linux or macOS and have Ninja installed, you'd want to use that
instead:

```bash
# on Linux
./fibs config linux-ninja-release
./fibs build
# on macOS
./fibs config macos-ninja-release
./fibs build
```

To see all runnable targets:

```bash
./fibs list targets --exe
```

To open the project in an IDE:

```bash
# on macOS with Xcode
./fibs config macos-xcode-debug
./fibs open

# on macOS with VSCOde:
./fibs config macos-vscode-debug
./fibs open

# on Linux with VSCode:
./fibs config linux-vscode-debug
./fibs open

# on Windows with VStudio:
./fibs config win-vstudio-debug
./fibs open

# on Windows with VSCode:
./fibs config win-vscode-debug
./fibs open
```

To build the WebAssembly demos (Linux or OSX recommended here, Windows
might work too, but this is not well tested).

```bash
# check if required tools are installed (ninja and maybe http-server)
./fibs diag tools
# install the Emscripten SDK
./fibs emsdk install
# configure, build and run
./fibs config emsc-ninja-release
./fibs build
./fibs run cpc
# or to open the project in VSCode:
./fibs config emsc-vscode-debug
./fibs open
```

When the above emscripten build steps work, you can also build and test the
entire samples webpage like this:

```bash
./fibs webpage build
./fibs webpage serve
```

If fibs gets stuck in an error situation, try `./fibs reset` to start over from scratch:

```bash
./fibs reset
```

## Many Thanks To:

- utest.h: https://github.com/sheredom/utest.h
