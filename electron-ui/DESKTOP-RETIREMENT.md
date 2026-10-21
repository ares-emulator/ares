# Desktop frontend retirement inventory

Reviewed against `b9f42e052` (the last checkpoint before extraction), 2026-10-21.
The user approved replacement of the desktop frontend and retaining reference
material in Git. This inventory is a scope disposition, not a claim of feature
parity. Source can be recovered with `git show b9f42e052:desktop-ui/<path>`.

| Desktop implementation | Disposition in the Electron product |
| --- | --- |
| `emulator/` registry, all core-specific constructors/loaders, firmware, region selection, peripheral setup, media/save routing and keyboard mappings | Extracted into `native/runtime/emulator/`. All 40 compiled variants retain their registry entries. Core options and saved peripheral selection still import from explicit BML configuration. |
| `input/input.*` virtual ports, direct/inherited bindings, digital/analog/relative mapping, trigger ranges and rumble forwarding | Runtime mappings extracted into `native/runtime/input/`; input mutex and capture flags belong to `FrontendRuntime`. Existing Electron virtual-device defaults stay in the host until task 13. No new physical-device/multiplayer/rumble support is claimed. |
| `settings/settings.*` data model and BML binding process | Extracted without the windows, write-on-load or import/export UI. Profile settings and explicitly selected BML files are read only until task 13 provides owned settings. No migration or implicit import of the desktop user's configuration. |
| `program/utility.cpp` palette application | Extracted as `updatePalette`; screen luminance/saturation/gamma continue to apply after loading. |
| `program/platform.cpp`, drivers and emulator loop | Replaced by the existing host platform, SDL audio, copied video and single execution coordinator. Cores, native services and Ruby remain intact. Desktop event-loop/thread guards were inactive in the Electron host; emulation-thread ownership remains authoritative, with the cross-thread input mutex retained. |
| `emulator/` Hiro menu hooks and disc-insert/tray timers | Retired GUI callbacks. Startup primary/companion media loading remains. Runtime disc/tape changes, per-device menus, orientation/color/phosphor menus and interactive controller-pak switching are deferred UI features; they were not called by Electron. Core settings/device implementations are retained. |
| `presentation/` windows, menus, status, file/system dialogs, scaling and fullscreen | Existing Electron shell handles loading/system choice, status, controls, presentation and plugin docks. Native menus, recent library/start screen, fullscreen/hotkey parity and shader-browser integration are deferred. |
| `game-browser/` arcade metadata/search/list window | Window retired. Registry/category metadata and Arcade/Neo Geo loader/database support retained; a library browser is deferred. |
| `program/states.cpp`, rewind/run-ahead, auto-save timer and hotkeys | Desktop slots/undo UI, rewind, run-ahead and periodic-save policy are deferred. Existing native state services/MCP remain supported with their documented serializer/ownership limits. Shutdown save flushing is preserved; periodic saving is not newly promised. |
| `tools/` memory, trace, properties, graphics, audio streams, cheats, manifest and tape windows | Desktop windows retired. Existing generic memory/debugger plugins and 68 MCP services retain their supported capabilities. Generic graphics/properties/manifest/stream viewers, cheat-file UI and tape controls are deferred, not declared equivalent to N64 tools. |
| `settings/` firmware scan, paths, core/video/audio/input/hotkey editors, home and import/export pages | Task 13 owns real settings and configurable input. Firmware can already be supplied through an explicit settings file. No physical-device/audio-output gate is closed by removing these editors. |
| `resource/`, `desktop-ui.cpp`, desktop CMake/platform launch, icons, font, entitlements and JIT window fixes | Retired with the desktop executable; none is needed by the extracted runtime. Core/MIA embedded firmware resources and media databases remain. Development macOS packaging remains the existing Electron pipeline; public releases/notarization and other OS frontends remain deferred. |
| `.github` desktop build/archive/sign/release workflows, wrappers and `scripts/macos-make-universal.sh` | Retired because their desktop target/artifact paths no longer exist. The fixture-only macOS Electron acceptance lane now replaces the automated frontend checks. No replacement public-release workflow is promised. |
| `hiro/` toolkit | Retained only for optional independent MIA/genius utilities, not an Electron include/link/build dependency in the normal configuration. |

The extraction is licensed under the repository's ISC license. Loader, mapping
and BML behavior derive from the existing ares frontend at the checkpoint above;
this is relocation and removal of unused UI coupling, not replacement core code.

Resource ownership is explicit: Electron passes the selected native runtime's
resource root with `--resource-dir`; MIA uses a narrow optional resolver callback.
Without that callback, standalone MIA retains its existing resource lookup policy.
The Electron host no longer embeds source-tree paths or falls back to desktop
resources/configuration. Default profile settings are `<profile>/settings.bml`;
`ARES_SETTINGS` remains an explicit read-only override. Native standalone tests
resolve default resources alongside their built/copied runtime.
