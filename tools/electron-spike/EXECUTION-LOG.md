# Execution log

## Task 1 — contracts and a minimal session round trip

Implemented 2026-10-17, following baseline commit `39565bbfe`. Next: **task 2,
native memory regions and bounded access**. The task 3 MCP/inspector milestone
has not been reached; no plugin loader, external endpoint, MCP adapter, CPU
instrumentation or fork native code was added.

Implemented:

- Shared TypeScript request/reply validation, typed session interfaces, a bounded
  native client and an Electron-main-owned session router in `services/`.
- Versioned native control protocol with request IDs, host session IDs and reset
  epochs. Status, pause/resume and the existing reset action run at native outer
  loop safe points, including while paused. Stale identities reject mutations.
- Independent bounded native control reply writer on fd 3, preserving AEF2 stdout
  video and stderr diagnostics. Neither dispatch nor replies depend on canvas
  acknowledgements. Timeout/disconnect never retries a mutation.
- Pause-button state from native results. Renderer reload retains the host,
  session, pause state and frame-credit ownership; stale frame ports are revoked.
- All 68 reference tool signatures/defaults/behavioral docstrings, dependencies,
  status and limits in `mcp/parity-ledger.json` and `MCP-PARITY-LEDGER.md`.
  Provenance is the audited `ares-64` commit
  `1c9b9637476a83f7b80d3db1115a0f71529820d9`; no native implementation was
  transplanted. The source checkout was only read. Reference test migration
  targets are annotated in the ledger.

Validation on the actual macOS arm64 host, with isolated sequential test instances
and generated GB/NES ROMs, using disposable profiles/separate test save paths:

| Check | Command / evidence | Result |
| --- | --- | --- |
| Native host build | `cmake --build build-electron-make --target ares-electron-host -j 8` | Passed |
| TypeScript and contract/transport/native tests | `npm test --prefix tools/electron-spike` | Passed, 8 tests |
| Native running/paused requests, no video credit, stale session/reset epoch, no retry after timeout/disconnect, fragmented replies, malformed/unknown requests | `tests/client.test.cjs`, `tests/session-live.test.cjs` | Passed |
| Actual Electron stalled-renderer control, paused/running reloads, same session, frame credit/WebGL, pause-button resume and stale identity after host replacement | `npm run test:electron --prefix tools/electron-spike` | Passed, `ELECTRON_SESSION_PASS`; disposable app closed |
| Existing load/video/input/pause/resume/shutdown and 40-system discovery | `ARES_TEST_AUDIO=1 npm run smoke --prefix tools/electron-spike` | Passed; all test hosts closed |
| Synthetic 640×480 / 1080p | Native smoke | 59.92 / 59.88 fps |
| Generated GB with native SDL audio output enabled | Native smoke | 56.49 fps; audio initialized, no output errors |
| Generated NES | Native smoke | 60.08 fps |
| Frozen inventory | Checked 68 unique tool entries against source AST | Passed |
| Diff whitespace and JS syntax | `git diff --check`, `node --check` | Passed |

The restricted sandbox could not launch Electron or open native audio. Those
checks passed after rerunning with local app/audio access. Audio initialization
and the output path were exercised; audible quality/underrun instrumentation was
**not run**. The existing N64/GBA game regressions and physical gamepad checks were
**not rerun** in task 1. Native queue saturation, a peer that stops reading control
replies, long soak, renderer process crash recovery, and Windows/Linux transport
fault injection remain **not run**. These are not passing results carried over
from the reference fork or the earlier spike.

Current limitations:

- Status is generic paused/system/capability state. Reference N64 register,
  effective-executor, operation ownership and exact VI/instruction semantics await
  their scheduled tasks. Safe-point pause here is the outer `root->run()` boundary.
- `emu_status`/`pause`/`resume` have only a native service foundation, not MCP schema
  or result parity. `resume(keep_overrides=...)` needs task 6 input ownership.
