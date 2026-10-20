# Native Electron canvas spike

This frontend runs ares as native C++ in a separate process and displays its raw
video output in a WebGL2 canvas. It does not compile emulation code to WebAssembly.

Start with [EXECUTION-ROADMAP.md](EXECUTION-ROADMAP.md) for the implementation
sequence, bounded tasks and completion gates across the plans below.

The proposed next steps for memory access, agent-authored plugins, hot reload,
game matching, UI customization, and bundled settings are in [PLUGIN-PLAN.md](PLUGIN-PLAN.md).
The ares-64 debugging audit and preservation plan for the existing MCP tools are
in [MCP-PORT-PLAN.md](MCP-PORT-PLAN.md).

The native host **uses the existing `desktop-ui/emulator` registry and loaders**.
Every system included in `ARES_CORES` is available, including each core's frontend
variants (for example, Mega Drive/32X/CD and PC Engine/SuperGrafx/CD). There is no
second per-system loading implementation in the Electron code. Firmware and valid
media are still required for systems that require them in desktop ares.

## Build and run (verified on macOS arm64)

From the repository root:

```sh
cmake -S . -B build-electron-make -G 'Unix Makefiles' \
  -UARES_CORES -DARES_BUILD_ELECTRON_SPIKE=ON \
  -DARES_ENABLE_LIBRASHADER=OFF -DENABLE_CCACHE=OFF -DENABLE_IPO=OFF
cmake --build build-electron-make --target ares-electron-host ares-electron-cpu-fixture -j 8
npm ci --prefix tools/electron-spike
npm start --prefix tools/electron-spike
```

`-UARES_CORES` restores the normal full core list if this build directory previously
contained the initial Game Boy-only spike. Optional cores also appear automatically
if added to `ARES_CORES`. Other generators can be used in a different directory;
set `ARES_HOST` to the resulting native executable when running Electron.

`-DENABLE_IPO=OFF` disables LTO for local iteration and avoids optimizing every
core again at each native link. Switching an existing build requires one full
recompile. Release builds can enable IPO independently.

The app starts with a synthetic video test. Use **Open ROM** to detect the system
automatically. The system selector lists the native registry and can override
detection for ambiguous formats, alternate hardware, arcade sets, or disc images.
Multiple matches produce a system-choice dialog. For companion-media setups,
select the primary ROM first and additional media in the same file picker; their
paths are passed to the existing loader queue.

The host reads the existing ares `settings.bml` when found, including firmware
locations, core options, and peripheral configuration. **Use ares settings…** can
select another settings file. The selected settings apply to the next ROM and are
read only: the spike does not rewrite the desktop settings. Firmware failures are
reported in the status bar. No firmware is downloaded by this frontend.

```sh
ARES_ROM='/absolute/path/to/game.z64' npm start --prefix tools/electron-spike
ARES_SETTINGS='/absolute/path/to/settings.bml' npm start --prefix tools/electron-spike
ARES_SYSTEM='PlayStation' ARES_ROM='/absolute/path/to/disc.cue' npm start --prefix tools/electron-spike
```

Saves from Electron go into `tools/electron-spike/artifacts/saves`, using MIA's
system-specific save layout. They do not replace saves alongside the original
ROM. The generated `artifacts` directory and `node_modules` are ignored by Git.
Quit normally to let the original system adapter flush its saves.

## Controls and customization

Click the canvas to give it keyboard focus. Standard browser gamepads feed ares's
virtual controller; the existing per-system adapter determines the actual buttons.
Keyboard controls use arrows for D-pad, Z/X/C/V for face buttons, Q/E for shoulders,
Shift/Enter for Select/Start, Space for a trigger, WASD for the left analog stick,
and IJKL for the right analog stick. Number keys feed keypad inputs.

