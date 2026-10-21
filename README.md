<img src="https://github.com/ares-emulator/ares/blob/master/ares/ares/resource/logo@2x.png" width="350"/>

[![License: ISC](https://img.shields.io/badge/License-ISC-blue.svg)](https://github.com/higan-emu/ares/blob/master/LICENSE)

**ares** is a multi-system emulator that began development on October 14th, 2004.
It is a descendant of [higan](https://github.com/higan-emu/higan) and [bsnes](https://github.com/bsnes-emu/bsnes/), and focuses on accuracy and preservation.

It's worth noting that ares takes some uncommon design approaches that essentially trade speed for code clarity. We avoid state machines and bitmasks (when possible). Most cores end up being half the amount of code, but slower. The code is clearer and less spaghettified, especially for systems with lots of processors. C bitfields being non-portable incurs a speedhit. Windows also has a speedhit over Linux due to its ABI needing more instructions to switch contexts.

The Electron frontend lives in [electron-ui/](electron-ui/README.md).
For ongoing frontend work, start with [the handoff](electron-ui/RESUME.md).

Official Releases
-----------------

Official releases are available from
[the ares website](https://ares-emu.net).

Nightly Builds
--------------

Automated, untested builds of ares are available for Windows and macOS as a [pre-release](https://github.com/ares-emulator/ares/releases/tag/nightly). 
Only the latest nightly build is kept.

Building this fork
------------------

This fork uses [Ares Workbench's Electron frontend](electron-ui/README.md).
The native host is the default CMake frontend target; emulation remains native
C++. Build/run instructions, tested macOS arm64 prerequisites, isolated profiles
and development/package checks are in that README. For ongoing implementation,
start with [the handoff](electron-ui/RESUME.md).

The superseded desktop frontend has been retired after extracting its loaders,
firmware, save routing, input and configuration support. The
[retirement inventory](electron-ui/DESKTOP-RETIREMENT.md) records preserved
behavior and deferred desktop features. Windows/Linux frontend distribution and
public-release packaging remain separate projects. Release links above describe
upstream ares, not releases of this fork's Electron frontend.

High-level Components
---------------------

* __ares__:       emulator cores and component implementations
* __electron-ui__: Electron frontend, native host, tools and shared services
* __hiro__:       native GUI toolkit for optional standalone MIA/genius utilities
* __nall__:       Near's alternative to the C++ standard library
* __ruby__:       platform-specific APIs for emulator video, audio, and input
* __mia__:        internal ROM database and ROM/image loader
* __libco__:      cooperative multithreading library

Contributing
------------

Please join our discord to chat with other ares developers: https://discord.com/invite/gz2quhk2kv