- The 68-tool snapshot stores source annotations/defaults; generated FastMCP JSON
  Schema and real stdio parity are **not run**, due in task 3 and later backend tasks.
- Timeout may leave a mutation's outcome unknown. No replay or rollback is claimed.
  App failure terminates the host rather than reconnecting/replaying mutations.
- Distribution/runtime packaging and explicit local app-instance selection remain
  task 3. The internal line protocol is not the fork's telemetry protocol.

Task 2 should enumerate debugger memory nodes with session-scoped opaque IDs,
validate writable metadata, add bounded emulation-thread reads/writes, explicit
endian JS helpers and a temporary inspector. Keep N64 VA/cache APIs deferred.

## Task 2 — native memory regions and bounded access

Implemented 2026-10-18, following task 1 commit `cc30a4429`. Next: **task 3,
TypeScript MCP adapter and first integration gate**. MCP is still unavailable;
no Python bridge, external listener, plugin system, or N64 VA/cache engine was added.

Implemented:

- Exposed debugger-node read/write callback availability with two const accessors
  in `ares/ares/node/debugger/memory.hpp`, preserving existing node callbacks.
- Enumerated opaque session-scoped region IDs with actual sizes and conservative
  access metadata. Memory reads/writes use the same native safe-point dispatcher
  as session requests, also while paused. Each request transfers 1–1024 bytes;
  offsets/ranges and complete write payloads are checked before any callback.
- Shared typed service results/parameters, strict runtime validation, a reusable
  `MemoryClient` with identity-bearing region handles, and explicit-endian unsigned
  8/16/32-bit helpers. Browser and Node builds use the same client and schemas.
- Temporary inspector with region selection, labeled region offsets, bounded
  reads and explicit hex writes. Read-only/unavailable nodes disable appropriate
  controls. Stale work is rejected instead of retried against a replacement game.
- Deterministic synthetic debugger nodes (RAM, read-only, unavailable) and an
  initial `--paused` host option for isolated core initialization checks.
- Main-router session-state notifications update the pause button for requests
  made outside the renderer too; reset epochs refresh inspector handles.

Audit findings and explicit limits:

- The existing N64 `RDRAM` callback follows hardware chip mapping. A fixture
  write/read assertion before initialization failed because writes can be ignored.
  The service now marks this debugger view read-only and rejects its writes.
  Reads retain the existing debugger view, not a promised linear physical-memory
  view. The roadmap and MCP port plan explicitly carry the write behavior forward
  to task 5; it was not removed from the parity target.
- `DCache` is entirely unavailable through raw regions: it exposes CPU-cache
  semantics and metadata independent of installed RAM. Task 5 owns that service.
- N64 RSP DMEM and GBA EWRAM use direct storage callbacks and provide deterministic
  write/read/restore evidence here. Callback invocation is not a CPU cache flush,
  verified instruction patch, rollback, exact-frame stop or owned experiment.

Validation on macOS arm64, using one isolated test instance at a time:

| Check | Command / evidence | Result |
| --- | --- | --- |
| Shared-header/core and native host rebuild | `cmake --build build-electron-make --target ares-electron-host -j 8` | Passed, full default core list retained |
| Node + browser TypeScript builds and focused tests | `npm test --prefix tools/electron-spike` | Passed, 12 tests |
| Actual N64/GBA region enumeration and inspection | `tests/memory-live.test.cjs` | Passed on paused cores via normal loaders |
| Max-size bulk write/read/restore; full prevalidation at boundaries; stale session, region and epoch rejection | `tests/memory-live.test.cjs` | Passed; N64 RSP DMEM, GBA EWRAM and synthetic RAM restored |
| Read-only/unavailable callback metadata and malformed native payload | Synthetic debugger-node fixture | Passed |
| Explicit endian and unaligned byte-slice handling | `tests/memory.test.cjs` | Passed |
| Real browser inspector writes/readback agree with Node client; original bytes restored | `npm run test:electron --prefix tools/electron-spike` | Passed, `ELECTRON_SESSION_PASS` |
| Stalled renderer, paused/running reload, one-frame queue, WebGL, native pause notifications, host replacement and stale requests | Same disposable Electron test | Passed; app and native hosts closed |
| Inspector layout | `artifacts/memory-inspector.png`, visually inspected | Passed; ignored generated artifact |
| Existing 40-system registry and GB/NES ROM loading, video, input, pause/resume/shutdown | `ARES_TEST_AUDIO=1 npm run smoke --prefix tools/electron-spike` | Passed; generated ROMs, isolated test save paths |
| Synthetic 640×480 / 1080p | Native smoke | 59.92 / 59.96 fps |
| Generated GB with native SDL audio enabled / NES | Native smoke | 56.49 / 60.08 fps |
| Diff and JS syntax | `git diff --check`, `node --check` | Passed |

