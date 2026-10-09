# ares Profile Manager

An unofficial modification of [ares](https://github.com/ares-emulator/ares)
that adds a built-in settings profile manager and timestamps to save-state
slot labels.

This project is not an official ares release and is not endorsed by the ares
team.

## Features

### Settings profiles

The `Settings -> Settings File` page can discover and activate multiple
`*.bml` settings files.

- Select a profile from the `Settings Profiles (*.bml)` list.
- Click `Activate` to make it the current settings file.
- Use `Folder...` to choose a different profile directory.
- Use `Refresh` to scan the directory again.
- The active profile is marked with `(current)`.
- Subsequent settings changes are saved to the active profile.

By default, the manager scans the directory containing the current settings
file, so files such as `settings.bml` and `ares-settings-*.bml` stored there
appear automatically.

### BML comment preservation

Single-line comments beginning with `//` are preserved when ares rewrites the
settings file:

```bml
// Profile for a TCL C835 65-inch TV
// HDMI, 3840x2160, TV Game mode

Video
  Driver: OpenGL 4.6
```

Preserved comments are collected at the beginning of the file. The
`Settings File` page displays the original contents of the active file,
including comments.

### Timestamped save-state slots

Occupied entries under `Tools -> Save State` and `Tools -> Load State` display
the time of the most recent save:

```text
Slot 1 — 2026-09-21 18:42:07
Slot 2 — Empty
```

The timestamp is read from the save-state file's modification time, so labels
also work for states created before this feature was added. Labels refresh
immediately after saving or undoing a save. Empty slots are disabled in the
Load State menu.

ares v148 provides nine save-state slots, numbered 1 through 9. Their existing
`.bs1` through `.bs9` filenames and file format are unchanged, preserving
compatibility with standard ares builds.

## Windows installation

The modified executable can be installed alongside the original application:

```text
<ares-directory>\ares.exe                  Original executable
<ares-directory>\ares-profile-manager.exe Modified executable
```

Keep the executable in the ares installation directory so it can use the same
`Database`, `Shaders`, `Systems`, and `Games` directories. The original
`ares.exe` does not need to be replaced.

## Building from source

This repository uses the standard ares build system. Refer to the upstream
[ares build documentation](https://github.com/ares-emulator/ares) for current
platform requirements and build instructions.

On Windows with a configured Visual Studio and CMake environment, the desktop
frontend target can be built with:

```powershell
cmake --build build_msvc --config RelWithDebInfo --target desktop-ui --parallel 6
```

The resulting executable is normally located at:

```text
build_msvc\desktop-ui\rundir\ares.exe
```

## Compatibility and data safety

- Settings profiles use the existing ares BML format.
- Save-state filenames and serialized data are not changed.
- The modified and standard executables can use the same game and resource
  directories.
- Back up important settings and save states before testing development
  builds.

## License

This modification is distributed under the licenses included with ares.
Retain the repository's `LICENSE` file and all applicable copyright notices
when redistributing source code or binaries.

ares is copyright its respective contributors. The additional modifications
in this fork do not imply endorsement by the upstream project.