For N64, Z is B, X is A, IJKL are C-buttons, Space is Z, and WASD is the analog stick.
The screen shows the relevant control guide. Keyboard events and relative mouse
motion/buttons also reach the existing keyboard/mouse mappings. Losing focus
releases input. This spike currently exposes player one's virtual controller.

Themes, integer/best-fit scaling, scanlines, and panel placement are persisted in
local storage. The panel can be resized. Rendering is independent of controls and
UI layout. The UI-stall button deliberately blocks JavaScript for 250 ms to test
backpressure while native audio/emulation continue.

## Architecture and boundaries

- `services/` defines the shared TypeScript session contract, native client and
  Electron main router. [Protocol and module details](services/README.md).
- `native/session-service.hpp` queues versioned control requests and dispatches
  status/pause/resume/reset on the emulation thread, including while paused.
- `native/memory-service.hpp` provides bounded debugger-region access through the
  same dispatcher. The temporary memory inspector uses the shared browser/Node
  `MemoryClient`; offsets are region-relative and byte order is explicit in helpers.
- `host.cpp` supplies an `ares::Platform` adapter, native SDL audio, virtual HID
  input, and the emulator loop. System loading, firmware, peripherals, and saves
  are delegated to the existing `Emulator` implementations.
- The host currently links desktop support code and Hiro because the existing
  loader types reference menus/timers and desktop support classes. It **does not
  construct the desktop presentation/settings windows or run their UI loop**.
  Extracting a clean frontend-independent loader library is subsequent work.
- Video is copied from the core's callback before its pixel pointer expires.
  Source pitch is respected and ARGB words are explicitly converted to RGBA bytes.
  Aspect ratio is carried separately; rotation is already applied by ares.
- A dedicated native writer has one outstanding frame credit. The producer keeps
  one replaceable latest frame. Renderer acknowledgement after WebGL submission
  releases the next credit. Stalls discard obsolete presentation frames rather
  than growing an unbounded FIFO or stopping audio.
- The host reserves a duplicate of its original stdout for the binary frame
  stream, then redirects ordinary stdout to stderr. This also isolates core
  diagnostics such as GBA EEPROM detection printed during gameplay. Commands use
  stdin. Electron parses fragmented pipe data and forwards frames through a
  dedicated MessagePort. This is an intentionally **copied** baseline, not zero-copy.
- The renderer is sandboxed with context isolation and no Node integration.
  A narrow preload API exposes dialogs, controls, and the frame port.

The existing desktop adapter code is compiled into this target unchanged, except
for an opt-in external-input flag that lets virtual mouse mappings report capture
without a native viewport. Native desktop behavior retains its default path.

### AEF2 protocol

Each frame has a 48-byte little-endian header followed by tightly packed RGBA8:

| Offset | Type | Meaning |
| --- | --- | --- |
| 0 | 4 bytes | `AEF2` |
| 4 | uint32 | Payload byte length |
| 8, 12 | uint32 | Pixel width, height |
| 16 | uint32 | Produced frame sequence |
| 20 | uint32 | Last input sequence observed by a core input callback |
| 24 | float64 | Capture time in milliseconds since host start |
| 32 | float64 | Native capture-to-writer queue time in milliseconds |
| 40 | float64 | Intended display aspect ratio |

Input/presentation commands remain newline-delimited on stdin: `ack`,
`input sequence mask x y rx ry`, `key name 0|1`, `mouse dx dy mask`, `clear`, `quit`.
Status/pause/resume/reset use the versioned [session protocol](services/README.md),
with replies on a separate fd 3. EOF requests clean shutdown. Electron closes stdin
on shutdown and kills an unresponsive host after a timeout. Renderer reload keeps
the native host and session; frame-port acknowledgements from an old renderer are
ignored. `ARES_SAVE_DIR` can select an isolated save directory for automated runs.

## Validation