The N64/GBA fixtures use original generated dummy cartridges and an original
16 KiB ARM-loop BIOS stand-in. They initialize real core debugger nodes through
the normal loaders, with execution paused from startup. They do **not** establish
BIOS boot, game compatibility, exact CPU/VI semantics or instruction-cache patch
behavior. No commercial ROM, user firmware or active emulator session was used;
fixtures and save directories were disposable and removed afterward.

Task 1's pending manual/fault-injection checks remain pending. Additional
**not run** checks include every other system's individual region behavior,
region-count/queue-saturation faults and metadata changes during peripheral
hotplug. The 68-tool inventory remains intact; memory tools have a native
foundation only. MCP schema/default/result parity, instance selection, reconnect
without mutation replay and distributed Node launch still require task 3 and its
integration gate. No prior fork test result is counted as new-host evidence.

## Task 3 — TypeScript MCP adapter and first integration gate

Implemented 2026-10-18, following task 2 commit `72bdc9c3e`. **Milestone 1 passed
on macOS arm64.** Next: **task 4, core execution coordinates and ownership**.
No advanced CPU/RSP debugging or plugin loader was added.

Implemented:

- Opt-in Electron-main-owned local service endpoint (`--services` or
  `ARES_ENABLE_SERVICES=1`) with explicit app UUID selection, private Unix socket,
  owned 0700 directory/0600 metadata, token authentication and per-connection
  UUID/request IDs. No TCP listener or arbitrary app selection.
- Shared `LocalServiceClient`/`MemoryClient` and typed contracts; bounded messages,
  clients, pending work and request histories. Stale connection/duplicate IDs
  fail before dispatch. Timeouts/disconnections reject pending requests; only a
  new call can reconnect and no request is replayed. Short accepted mutations
  have no rollback guarantee; owned-operation disconnect cleanup awaits task 4.
- Actual Node stdio MCP entry point using official SDK 2.3.1. Exactly six tools:
  `emu_status`, `pause`, `resume`, `memory_regions`, `read_memory`, `write_memory`.
  Names, required arguments and defaults are asserted through real `tools/list`;
  `read_memory` also exercises the omitted-length default of 64 at runtime.
  Adapter stdout is MCP-only; all diagnostics use stderr.
- Native/renderer session identity remains separate from app/connection identity.
  Explicit discovery refreshes handles; stale cached memory mutations do not
  rebind/retry against a reset/replacement. Renderer reload leaves MCP attached.
- A self-contained macOS development bundle and `Contents/Resources/ares-mcp`
  launcher using Electron's verified Node 24.22.0 runtime. Production dependency
  graph, native executable/databases, SDL3/MoltenVK and licenses are copied;
  developer rpaths are replaced and the local bundle is ad-hoc signed. The
  launcher uses relative bundle paths and needs no system Node or Python.
- Installed native-host lookup uses the installed resource layout even when the
  copied Electron development executable reports `isPackaged=false`.

Validation (sequential isolated instances; no active user game/save was a fixture):

