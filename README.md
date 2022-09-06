![ares logo](https://github.com/ares-emulator/ares/blob/master/ares/ares/resource/logo.png)

[![License: ISC](https://img.shields.io/badge/License-ISC-blue.svg)](https://github.com/higan-emu/ares/blob/master/LICENSE)

**ares** is a multi-system emulator that began development on October 14th, 2004.
It is a descendent of [higan](https://github.com/higan-emu/higan) and [bsnes](https://github.com/bsnes-emu/bsnes/), and focuses on accuracy and preservation.

Official Releases
-----------------

Official releases are available from
[the ares website](https://ares-emu.net).

Nightly Builds
--------------

Automated, untested builds of ares are available for Windows and macOS as a [pre-release](https://github.com/higan-emu/ares/releases/tag/nightly). 
Only the latest nightly build is kept.

Prerequisites
-------------

#### *nix building

	g++ make pkg-config libgtk2.0-dev libcanberra-gtk-module libgl-dev libasound2-dev

By default, GTK2 is used, but support for GTK3 is available. You will need to install the additional package `libgtk-3-dev` as well
as specifying the command line option `hiro=gtk3` at compile time.

#### Windows building

To build on Windows, using MSYS2 is recommended which can be download [here](https://www.msys2.org/). Follow the instructions
on this page to install and setup an appropriate MINGW64 environment. Running the command  `pacman -S --needed base-devel mingw-w64-x86_64-toolchain` from the MSYS2 MSYS terminal should setup everything you need to compile Ares. Note that in order 
to compile, you will want to be in a MINGW64 terminal window after install and setup is complete. 

Compilation
-----------

Check out the source code by running this command:

	git clone https://github.com/ares-emulator/ares.git
	
From the root of the project directory run:

	make -j4 build=release
	
that builds with build type of type 'release'. 
`-j4` indicates number of parallel build processes, and shouldn't be set higher than N-1 cores on your processor. Specifying this option can significantly decrease the time to build this project. There are multiple build types available (debug, etc.). Most additional options can be 
found in nall's make file (nall/GNUmakefile).

To start compilation from the beginning, run the following prior to compiling:

	make clean

Build Output
------------

There is a single binary produced at the end of compilation which can be found in `desktop-ui/out`. On OS's besides Linux, the `Database` & `Shader` directories are copied over here as well. On Linux, running `make install` after compilation will copy these directories and binary into suitable locations (see desktop-ui/GNUmakefile for details). Alternatively, these directories can be copied from `ares/Shaders/*` and `mia/Database/*`.


Command-line options
--------------------

When started from the command-line, ares accepts a few options.

```
Usage: ./ares [options] game

  --help                 Displays available options and exit
  --fullscreen           Start in full screen mode
  --system system        Specify the system name
```

The --system option is useful when the system type cannot be auto-detected.
--fullscreen will only have an effect if a game is also passed in argument.

Example:
`ares --system MSX examples.rom --fullscreen`


High-level Components
---------------------

* __ares__:       emulator cores and component implementations
* __desktop-ui__: main GUI implementation for this project
* __hiro__:       cross-platform GUI toolkit that utilizes native APIs on supported platforms
* __nall__:       Near's alternative to the C++ standard library
* __ruby__:       interface between a hiro application and platform-specific APIs for emulator video, audio, and input
* __mia__:        internal ROM database and ROM/image loader
* __libco__:      cooperative multithreading library
