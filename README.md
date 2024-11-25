![ares logo](https://github.com/ares-emulator/ares/blob/master/ares/ares/resource/logo.png)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fares-emulator%2Fares.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Fares-emulator%2Fares?ref=badge_shield)

[![License: ISC](https://img.shields.io/badge/License-ISC-blue.svg)](https://github.com/higan-emu/ares/blob/master/LICENSE)

**ares** is a multi-system emulator that began development on October 14th, 2004.
It is a descendant of [higan](https://github.com/higan-emu/higan) and [bsnes](https://github.com/bsnes-emu/bsnes/), and focuses on accuracy and preservation.

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
g++ make pkg-config libgtk-3-dev libcanberra-gtk-module libgl-dev libasound2-dev
```  
###### Additional Audio Drivers
ares supports additional audio drivers besides the ALSA drivers included above. Installing these additional packages will allow them to be selected in Settings > Drivers:  
`libao-dev libopenal-dev`

###### GTK2 & GTK3
By default, GTK3 is used, but support for GTK2 is available. You will need to install the additional package `libgtk2.0-dev` as well as specifying the command line option `hiro=gtk2` at compile time.

###### SDL2 for input
If you would like to use SDL for input (e.g. for using a controller), you will need to install the `libsdl2-dev` and `libsdl2-2.0-0` packages and perform a clean build of ares.
You should then be able to select SDL for input in the Settings > Drivers menu.

##### Building with clang

clang++ is now the preferred compiler for ares. If clang is detected on Windows/macOS/BSD, it will be selected by default. On Linux and other platforms, g++ remains the default if present. To build with clang, it is necessary to install both the `clang` and `lld` packages. If you would like to manually specify a compiler, you can use the following option: `compiler=[g++|clang++]`  

##### Librashader Support
If you do not want to include librashader support, you can pass the following option to the `make` command to skip these requirements using: `librashader=false`

Librashader requires rust in order to build. You can install it with the following command:
``` 
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source ~/.cargo/env
rustup toolchain install nightly
```

In order to build librashader, change into the `ares/thirdparty/librashader` directory and run the script `build-librashader.sh`
Note that once the build completes, it will instruct you to run three copy commands to install the library on your system. These paths may be different depending on Linux distribution. 
  
--------------

### Windows building

To build on Windows, using MSYS2 is recommended which can be download [here](https://www.msys2.org/). Follow the instructions
on this page to install and setup an appropriate MINGW64 environment. Running the command:  
```
pacman -S --needed base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-SDL2
```  
from the MSYS2 MSYS terminal should setup everything you need to compile ares. Note that in order to compile, you will want to be in a MINGW64 terminal window after install and setup is complete. 

##### Building with clang

clang is available through Visual Studio (or Build Tools for Visual Studio) through its installer and can be used to build ares. You will still need to supply GNU make in this instance. MSYS2 also offers a clang environment. You will want to make sure you select the clangw64 option during installation of MSYS2 which should provide and additional CLANG64 pre-configured environment. Install the clang toolchain package from the MSYS2 terminal:  
```
pacman -S mingw-w64-clang-x86_64-toolchain mingw-w64-clang-x86_64-SDL2
```  
Once complete, open a CLANG64 terminal window and proceed with building ares. 

###### Debug Symbols
When building with clang, by default symbols will be generated for debug builds using an MSVC compatible format (CodeView) for use with Windows debugging tools. In order to generate GDB compatible symbols, specify the following option: `symformat=gdb`  

###### Librashader Support
ares has introduced support for the [librashader](https://github.com/SnowflakePowered/librashader) library, which is a preprocessor, compiler, and runtime for RetroArch 'slang' shaders. If you are not interested in working with librashaders, you can pass the following option to the `make` command to skip these requirements using: `librashader=false`

Building the librashader library on Windows requires Rust. Follow these steps to prepare your system and build the librashader library:

1. Download and execute the rustup-init.exe installer from: https://rustup.rs/
2. Open a new Windows terminal and run the following commands:
    ```
    rustup toolchain install nightly
    rustup default nightly
    ```
3. With Rust successfully installed, you will need to update your PATH environment variable within your MSYS2 environment. In an MSYS2 shell, open $HOME/.bash_profile and append something like the following (note you will need to replace both instances of %USER_NAME% with the real user name you used to install Rust):
    ```
    if [ -d "/C/Users/%USER_NAME%/.cargo/bin" ] ; then
      PATH="/C/Users/%USER_NAME%/.cargo/bin:${PATH}"
    fi
    ```
4. Open a new MSYS2 based shell for the compiler you are using (MinGW64|CLANG64|UCRT64), execute `rustup` to validate the path is set properly within your MSYS2 environment.
5. In your MSYS2 compiler shell, `cd` into the `ares/thirdparty/librashader` directory
6. Execute the build script, build should complete successfully:
    ```
    ./build-librashader.sh
    ```

This only needs to be done once, or anytime the librashader library is updated. Once built, ares will build with librashader support by default requiring no additional flags.

Compilation
-----------

Check out the source code by running this command:

```
git clone https://github.com/ares-emulator/ares.git
```

From the root of the project directory run:

```
make -j4
```
 
`-j#` indicates number of parallel build processes, and shouldn't be set higher than N-1 cores on your processor.  Specifying this option can significantly decrease the time to build this project. There are multiple build types available and it defaults to an 'optimized' build. Build types can be specified using: `build=[debug|stable|release|minified|optimized]`  

Build options can be found in the following two make files: nall/GNUmakefile desktop-ui/GNUmakefile

To start compilation from the beginning, run the following prior to compiling:

```
make clean
```

#### Building specific cores  
If you would like to build a subset of cores, you can specify the `cores="core1 core2"` option. Currently available cores:  
```
a26 fc sfc n64 sg ms md ps1 pce ng msx cv myvision gb gba ws ngp spec
```  

Build Output
------------

There is a single binary produced at the end of compilation which can be found in `desktop-ui/out`. On OS's besides Linux, the `Database` & `Shader` directories are copied over here as well. On Linux, running `make install` after compilation will copy these directories and binary into suitable locations (see desktop-ui/GNUmakefile for details). Alternatively, these directories can be copied from `thirdparty/slang-shaders/*` into a `Shader` directory, and by copying `mia/Database/*`


Command-line options
--------------------

When started from the command-line, ares accepts a few options.

```
Usage: ./ares [options] game(s)

  --help                 Displays available options and exit
  --fullscreen           Start in full screen mode
  --system system        Specify the system name
  --shader shader        Specify GLSL shader name to load (requires OpenGL driver)
  --setting name=value   Specify a value for a setting
  --dump-all-settings    Show a list of all existing settings and exit
  --no-file-prompt       Do not prompt to load (optional) additional roms (eg: 64DD)
```

The --system option is useful when the system type cannot be auto-detected.
--fullscreen will only have an effect if a game is also passed in argument.

Example:
`ares --system MSX examples.rom --fullscreen`

Specifying multiple games allows for multi-cart support.  For example, to load
the Super GameBoy BIOS and a game in one command (to avoid a file prompt), you 
can do:

`ares "Super GameBoy.sfc" "Super Mario Land.gb"`

The --no-file-prompt option is useful if you wish to launch a game from CLI
without being prompted to load additional roms. For example, some Nintendo 64 
games optionally support 64DD expansion disks, so this option can be used to
suppress the "64DD Disk" file dialog, and assume any secondary content is 
disconnected.

High-level Components
---------------------

* __ares__:       emulator cores and component implementations
* __desktop-ui__: main GUI implementation for this project
* __hiro__:       cross-platform GUI toolkit that utilizes native APIs on supported platforms
* __nall__:       Near's alternative to the C++ standard library
* __ruby__:       interface between a hiro application and platform-specific APIs for emulator video, audio, and input
* __mia__:        internal ROM database and ROM/image loader
* __libco__:      cooperative multithreading library

Contributing
------------

Please join our discord to chat with other ares developers: https://discord.com/invite/gz2quhk2kv


## License
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fares-emulator%2Fares.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2Fares-emulator%2Fares?ref=badge_large)