The full default macOS arm64 build exposes **40 system variants** through the
shared registry. Verified on 2026-10-17: generated GB and NES programs ran at
59.7 and 60.1 fps; synthetic 640×480 and 1080p transport ran at approximately
60 fps. Kirby 64 reached its file-selection screen with working keyboard input
and native SDL audio enabled. The 41-second N64 UI check averaged 57.1 native fps
including shader startup and a pause, with 8.73 ms bridge p95 and no WebGL errors.
Dev-browser checks passed for pause/resume, themes, scaling, scanlines, panel
placement, and an intentional UI stall. These checks validate the shared path;
they do not establish game compatibility for the other 37 system variants.

GBA follow-up: Minish Cap reached its title screen in Electron at approximately
60 fps with native audio enabled. Its optional 12-second native regression passed
at 59.7 fps, including the EEPROM diagnostic that previously interrupted playback.

```sh
npm test --prefix tools/electron-spike
npm run smoke --prefix tools/electron-spike
npm run test:electron --prefix tools/electron-spike
npm run test:mcp --prefix tools/electron-spike
```

Tasks 1–5 results and remaining checks are recorded in [EXECUTION-LOG.md](EXECUTION-LOG.md).
Memory transfers are limited to 1024 bytes. The synthetic source exposes
deterministic fixture memory for inspector experiments. Raw N64 `DCache` is unavailable
and its mapped `RDRAM` region is read-only. Explicit N64 VA/cache services are
separate; region access does not imply those semantics.

The [68-tool parity ledger](MCP-PARITY-LEDGER.md) records the audited reference
schemas/defaults and dependencies. 49 bounded MCP tools are exposed by
the opt-in stdio adapter; [setup and bundled runtime](mcp/README.md). `npm test` requires
the built native host and CPU fixture, and includes sequential isolated session checks.
`test:electron` uses a disposable profile and verifies stalled-renderer controls,
reload/session identity, frame credit, native pause-button behavior, and inspector
read/write/restore against the same paused bytes as a Node client. It writes an
ignored screenshot to `artifacts/memory-inspector.png`.

Parser tests cover fragmented/coalesced frames and malformed lengths/aspect ratios.
Native smoke tests cover synthetic 640×480 and 1080p frames (with padded source
rows), original generated Game Boy and NES programs, input delivery, pause/resume,
withheld acknowledgements, and clean process shutdown. No commercial ROMs are
included. Set `ARES_TEST_AUDIO=1` to enable native audio for the Game Boy fixture.
Set `ARES_GBA_ROM=/absolute/path/to/game.gba` to additionally run a 12-second GBA
regression past BIOS startup with your configured firmware. Minish Cap exercises
EEPROM diagnostics that previously corrupted the video stream.

Electron can also produce a renderer timing report and WebGL readback:

```sh
ARES_AUTOTEST=1 ARES_REPORT=/tmp/ares-renderer.json npm start --prefix tools/electron-spike
ARES_AUTOTEST=1 ARES_HD=1 ARES_REPORT=/tmp/ares-renderer-hd.json npm start --prefix tools/electron-spike
```

`ARES_TEST_SECONDS` changes the default 12-second duration; `ARES_SILENT=1`
disables native audio and uses the core's refresh-rate hint for pacing.

For interactive browser-driven checks:

```sh
npm start --prefix tools/electron-spike -- --remote-debugging-port=9237 --remote-debugging-address=127.0.0.1
dev-browser --connect http://127.0.0.1:9237 run tools/electron-spike/ui-smoke.js
```

`ui-smoke.js` exercises pause/resume, presentation options, input, and a renderer
stall; it saves a screenshot and report through dev-browser.

### What the measurements mean

“Bridge” is measured from Electron main receiving a complete frame to the renderer
submitting its WebGL draw. It excludes native-to-main pipe time, GPU completion,
compositor presentation, and physical display scanout. “Upload” measures CPU-side
texture upload/draw submission. Input echo measures a control update reaching a
core input callback and a subsequent submitted frame; it is **not** proof that the
game visually responded in that frame. Startup samples are excluded from input
echo statistics. Pauses affect the cumulative FPS average.