| Check | Command / evidence | Result |
| --- | --- | --- |
| TypeScript builds and focused session/memory/local-transport suite | `npm test --prefix tools/electron-spike` | Passed, 15 tests |
| Token authentication, explicit UUID/permissions, fragmented hello, stale connection UUID, duplicate request, timeout/disconnect/reconnect without replay | `tests/local-endpoint.test.cjs` | Passed; timed-out mutation observed once |
| Actual MCP stdio initialize/list/call, source parameter names/required fields/defaults, six-tool inventory | `npm run test:mcp --prefix tools/electron-spike` | Passed |
| MCP writes and inspector reads the same paused **real GB CPU WRAM**; original bytes restored; native session retained across renderer reload | Same test with generated GB ROM and isolated profile/saves | Passed, `MCP_INTEGRATION_PASS` |
| Wrong app selection, stale reset epoch, stale session after host replacement, native-host disconnection, unsupported tools/override flag | Same real stdio test | Passed, explicit errors |
| Existing frame-credit/WebGL/inspector/reload/stalled-renderer behavior | `npm run test:electron --prefix tools/electron-spike` | Passed, `ELECTRON_SESSION_PASS` |
| Local distribution construction | `npm run package:mac --prefix tools/electron-spike` | Passed; native host and full production SDK graph included |
| Copied distribution outside repository; generated GB ROM; bundled stdio runtime; read/write/restore/resume, clean app/MCP exit and metadata cleanup | `npm run test:package --prefix tools/electron-spike` | Passed, `PACKAGE_MCP_PASS`, 455 rendered GB frames, no WebGL error |
| No checkout dependency for native loader and adapter | Same package test, denying checkout reads for native-loader preflight and MCP Node process | Passed; GUI retains its normal Chromium sandbox |
| Runtime supplied by distribution | `artifacts/package-mcp.json` and bundled `runtime.json` | Electron 44.9.0 / Node 24.22.0, macOS arm64 |
| Syntax, inventory and whitespace | `node --check`, 68 ledger rows, `git diff --check` | Passed |

The restricted sandbox initially denied Unix-socket creation; those checks passed
with local socket/app access. Distribution checks caught an omitted transitive SDK
module and incorrect build-tree routing from `isPackaged=false`; both were fixed
and the final default-path package passed. An attempted outer Seatbelt sandbox
prevented Chromium initializing its own sandbox. The final test does **not**
disable Chromium security: it constrains the plain Node adapter/native preflight
separately and runs the copied GUI with its normal sandbox and disposable profile.
Failed intermediate distribution artifacts were removed.

Compatibility remains intentionally incremental. Status omits unported N64
VI/register/executor/ownership fields, transfers are capped at 1024 bytes, mapped
RDRAM writes/DCache remain deferred, and `keep_overrides=true` is explicitly
unavailable until input ownership exists. Read/write result names/hex addresses
are retained with added session/region identity metadata. Full generated FastMCP
JSON Schema equality and full reference-result parity are **not run/complete**;
the ledger records the differences instead of counting six registrations as full
68-tool preservation. No tool was removed from the target.

The packaging output is a local development distribution, not a notarized release.
Windows transport is explicitly unsupported; Linux runtime/platform checks,
release signing/entitlements, full 68-tool adapter/backend assertions, protocol
revisions beyond the tested 2025 MCP initialization, packaged audible audio,
queue/client-count saturation and long soak remain **not run**. Prior pending
manual checks carry forward. Task 3 needed no new native-core rebuild; the tested
native binary is the task 2 binary (the copied artifact gets local rpath/signature
changes only).

## Task 4 — native VI coordinates and owned bounded execution

Implemented in checkpoint `bbe94d832`; final Electron gates passed during task 4.5 below.

