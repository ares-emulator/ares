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

### *nix building

###### Minimum required packages:
```
g++ make pkg-config libgtk2.0-dev libcanberra-gtk-module libgl-dev libasound2-dev libao-dev libopenal-dev
```

###### GTK2 & GTK3
By default, GTK2 is used, but support for GTK3 is available. You will need to install the additional package `libgtk-3-dev` as well as specifying the command line option `hiro=gtk3` at compile time.

###### SDL2 for input
If you would like to use SDL for input, you will need to install the following the `libsdl2-dev` and `libsdl2-0.0` packages and perform a clean build of ares. You should then be able to select SDL for input in the Settings > Drivers menu.   

##### Building with clang

clang++ is now the preferred compiler for ares as it is known to produce a higher performing executable. If it is detected, the build will default to building with clang. It is recommended to install the `clang` package. If you would like to manually specify a compiler, you can use the following option: `compiler=[g++|clang++]`

### Windows building

To build on Windows, using MSYS2 is recommended which can be download [here](https://www.msys2.org/). Follow the instructions
on this page to install and setup an appropriate MINGW64 environment. Running the command:  
```
pacman -S --needed base-devel mingw-w64-x86_64-toolchain
```  
from the MSYS2 MSYS terminal should setup everything you need to compile ares. Note that in order to compile, you will want to be in a MINGW64 terminal window after install and setup is complete. 

##### Building with clang

clang is available through Visual Studio (or Build Tools for Visual Studio) through its installer and can be used to build ares. You will still need to supply GNU make in this instance. MSYS2 also offers a clang environment. You will want to make sure you select the clangw64 option during installation of MSYS2 which should provide and additional CLANG64 pre-configured environment. Install the clang toolchain package from the MSYS2 terminal:  
```
pacman -S mingw-w64-clang-x86_64-toolchain
```  
Once complete, open a CLANG64 terminal window and proceed with building ares. 

###### Debug Symbols
When building with clang, by default symbols will be generated using an MSVC compatible format for use with Windows debugging tools. In order to generate GDB compatible symbols, specify the following option:  
`symformat=gdb`  

###### Console Output  
By default, the console is disabled on Windows builds. To enable it, specify the following option:  
`console=true`

Compilation
-----------

Check out the source code by running this command:

```
git clone https://github.com/ares-emulator/ares.git
```

From the root of the project directory run:

```
make -j$((`nproc`-1)) build=release
```

that builds with build type of type 'release'. 
The 'nproc-1' option will use N-1 total cores available on your system and indicates number of parallel build processes. Specifying this option can significantly decrease the time to build this project. There are multiple build types available (debug, etc.). Most additional options can be 
found in nall's make file (nall/GNUmakefile).

To start compilation from the beginning, run the following prior to compiling:

```
make clean
```

#### Building specific cores  
If you would like to build a subset of cores, you can specify the `cores="core1 core2"` option. Currently available cores:  
```
a26 fc sfc n64 sg ms md ps1 pce ng msx cv gb gba ws ngp spec
```  

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
  --shader shader        Specify GLSL shader name to load (requires OpenGL driver)
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