The spike establishes the transport and integration approach. It is not yet a
release package or a claim that every ROM/peripheral combination was tested.
Remaining evaluation includes a long soak, comparison against the desktop frontend
for end-to-end latency, audio underrun instrumentation, cross-platform packaging,
and unusual multi-screen/peripheral configurations. Runtime disc swapping and the
full desktop menu/debugger feature set are outside this spike. Original loader
support and missing-firmware diagnostics remain available for all compiled systems.

### Controlled VI execution

Task 4 adds `run_frames(n, max_seconds=60)`, `run_until_frame(k,
max_seconds=60)`, `step()`, `operation_status(operation_id)` and
`cancel_operation(operation_id=null)` to MCP. These support N64 and the explicitly
labelled synthetic fixture; other systems return `unavailable`. A VI coordinate
counts active scanout boundaries (interlaced fields count separately), not canvas
frames or inactive-display liveness yields. From a partial frame, one frame means
the next boundary. Zero frames completes immediately without advancing; absolute
targets in the past reject. Each new native process has a new session; reset
increments its epoch and resets the coordinate to zero.

The shared service methods are `execution.runFrames({count, timeoutMs})`,
`execution.runUntilFrame({frame, timeoutMs})`,
`execution.status({operationId})` and `execution.cancel({operationId})`.
Starts return a running operation or an immediate final result. Poll with the
original session/epoch. Completed snapshots are immutable and the latest 16 are
retained. Deadlines are 1–600000 ms; relative runs cap at 1000000 frames.
MCP polls internally while accepting concurrent status/cancel requests.

Each authenticated local connection owns its operations by connection generation.
A competing start, resume, write or cancel returns `busy`; reads and polling stay
available. Disconnect stops that connection's operation, and reconnect never
replays it. UI pause may interrupt any owner and UI reset revokes the old epoch.
Renderer reload leaves external ownership intact. Completion leaves ordinary play
paused. Controlled runs drain/discard audio and bypass normal frame pacing;
ordinary play keeps its configured executor and pacing. The native CPU loop has
a bounded iteration slice, not an instruction-stepping promise. GDB attachment
prevents starting controlled execution. Register snapshots, break/watch stops,
input leases/neutralization, captures and state-loading hooks remain later tasks;
requests for unimplemented MCP snapshot/capture options fail explicitly.


### TypeScript and module layout (task 4.5)

`npm run build` cleans and compiles only the Electron side. It does not invoke
CMake or relink the native emulator. `npm start`, the tests and packaging build
these outputs automatically; `build:services` remains an alias for compatibility.

| Source | Output / runtime |
| --- | --- |
| `main.ts`, `protocol.ts`, `services/`, `mcp/server.ts` | Native Node ESM in `dist/` |
| `renderer.ts`, `services/inspector.ts` | Browser ESM in `dist/browser/` |
| `preload.cts` | Single `dist/preload.cjs` for Electron’s sandboxed preload |
| `bridge.ts`, `renderer-globals.d.ts` | Shared IPC, frame data and window types |

The package declares `type: module` and starts `dist/main.js`. The main process
resolves assets relative to its compiled location in both checkout and copied
bundle. The preload’s emitted CJS is an Electron sandbox constraint; its source
is TypeScript. Test/packaging helper scripts retain explicit `.cjs` extensions;
Electron integration entry points are `.mjs`. Development tests use modern Node
with synchronous `require(ESM)` support; the distributed app supplies Node 24.

### N64 CPU debugging

The shared services expose N64 instruction steps, register inspection,
breakpoints, data watchpoints and immutable stop snapshots. MCP provides
`cpu_registers`, `step_instructions`, `run_until_hit`, `break_add/remove/list`,
`watch_add/remove/list` and `last_stop`. Branch delay slots count separately;
breakpoints stop before fetch and watchpoints after the accessing instruction.
Instrumentation temporarily selects the interpreter and preserves the configured
recompiler setting. UI state is observed by main even when no new video arrives.