Minimum semantics were adapted from the `ares-64` reference at
`1c9b9637476a83f7b80d3db1115a0f71529820d9` (original controlled-run work
`bdcadf33d`). The port retains active VI scanout boundary counting, partial-frame
coordinates, bounded service slices, single completion, bounded final-result
retention and connection-generation ownership. It does not import the fork's
Python/TCP transport, desktop event loop, CPU debug engine or capture/input code.
Reference-file SHA-256 hashes:

| Reference file | SHA-256 |
| --- | --- |
| `ares/n64/system/debug-session.hpp` | `a2d09f5aee1edde6caf7d8a330fe95756d1a6de6ad4ff79f7e17b8305cb44e2f` |
| `desktop-ui/program/debug-session.cpp` | `93c4061ac352f8fc887e793e2d8f5317446c17dabc4e3c7027043dd2c877f700` |
| `ares/n64/cpu/cpu.cpp` | `f897cd8b6a93fa8afe2a6d75644fa114d931ed0246926db4b8d79da69bf94292` |
| `ares/n64/vi/vi.cpp` | `5a0fa9b3fc540e05f499cdbb40a20ff4a234055496aa1a1b4e2a5225f7dd70cd` |

The Electron native loop enables a 20000-iteration CPU slice on N64 without
forcing its interpreter. Active VI boundaries increment the native coordinate;
blank-display liveness yields and renderer credits do not. The synthetic host
uses two slices per explicit synthetic VI to exercise partial-frame starts.
Operations drive the same root loop without normal frame/audio pacing, and leave
ordinary play paused. Buffered native audio is cleared at entry; streams are
drained/discarded during controlled runs. Default desktop use keeps a zero slice
budget. This is bounded outer-loop servicing, not instruction counting or a hard
real-time guarantee for every device/backend call.

The authenticated endpoint supplies connection UUID owners. Competing starts,
resume, writes and cancellation return `busy`; reads/polls remain available.
UI pause interrupts any owner. Reset finishes active work before incrementing the
epoch; native replacement creates a new session. Disconnect releases only that
connection generation. Native request capacity reserves eight cleanup slots; if
cleanup cannot be queued within the bounded reserve, main terminates the host.
One final snapshot is retained per operation (latest 16), including cancel,
timeout, pause, disconnect, reset and shutdown paths. The MCP adapter polls with
its original identity and never rebinds/replays across reset/ROM replacement.

Five tools are added: `run_frames`, `run_until_frame`, `step`,
`operation_status`, `cancel_operation`. Source names/defaults are retained,
with explicit limits (one million relative frames, deadlines 0.001–600 seconds)
and `unavailable` for requested register/capture options. Other systems' bounded
VI runs return `unavailable`. CPU break/watch/register semantics, input leases
and neutralization, state-load epochs and captures remain tasks 5–8. The ledger
continues to target all 68 tools; eleven are now registered as bounded slices.

Long polling keeps the same connection owner: monotonic `localN` request IDs use
a constant-space replay high-water mark instead of exhausting the old 1024-ID
quota. Arbitrary-ID clients retain bounded deduplication and the quota. A focused
1050-request same-connection regression passed. Recent MCP operation handles
also retain their original session/epoch (32 adapter handles; 16 native finals),
so explicit status refresh does not silently rebind a recent old operation ID.

Intermediate verification caught a stale host object because the first build
overlapped final host edits: 19/21 checks passed, while slice-count and executor
assertions saw the old values. The host source was then touched for an explicit
fresh compilation. These intermediate failures are not counted as passing
runtime evidence. A load-sensitive 30 ms transport test was also replaced with
a deterministic held/released reply and a separate timeout; its focused rerun
passed all four transport checks.

The fresh native build completed and **all 21 service tests passed**, including
real MCP stdio, exact/unpaced synthetic VI runs, N64 inactive-VI timeout/cancel,
owner conflict/disconnect/reconnect, reset revocation and immutable finals.
Electron integration remains pending: the existing UI harness reached its
20-second watchdog, and the Electron-hosted MCP harness reached its 8-second
initialize watchdog. These are recorded as failed/incomplete checks, not passes;
their cause has not yet been established. The user requested task 4.5 (TypeScript
and ESM migration) before further shell expansion. That detour will update the
entry points and harnesses, then rerun these gates using this native binary.


