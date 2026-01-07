<img src="https://github.com/ares-emulator/ares/blob/master/ares/ares/resource/logo@2x.png" width="350"/>

[![License: ISC](https://img.shields.io/badge/License-ISC-blue.svg)](https://github.com/higan-emu/ares/blob/master/LICENSE)

**ares** is a multi-system emulator that began development on October 14th, 2004.
It is a descendant of [higan](https://github.com/higan-emu/higan) and [bsnes](https://github.com/bsnes-emu/bsnes/), and focuses on accuracy and preservation.

It's worth noting that ares takes some uncommon design approaches that essentially trade speed for code clarity. We avoid state machines and bitmasks (when possible). Most cores end up being half the amount of code, but slower. The code is clearer and less spaghettified, especially for systems with lots of processors. C bitfields being non-portable incurs a speedhit. Windows also has a speedhit over Linux due to its ABI needing more instructions to switch contexts.

Official Releases
-----------------

Official releases are available from
[the ares website](https://ares-emu.net).

Nightly Builds
--------------

Automated, untested builds of ares are available for Windows and macOS as a [pre-release](https://github.com/ares-emulator/ares/releases/tag/nightly). 
Only the latest nightly build is kept.

Building ares
-------------

ares supports building on Windows, macOS, and various Linux/BSD distributions. See build instructions for:

* [Windows](https://github.com/ares-emulator/ares/wiki/Build-Instructions-For-Windows)
* [macOS](https://github.com/ares-emulator/ares/wiki/Build-Instructions-For-macOS)
* [Linux](https://github.com/ares-emulator/ares/wiki/Build-Instructions-For-Linux)
* [BSD](https://github.com/ares-emulator/ares/wiki/Build-Instructions-For-BSD)

Command-line options
--------------------

When started from the command-line, ares accepts a few options.

```
Usage: ./ares [options] game(s)

  --help                 Displays available options and exit
  --terminal             Create new terminal window (Windows only)
  --fullscreen           Start in full screen mode
  --system system        Specify the system name
  --shader shader        Specify a slang shader to load (requires OpenGL or Metal)
  --setting name=value   Specify a value for a setting
  --dump-all-settings    Show a list of all existing settings and exit
  --no-file-prompt       Do not prompt to load (optional) additional roms (eg: 64DD)
  --settings-file path   Specify a settings file override (settings.bml)
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