`read_memory_va` and `write_memory_va` accept lossless 64-bit address strings and
strict physical/KSEG0/KSEG1 ranges within installed RDRAM. Transfers are limited
to 1024 bytes. CPU view uses resident D-cache, with no allocation on misses;
KSEG1 bypasses it. Writes report cache hazards, invalidate JIT and preserve resident
I-cache. They reject before RDRAM initialization completes. Raw debugger regions
remain a separate API. FPR inspection currently returns raw bits; stack unwinding
remains pending. Delayed write verification is implemented (see below).

Build both native targets from the recipe above before `npm test`. The
`ares-electron-cpu-fixture` target executes original hand-assembled MIPS in an
isolated core; it uses no game, user state or saves. `ARES_CPU_FIXTURE` can select
a fixture executable from another build directory.


### Task 6 input ownership (partial)

N64 controller tools provide four independent port owners and pending/latched
states. Holds take effect at the next VI or controlled-operation start. Optional
frame and wall-time leases release them; wall leases also expire while paused.
Disconnect revokes only that connection's holds. `resume(keep_overrides=true)`
preserves N64 holds; default external resume releases its own holds, while UI
resume releases all holds.

New keyboard presses, gamepad button presses/stick movement and mouse button
presses request human takeover on the native thread. Takeover finishes an active
operation with `host_input`, releases overrides and resumes ordinary play. Ports
without overrides receive neutral input during controlled operations by default.
The automated tests exercise real controller nodes and emulated keyboard events;
physical keyboard/gamepad acceptance remains pending. Controller taps and profiling are implemented; milestone 2 still awaits physical-device acceptance.


PC/call trace tools now run through the same owned coordinator. PC histograms are
sorted by frame and PC; linked calls capture the target and optional arguments
before execution. Filters and future VI intervals are supported. Captures report
truncation, overflow, observed/counted instructions and their actual storage cap
(262144 PC records or 65536 calls). Histogram buffers are released on completion.
`trace_page` reads the most recent capture by operation ID. MCP responses contain
at most 1024 entries, with an explicit next offset; optional atomic JSON file exports are implemented in the first task 7 slice.


`overlay_log` exposes a bounded PI cartridge-to-RDRAM transfer history (4096
records, up to 64 ranges each). Services-enabled apps record from boot; otherwise
the first query enables recording and marks missing history. Confirmed writes are
clipped to installed RAM; unknown chip mapping reports attempted ranges and null
bytes_written. The log reports overwrites and range truncation. Code candidates
are hints, not proof of overlays. Ordinary apps leave recording disabled unless
requested. MCP pages return at most 128 records with a continuation ID.

### Task 6 CPU cost profiling and controller taps

`profiler_control`, `profiler_clear` and `cpu_profiler_top` share emulation-thread
state. Profiling is off by default and retains JIT execution. Capture is owned by
the connection: disconnect disables it while retaining results; epoch reset
revokes it and clears data. Top results use the reference call/return and synthetic
exception accounting, with explicit master-clock units (187.5 MHz). Frame means
last observed VI-origin swap, not every VI tick. Open calls commit on return,
swap or disable. Function maps, call stacks and parked thread stacks are bounded;
continuous totals freeze at 1000 swaps, with explicit drop/freeze metadata.
Symbol loading, name-based wait classification and UI timeline/cache rings are
pending. Function names therefore use addresses (or synthetic exception labels).

`controller_tap` uses one owned native operation for 1..100000 VI boundaries,
with the same deadline/cancel/stop coordinator. It holds one button from operation
start; all finish paths release it. `keep_override` keeps a neutral port except
when ownership is revoked by disconnect, reset, shutdown or human takeover.
The tapped port cannot be overwritten/released during the operation. Its result
freezes `held_frames`, release status and resulting ownership.