## Task 4.5 — TypeScript application sources and native ESM

Requested by the user before further shell expansion. Main, renderer, preload and
frame parser are now strict TypeScript (`main.ts`, `renderer.ts`, `preload.cts`,
`protocol.ts`). Shared services and MCP emit native Node ESM; the renderer and
inspector emit browser ESM. `bridge.ts` types IPC commands, session requests,
frame DTOs and renderer statistics; `renderer-globals.d.ts` types `window.ares`.
The preload emits one CJS artifact because Electron’s renderer sandbox remains
enabled. Its runtime import is only `electron`; bridge imports are type-only.

The root package declares `type: module` and starts `dist/main.js`. Relative Node
imports include `.js`; main resolves assets from `import.meta.url`. Clean builds
remove stale output. Packaging copies compiled application output and sets the
same ESM metadata. Electron test entry points are `.mjs`; standalone build/test
helpers retain explicit `.cjs` extensions. No dependencies or native source were
changed, and no native core rebuild was performed for this detour.

Verification on macOS arm64:

- **Passed:** clean strict TypeScript builds for Node/main/services/MCP, browser
  and sandbox preload.
- **Passed:** `npm test --prefix tools/electron-spike`, all 21 tests, about 7 s in
  the final run. This reruns exact/unpaced synthetic frames, native N64 inactive
  VI deadline/cancel, owners, disconnect/reconnect, immutable results, real MCP
  stdio, bounded memory, stale identities and long polling against the task 4 host.
- **Passed:** `npm run test:electron`, including stalled-renderer responsiveness,
  inspector write/read/restore, running/paused reloads, WebGL frame credits and
  native replacement through the migrated ESM main and sandboxed preload.
- **Passed:** real ESM Electron/MCP integration, eleven tool schemas/defaults,
  shared paused GB bytes, UI pause cancelling owned work, renderer reload keeping
  ownership, explicit cancellation, reset/ROM revocation and native disconnect.
  The retained stage log is `/tmp/ares-esm-mcp.log`; the final integration took
  about 1.7 s after build on the warmed machine.
- **Passed:** separate `artifacts/task-4.5/Ares.app` development bundle and copied
  package test. Bundled Electron 44.9.0 / Node 24.22.0 loaded the ESM application,
  served real stdio and shared/restored GB memory without system Node/Python.
  Checkout reads were denied for native preflight and the MCP adapter; the GUI
  used its normal Chromium sandbox. The fixture presented 453 frames and quit
  normally. The previous task 3 bundle was not overwritten.

Earlier 20 s UI / 8 s MCP initialization / 60 s expanded-suite watchdog failures
were not counted as passes. Harnesses now report stages, permit 30 s cold MCP
initialization and give the expanded app integration a 120 s overall watchdog;
ordinary MCP requests retain their 8 s deadline, and native cancellation/deadline
assertions retain their original bounds. The final warmed run passed quickly.
Task 4’s previously pending Electron integration gates are now closed.

Full generated FastMCP schema equality, all 68 tools, controlled audible N64 game
acceptance, hardware input takeover, long soak and non-macOS runtime checks remain
pending in their planned tasks. The next task is task 5. The user authorized
continuing through subsequent roadmap tasks without stopping after each commit.

- **Passed:** `ARES_TEST_AUDIO=1 npm run smoke`, all 40 registered systems and
  GB/NES identification, synthetic 640×480 59.86 fps / 1080p 59.84 fps,
  original GB fixture with native SDL audio 56.50 fps and Famicom 60.07 fps.
  These are transport/core-cadence checks, not a listening-quality or end-to-end
  latency measurement. Each isolated fixture closed before the next one started.