### Task 7 artifact publishing (in progress)

Trace `path`/`overwrite` now export version-1 JSON with the complete retained
capture, summary, session/epoch, named call arguments and raw f12 data. Electron
main owns artifact I/O. The MCP adapter requests it through the authenticated
shared service and never opens destination files. Files publish from exclusive
private temporary siblings: no-overwrite uses an atomic hard link, explicit
overwrite uses rename. Failed or cancelled work preserves existing output.

Exports stream four bounded native pages per batch, cap output at 64 MiB and
25 seconds, and report byte count/SHA-256. Two jobs may run globally, one per
connection; disconnect, host replacement and stale epochs abort work. Relative
paths resolve against the app process working directory. `file_error` reports
export failure after capture; completed emulation is retained. Native screenshots are implemented below; audio taps and recording remain task 7 work.
The MCP registry now exposes 49 tools.

### Native screenshots (task 7, partial)

`screenshot` supports N64 `source=framebuffer` (default), reading VI-selected
RDRAM as RGBA5551/RGBA8888 before VI filtering, and `source=presented`, using
native post-screen output across cores (post-VI on N64). Presented capture is
independent of canvas credits and renderer lifetime. A disabled-by-default frame
submission observer tags each queued refresh on the emulation thread. A delayed
pre-reset worker refresh retains its old epoch and is discarded. Reset clears
captured frames; a paused capture with no fresh frame returns no_frame. Services-
enabled Electron sessions record presented output from startup; raw hosts may
use --debug-capture or enable future capture with the first video.begin query.

Native immutable snapshots use bounded 16 KiB pages and connection-owned pins.
Main fetches four pages per batch; crop/nearest-neighbor scale and PNG encoding
run in a worker. Up to 4194304 source/output pixels, scale 0.125..8 and an 8 MiB
PNG are allowed. Two native pins and two inline PNGs have 30-second leases;
capture jobs have a 25-second deadline checked between work. Files use the
shared atomic publisher; inline requests return actual MCP image content plus
metadata. File and inline output can be requested together. With neither inline
nor path, a unique PNG is saved under the host temporary directory. Generic core
frame coordinates are explicitly unavailable; N64 and synthetic VI kinds are
reported separately. Bounded presented-frame AVI recording is implemented. Physical keyboard/gamepad acceptance is still not run.

Audio tools (`audio_capture`, `audio_status`, `audio_save`) use a connection-owned
48000 Hz stereo tap before host muting, bounded to four seconds. The capture
freezes at its cap; reset and disconnect revoke it. WAV exports run in main,
atomically replace existing destinations (matching the reference audio API), and
consume capture only after successful publication. Partial and empty exports
work; failed exports retain samples for retry. See [MCP audio details](mcp/README.md#audio-capture).

Task 8 now includes N64 `state_save` and offline `state_info`, `state_list` and
`state_peek`. Native state files and metadata use bounded atomic publishing;
offline inspection never loads/restores the active machine. Header support is
reported separately from configuration compatibility. See the
[MCP state contract](mcp/README.md#native-states-task-8). Safe loading now passes the default-configuration cross-binary gate; multiwrite/delayed verification are implemented; bounded batch experiments pass the disposable baseline-load/write/active-VI-run/crop sweep.

Task 8 `batch` sequences state_load/write/run/screenshot/read under a native
reservation that survives file-I/O gaps. It follows epochs only for its own
successful loads, excludes competing mutations and reports actual run stops,
attempted/successful steps and retained effects. UI/parent pause, active-child
cancellation, human takeover, deadline and disconnect interrupt subsequent work.
Bounds: 32 steps, 1M frames, 16 KiB memory, 8 file screenshots, 20 seconds and
48 KiB results. These are explicit reductions from reference limits; no rollback,
nesting or parallel steps. See [batch details](mcp/README.md#bounded-batch-experiments).
