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
| TypeScript and contract/transport/native tests | `npm test --prefix electron-ui` | Passed, 8 tests |
| Native running/paused requests, no video credit, stale session/reset epoch, no retry after timeout/disconnect, fragmented replies, malformed/unknown requests | `tests/client.test.cjs`, `tests/session-live.test.cjs` | Passed |
| Actual Electron stalled-renderer control, paused/running reloads, same session, frame credit/WebGL, pause-button resume and stale identity after host replacement | `npm run test:electron --prefix electron-ui` | Passed, `ELECTRON_SESSION_PASS`; disposable app closed |
| Existing load/video/input/pause/resume/shutdown and 40-system discovery | `ARES_TEST_AUDIO=1 npm run smoke --prefix electron-ui` | Passed; all test hosts closed |
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
| Node + browser TypeScript builds and focused tests | `npm test --prefix electron-ui` | Passed, 12 tests |
| Actual N64/GBA region enumeration and inspection | `tests/memory-live.test.cjs` | Passed on paused cores via normal loaders |
| Max-size bulk write/read/restore; full prevalidation at boundaries; stale session, region and epoch rejection | `tests/memory-live.test.cjs` | Passed; N64 RSP DMEM, GBA EWRAM and synthetic RAM restored |
| Read-only/unavailable callback metadata and malformed native payload | Synthetic debugger-node fixture | Passed |
| Explicit endian and unaligned byte-slice handling | `tests/memory.test.cjs` | Passed |
| Real browser inspector writes/readback agree with Node client; original bytes restored | `npm run test:electron --prefix electron-ui` | Passed, `ELECTRON_SESSION_PASS` |
| Stalled renderer, paused/running reload, one-frame queue, WebGL, native pause notifications, host replacement and stale requests | Same disposable Electron test | Passed; app and native hosts closed |
| Inspector layout | `artifacts/memory-inspector.png`, visually inspected | Passed; ignored generated artifact |
| Existing 40-system registry and GB/NES ROM loading, video, input, pause/resume/shutdown | `ARES_TEST_AUDIO=1 npm run smoke --prefix electron-ui` | Passed; generated ROMs, isolated test save paths |
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
| TypeScript builds and focused session/memory/local-transport suite | `npm test --prefix electron-ui` | Passed, 15 tests |
| Token authentication, explicit UUID/permissions, fragmented hello, stale connection UUID, duplicate request, timeout/disconnect/reconnect without replay | `tests/local-endpoint.test.cjs` | Passed; timed-out mutation observed once |
| Actual MCP stdio initialize/list/call, source parameter names/required fields/defaults, six-tool inventory | `npm run test:mcp --prefix electron-ui` | Passed |
| MCP writes and inspector reads the same paused **real GB CPU WRAM**; original bytes restored; native session retained across renderer reload | Same test with generated GB ROM and isolated profile/saves | Passed, `MCP_INTEGRATION_PASS` |
| Wrong app selection, stale reset epoch, stale session after host replacement, native-host disconnection, unsupported tools/override flag | Same real stdio test | Passed, explicit errors |
| Existing frame-credit/WebGL/inspector/reload/stalled-renderer behavior | `npm run test:electron --prefix electron-ui` | Passed, `ELECTRON_SESSION_PASS` |
| Local distribution construction | `npm run package:mac --prefix electron-ui` | Passed; native host and full production SDK graph included |
| Copied distribution outside repository; generated GB ROM; bundled stdio runtime; read/write/restore/resume, clean app/MCP exit and metadata cleanup | `npm run test:package --prefix electron-ui` | Passed, `PACKAGE_MCP_PASS`, 455 rendered GB frames, no WebGL error |
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
- **Passed:** `npm test --prefix electron-ui`, all 21 tests, about 7 s in
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

## Task 5 — N64 CPU debug engine and strict memory

Source: read-only `ares-64` checkout at
`1c9b9637476a83f7b80d3db1115a0f71529820d9`. Ported only the break/watch/step,
architectural snapshot and renderer synchronization dependencies. Tracing,
stack unwinding, call gates and their exception hooks remain deferred.

| Reference file | SHA-256 |
| --- | --- |
| `ares/n64/cpu/debug-control.hpp` | `b98961f629793d13ef1ec1939a645901b5a4536dd33a1a86454342393b8e7baf` |
| `ares/n64/cpu/debug-control.cpp` | `6952c5c0d7898ff6f0dfd6d73c98e67ff34c12940ba6c2d57285d8fa637e1597` |
| `tests/mcp/live_fixture.py` | `72a867054ee987cd10b43013c56cb5c3ed1a39d60d667a524402d3918e5b334b` |
| `desktop-ui/program/telemetry-server.cpp` | `738abe2ed5dd789b7b9968bd0f77c45ab03ddcecdf81e9106e962d250799646b` |
| `ares/n64/vulkan/vulkan.cpp` | `1906b8079516c483ab3116cd5b48620e1fd21d2b34c4c5f0bd87736746451745` |

Implementation:

- CPU interpreter gate while definitions or instruction stepping are active;
  original recompiler setting is preserved. Breakpoints stop before fetch;
  watchpoints latch semantic CPU data access ranges and stop after completion.
  Partial big-endian loads narrow the aligned container to consumed bytes.
- Owned instruction operations use the existing coordinator, deadlines, cancel,
  disconnect cleanup and immutable completion retention. CPU stop snapshots are
  copied into completed operation results. Clearing transient stepping restores
  recompiler eligibility without clearing persistent definitions.
- Side-effect-free GPR/HI/LO/PC, delay-slot, raw FPR and COP0 snapshots; Random is
  null because reading it advances RNG. Immutable hit records include accessing
  PC, opcode, effective/physical range and old/new or read value.
- Separate typed `n64.debug.*` and `n64.memory.*` services, lossless hex addresses,
  eight predicates per definition and paged definition listings (32 per reply).
  MCP adds twelve tools, for 23 registered tools out of the 68-tool inventory.
- Strict physical/KSEG0/KSEG1 installed-RDRAM bounds; TLB/MMIO/wrapped requests
  reject. CPU view reads or patches resident D-cache without allocating a miss;
  KSEG1 bypasses D-cache. Writes prevalidate all bytes, flush Vulkan, invalidate
  JIT and report dirty-D-cache/resident-I-cache hazards. They preserve I-cache.
  Writes reject `rdram_not_initialized` while chip mapping could drop them.
- Main observes session status every 250 ms with at most one observation pending,
  independently of renderer/video progress. CPU stops during ordinary play can
  update the Pause/Resume button without requiring a frame or another UI action.
- Local build recipe now disables IPO/LTO through the existing CMake option.
  The first switch required recompiling all cores. The no-LTO link exposed an
  unused nall entry-point wrapper; macOS host/fixture targets now dead-strip it.
  Application TypeScript builds still never invoke CMake.

Evidence so far: native host and dedicated ROM-free CPU fixture build passed;
24 service tests passed before adding the main state observer regression. The
core fixture executes actual hand-assembled MIPS code, checks delay slots,
pre-fetch versus post-access stops, LWL/LWR ranges, aliases, frozen snapshots,
CPU/RDRAM divergence, prevalidated memory writes and actual JIT dispatch after
removing debug hooks. Real isolated N64/MCP tests check default one-instruction
steps, live registers, definitions, stale epochs, memory bounds, early-init write
rejection, busy mutations, owner disconnect and transient executor restoration.

Intermediate failures were fixture setup/build issues (missing include root,
unexported template use, unallocated hidden RDRAM metadata, unloaded debugger
nodes, uninitialized RAM, disabled fixture recompiler), an installed-size test
assumption, and incorrect construction of the stdio test helper. The latter left
one adapter alive; its exact identified PID was closed before continuing. No
active user emulator or user saves were used. Final Electron/package gates and
updated test count will be recorded below.

Known bounds: raw FPR bits rather than reference classified float views; no
heuristic stacks/code-window enrichment; no delayed write verification yet;
1024-byte transfers; predicates/hits are CPU-only (not DMA/fetch/debugger access).
Uninitialized/nonidentity RDRAM writes explicitly reject. Full generated schema
and result parity, little-endian user-mode watch behavior, audible N64 gameplay
and cross-platform acceptance remain unverified. These are not implied by the
fixture assertions.


Task 5 final verification on macOS arm64 (2026-10-19):

| Check | Result |
| --- | --- |
| Native host + ROM-free CPU fixture builds | Passed; all default 40 systems retained |
| `npm test` | Passed, 25 tests in 7.1 seconds, including actual JIT dispatch after debug disable |
| `npm run test:electron` | Passed: sandboxed preload, stalled renderer controls, inspector restore, paused/running reload |
| `npm run test:mcp` | Passed: 23-tool registry, shared inspector/MCP bytes, owner/UI cancel, renderer reload, replacement and disconnect |
| `npm run package:mac -- artifacts/task-5/Ares.app` | Passed, separate ignored task-5 artifact |
| `npm run test:package -- artifacts/task-5/Ares.app` | Passed outside checkout, bundled Node 24.22.0/Electron 44.9.0, real stdio, normal GUI sandbox, 454 GB frames, checkout reads denied for native/MCP preflights |
| `ARES_TEST_AUDIO=1 npm run smoke` | Passed: 40 systems, GB/NES detection, synthetic 640×480 59.88 fps, 1080p 59.72 fps, GB with SDL audio 56.49 fps, Famicom 60.08 fps |
| `git diff --check` / ledger inventory | Passed; 68 target tools, 23 registered bounded implementations |
| Timed incremental native host compile + link without IPO | Passed in 9.93 seconds (`/usr/bin/time -p cmake --build ... --target ares-electron-host -j 8`) |

Task 5's bounded roadmap gate is complete; the explicit parity/manual limitations
above remain. Next: **task 6, input ownership, traces and provenance**. N64 service
and MCP checks used a dummy cartridge only to initialize the normal loader; the
separate ROM-free MIPS fixture supplies instruction/cache assertions. No full-game
N64 boot or listening-quality audio claim follows from these checks.

## Task 6 — input ownership slice (task remains in progress)

Implemented a bounded four-port input slice after task 5 commit `ade7025a5`.
Tracing, DMA provenance, profiling and controller taps remain unimplemented; the
milestone 2 gate is not complete. Physical keyboard/gamepad acceptance remains
manual and has not been claimed.

Adapted `ares-64` input/latch/lease and handoff semantics at
`1c9b9637476a83f7b80d3db1115a0f71529820d9`, preserving attribution:

| Reference | SHA-256 |
| --- | --- |
| `desktop-ui/program/telemetry-server.hpp` | `dd41280a2bb852e4f515e8d7313c5b1518bf761fc0c971f8abf6e1d916f987cb` |
| `desktop-ui/program/debug-session.cpp` | `93c4061ac352f8fc887e793e2d8f5317446c17dabc4e3c7027043dd2c877f700` |
| `desktop-ui/program/telemetry-server.cpp` | `738abe2ed5dd789b7b9968bd0f77c45ab03ddcecdf81e9106e962d250799646b` |

`InputService` replaces complete held states on four independently owned ports,
separates pending and VI-latched state, and applies overrides before normal host
input/focus filtering. Operation start/finish latch state, with neutral input on
unoverridden ports during controlled execution by default. Frame leases use
native VI coordinates; steady-clock leases expire even while paused. Rumble
continues to the host. Expiry, disconnect and epoch changes revoke holds.

Connection-generation owners come from the existing private endpoint. A foreign
caller cannot overwrite or release another connection's port. Null release and
default external resume release only the caller's holds; UI resume can release
all. N64 `resume(keep_overrides=true)` now preserves holds. This is an explicit
multi-client adaptation of the reference's single-client behavior.

New keyboard/button presses and gamepad stick movement request takeover through
an atomic flag. Only the emulation thread finishes an active operation
(`host_input`), releases injected input and resumes ordinary play. Repeated held
buttons do not request takeover; stick changes use a 4096-unit threshold to avoid
noise. Wall leases use 1 ms precision. Other systems keep their existing input
path. Internal owner-release uses task 4's reserved cleanup capacity.

Added typed `n64.input.*` services and MCP `controller_set`, `controller_release`,
`controller_state`, `controller_host_mode`: 27 registered bounded tools, 41 still
unregistered out of the preserved 68. `controller_tap` remains pending. Input
state returns masks/owners/frame deadlines; full legacy result keys and wall-time
lease diagnostics remain parity work.

Evidence: native host and controller/core fixture builds passed. The 25-test
service suite passed with real controller Button/Axis/Rumble nodes, pending/VI
latches, independent owners, frame expiry, wall expiry while paused, epoch
revocation, disconnect cleanup, explicit preserve-on-resume and generated host
keyboard events. Real N64/MCP stdio verifies that takeover interrupts an owned
instruction operation and returns ordinary play. These are generated events,
not physical-device acceptance. Electron/MCP checks passed with all 27 tools;
final ordinary-play smoke and final owner-scoped resume regression are recorded
below when complete. The task-5 packaged artifact remains unchanged; this input
checkpoint has not yet been repackaged.

Input checkpoint final checks: 25 service tests passed in 7.6 seconds, including
owner-scoped default resume and preserved overrides. Electron/MCP passed with 27
tools. Audio-enabled ordinary-play smoke passed: 40-system registry and GB/NES
detection, synthetic 640×480 59.69 fps, 1080p 59.87 fps, GB/SDL 56.50 fps,
Famicom 60.11 fps. Diff check passed. Test instances were closed. Task 6 remains
in progress; next implementation slice is PC/call traces, then DMA/profiling.

### Task 6 PC/call trace slice

Ported the reference `debug-control.*` trace structs, instruction hook and
PC/call aggregation from the same audited `ares-64` commit/hash recorded in task
5. No stack unwinder, call-gate hooks, execution-trace file writer or desktop
profiler viewer was imported. Added typed `n64.trace.start/page` and MCP
`trace_pc`, `trace_calls`, `trace_page`: 30 registered bounded tools, 38 still
unregistered. Task 6 remains incomplete (DMA provenance, profiling and taps).

Traces are owned operations over native VI intervals, including future-start
preroll. They force the interpreter, honor deadlines/cancel/human takeover/owner
disconnect, and restore executor eligibility afterward. PC records sort by
frame/PC. Calls decode JAL/JALR/taken branch-link before execution, with optional
GPR and raw f12/FR snapshots. Most-recent completed pages are immutable until
superseded/reset; stale capture IDs reject.

Safety/bounds adaptations: effective storage is capped at 262144 PC records or
65536 calls and reported as record_capacity. Native pages contain at most 64 PCs
or 32 calls; MCP accepts reference inline/page defaults but reports an effective
1024-entry response limit and next_offset. File paths explicitly return
unavailable until task 7. The non-RDRAM PC map is bounded before VI flush, with
separate overflow observations. Histogram counters saturate instead of wrapping;
truncation is explicit. Histogram vectors/maps release their allocations when
capture finishes. Retained records remain available for paging. The trace hook
has an out-of-line definition so fixture callers do not depend on incidental
inline-symbol emission in no-LTO builds.

Validation: native host/core fixture builds passed; 25 service tests passed in
8.7 seconds. The ROM-free fixture executes actual branch loops and JALR rd=rs,
checks bucket counts and half-open filters, record limits and target exclusions,
serializes a nonempty linked-call page through the native service and decodes it
with the shared JS contract. Direct accumulator boundary probes cover bounded
non-RDRAM storage and counter saturation. An additional real-JIT block assertion
passes after disabling tracing and verifies that no trace observations accrue.
Real N64/MCP tests cover timeout, inline/page results, frozen pages, superseded
IDs, future intervals, competing owners and disconnect cleanup. Electron/MCP
passed with the 30-tool registry, renderer reload, UI cancel, replacement and
shutdown. Final link/fixture verification for the out-of-line hook is recorded
below before committing. Input's audio-enabled smoke remains the latest ordinary
play/audio gate; no audible N64 gameplay or physical controller acceptance claim.

Trace checkpoint final link and 25-test suite passed in 8.9 seconds after making
the hook definition out-of-line. The fixture includes a real, nonempty JALR page
with target/link/selected-register/f12 values decoded through the shared reply
validator. Diff check and 68-row/30-registered ledger checks passed. Next slice:
PI DMA provenance. Task 6 is still in progress.

### Task 6 — bounded PI DMA provenance

Added opt-in PI cart-to-RDRAM transfer provenance and `overlay_log`, bringing the
registered MCP inventory to 31 of 68. The 4096-entry ring records initiating PC,
epoch/frame, requested length, attempted writes and bounded destination ranges.
Confirmed byte counts and code-candidate overlap require a linear initialized
RDRAM map; unknown maps return null rather than claiming writes were confirmed.
Recording is enabled before boot in service-enabled Electron sessions. Late
activation and ring overwrite explicitly report incomplete history; reset begins
a new epoch before power-on and revokes input and trace state first.

Source: ares-64 commit `1c9b9637476a83f7b80d3db1115a0f71529820d9`.
`system/debug-session.hpp` SHA-256:
`a2d09f5aee1edde6caf7d8a330fe95756d1a6de6ad4ff79f7e17b8305cb44e2f`;
`pi/dma.cpp` SHA-256:
`231f5f5eeb1753e80d66e043630f122d51985c98204df52a2f263ab9d9820434`.
Semantic adaptations and bounds are recorded in MCP-PORT-PLAN.md.

Validation: native host and fixture builds passed; all 25 tests passed in 9.04s.
The fixture executes actual PI DMA initiated by MIPS stores, checks destination
bytes, misalignment and CPU-watchpoint exclusion, and stresses 4097 transfers to
verify overwrite accounting. Native pages are decoded through the JS contract.
Electron/MCP additionally loads an original dummy N64 image, verifies boot-time
recording, instruction stepping and renderer Resume state without video. An
initial integration invocation aborted under the sandbox; the authorized GUI
invocation passed. Task 6 still needs profiler tools and controller_tap; physical
keyboard/gamepad acceptance and updated task-6 packaging remain outstanding.

### Task 6 — CPU cost profiling and bounded controller taps

Added `profiler_control`, `profiler_clear`, `cpu_profiler_top` and
`controller_tap`: 35 registered MCP tools out of the preserved 68-tool target.
Capture is disabled by default, emulation-thread owned and connection scoped.
It records real JAL/standard JALR calls, return arrival and synthetic exception
frames, with COUNT-safe 187.5 MHz master ticks and exclusive/inclusive RDRAM bus
bytes. Continuous totals retain the reference 1000 observed VI-origin-swap limit;
last-frame data remains fresh. Maps cap at 65536 functions, depth at 1024 and
parked stacks at 16; dropped updates and frozen accumulation are explicit. Top
responses cap at 64. Symbol loading, name-based wait classification and UI event
rings remain deferred, explicitly reported instead of fabricated.

Source: ares-64 `1c9b9637476a83f7b80d3db1115a0f71529820d9`.
`ares/n64/cpu/profiler.cpp` SHA-256:
`03fd5a87186305a82f958507d0b5a6a369dc9c1535cd683c5f931f5ac7ca7993`;
`ares/n64/cpu/cpu.hpp` SHA-256:
`f68e26b03b73a5fe681f1e10a749a67e5a2330cfb1c6f41b463ed3d915be7fbc`.
Tap policy derives from the already journaled telemetry-server/debug-session
sources. Per-connection adaptations are recorded in MCP-PORT-PLAN.md.

Profiling retains the configured recompiler. The profiler and debugger tracer
share one prologue gate and only invalidate compiled blocks when that gate
changes. A real fixture caught two JIT attribution defects: internal entry labels
skipped return-arrival hooks, and callsite instruction-cache refills were charged
to the newly opened callee. The hook now follows entry labels and cache refill;
profiling uses the counted C++ refill path rather than the inline JIT copy.
Initial validation also exposed capability ordering and the fixture's missing
standalone JIT budget. Those failures were corrected before the final passing
run. No active user emulator, ROM or saves were used.

Taps are one native owned VI operation, not a pair of RPCs separated by sleep.
The button latches at start, the tapped port stays stable, and every stop shares
cleanup. Results freeze held_frames, released and ownership. keep_override
retains a neutral port on ordinary stops; disconnect/reset/shutdown/human takeover
revoke it without reacquiring ownership.

Validation so far: native host/core fixture builds passed. All 25 tests passed in
9.66s, including actual MIPS call cost (106 master ticks and 32 RDRAM bytes),
COUNT flush, syscall/ERET nesting, JIT call-return into a loop, shared tracer gate,
map cap, exact two-VI tap scheduling and all nine cleanup reasons. Live N64/MCP
checks cover profiler ownership/clear/disconnect, tap timeout/cancel/disconnect,
neutral retention, stable port and immutable results. Electron/MCP passed with
35 tools and actual N64 profiler/tap calls; renderer stopped-state/reload tests
also passed. Ordinary audio-enabled smoke passed: 40-system registry, GB/NES
loading, synthetic 59.87/59.90 fps, GB SDL audio 56.50 fps and NES 60.07 fps.
Task-6 package validation is recorded below. Physical keyboard/gamepad acceptance
and audible N64 gameplay remain not run; milestone 2 is not declared passed.

Task-6 package gate passed: `npm run package:mac --prefix electron-ui --
artifacts/task-6/Ares.app`, then `npm run test:package --prefix electron-ui
-- artifacts/task-6/Ares.app`. The copied app outside the checkout passed native
loader and bundled MCP preflights with repository reads denied, the 35-tool
inventory, GB memory write/read/restore/resume and clean shutdown. Electron
44.9.0 / bundled Node 24.22.0; 455 rendered GB frames. This remains an ad-hoc-signed
development app, not a distribution-signing/notarization claim. Next work: task 7
artifact publishing and trace exports; carry the milestone-2 physical-device gate.

### Task 7 — atomic artifact publishing and trace export (partial)

Added main-owned `artifact.traceExport` and a reusable asynchronous artifact
publisher. `trace_pc` / `trace_calls` path and overwrite parameters now export all
retained entries independently of the inline limit; registry remains 35 tools.
Native code supplies immutable bounded pages. Electron main streams four pages
per batch and owns filesystem access; the MCP adapter does not open client paths.
Exports cap at 64 MiB and a 25-second deadline checked between I/O operations,
with two jobs globally and one per connection. Owner release, detach and stale
epochs revoke unfinished jobs. Private exclusive sibling temporaries, fsync and
atomic hard-link no-overwrite / rename overwrite publication preserve old output
on failure. Only the final small publication step is synchronous to prevent a
main-thread lifetime transition between guard and publication. Relative paths
resolve against the app working directory. Results include size/SHA-256; export
failure returns file_error after capture, without implying emulation rollback.

Source: ares-64 `1c9b9637476a83f7b80d3db1115a0f71529820d9`,
`desktop-ui/program/artifact.hpp` SHA-256
`f0b03467c0bbeff3dcad11a68a942ee07b1b41966a370a3679efd8e0ec5e6297`.
Version-1 trace envelope/entries derive from the already journaled
`desktop-ui/program/debug-session.cpp`. Shared formatting preserves named GPRs
and raw f12/FR for MCP and files. Distinct NATIVE_METHODS/main capabilities keep
host paths out of the native wire. The router's status observer also advertises
main capabilities consistently across renderer reload and operation completion.

Validation: final 27-test suite passed in 9.45s. Tests cover a two-writer
no-overwrite race, explicit replacement, failed/oversized/revoked exports,
dangling symlink refusal, temporary cleanup, whole-file SHA-256 and streamed
multi-page order. A real core JALR page exports selected registers and f12 through
the shared formatter; real N64/native/local/MCP exports nonempty PC data and call
files. Electron/MCP passed including actual main-owned native trace publication
with inline_limit=0, stopped UI and renderer reload. Native core was unchanged,
so no full-core rebuild or repeated ordinary-play smoke was needed. Packaging
check follows below. Screenshots, native image/audio taps and recording remain
unimplemented; physical-device milestone 2 acceptance remains not run.

Task-7 trace package passed: built `artifacts/task-7-traces/Ares.app`, then copied
it outside the checkout for `test:package`. Bundled Node 24.22.0 / Electron 44.9.0,
35-tool inventory and main artifact capability passed, along with GB memory
write/read/restore, resume and shutdown (452 frames). Native loader and MCP
adapter could not read the repository. No screenshot/audio/recording claim.

### Task 7 — native images and bounded screenshot transport (partial)

Added screenshot, bringing the verified MCP registry to 36 of 68. N64 framebuffer
reads capture actual pre-VI RDRAM RGBA5551/RGBA8888 pixels, with renderer flush,
VI-derived dimensions, clipping and source metadata. Presented snapshots use
native post-screen output (post-VI on N64), independently of canvas credits.
A small optional frame-submission observer runs on the emulation thread after
previous refresh completion and before queueing the next worker refresh. The
host tags epoch/VI at submission, avoiding unsynchronized render-thread reads
of mutable session coordinates and rejecting delayed pre-reset output. It is
installed before execution and removed after unload; default frontends perform
no capture work. Services-enabled Electron starts capture at boot. Raw hosts
can opt in with --debug-capture or enable future capture with the first query.

Native snapshots are immutable and connection owned: two pins, 30-second leases,
16 KiB raw pages. Main fetches four pages per batch, uses a worker for crop,
nearest-neighbor scale and RGB8 PNG encoding, and publishes through the shared
atomic writer. Inline PNGs use two owned 30-second leases and bounded chunks;
MCP verifies SHA-256 before returning ImageContent plus metadata. Source/output
caps are 4194304 pixels, scale 0.125..8 and 8 MiB PNG. Jobs retain the 25-second
deadline checked between work. Default non-inline output uses a unique host
/tmp PNG. Reset clears caches (no_frame until fresh output); generic core frame
coordinates explicitly report unavailable rather than fabricating VI semantics.
A one-pixel RDRAM framebuffer remains valid; presented 1x1 invalid GPU scanout is
rejected, matching the reference distinction.

Reference: ares-64 `1c9b9637476a83f7b80d3db1115a0f71529820d9`.
`desktop-ui/program/platform.cpp` SHA-256:
`0c1f8ec210b4d82727278f6efd28b2d8e51b2fc8950680597a4ea04e94a0bb8e`;
`desktop-ui/program/telemetry-server.cpp` SHA-256:
`738abe2ed5dd789b7b9968bd0f77c45ab03ddcecdf81e9106e962d250799646b`.
Publication uses the already journaled artifact.hpp semantics. Transport,
submission tagging, reset behavior and worker encoding adaptations are recorded
in MCP-PORT-PLAN.md.

Validation: native host and CPU fixture built. Final 30-test suite passed in
11.95s: actual RDRAM 16/32-bit pixel decoding, immutable pins, ownership/ranges,
expiry, blank and one-pixel VI, delayed old refresh rejection, worker crop/scale
pixels, PNG decoding by macOS sips, refused overwrites, pixel budgets and owner
cancellation. The initial 5551 assertion had an overlong expected hex literal;
corrected before the passing run. Code review caught the one-pixel framebuffer
versus presented distinction and added a focused regression. Real isolated MCP
returns decodable image content with no video acknowledgements, checks source
pixels/hash, saves PNG, rejects stale/reset/invalid-region/foreign-owner cases
and revokes image leases on disconnect. Electron/MCP passed all 36 tools and
actual nativeImage PNG decoding, paused reload, stopped state and replacement.
Ordinary-play smoke with capture disabled passed: registry/GB/NES, synthetic
59.75/59.72 fps, GB native SDL audio 56.49 fps and NES 60.08 fps. Package result
follows. Audio taps/recording and run_frames screenshot_after remain task 7 work;
physical keyboard/gamepad and audible N64 gameplay acceptance remain not run.

Task-7 image package gate passed: built artifacts/task-7-images/Ares.app and ran
copied-app test:package outside the checkout. Bundled Electron 44.9.0 / Node
24.22.0, 36-tool inventory, main artifact capability, native GB presented capture,
PNG worker loading, inline image/png content and byte-identical atomic PNG file
all passed. GB memory restore/resume/shutdown passed (450 frames); native loader
and MCP adapter ran with checkout reads denied. Development ad-hoc signing only.

#### Screenshot publication boundary follow-up

Review found a lifetime race after successful file publication: a later guard
could report cancellation despite an already saved PNG, or an inline-transfer
failure could hide the published file metadata. Inline-cache installation now
shares the synchronous file commit point. A later owner release can revoke that
cache without rewriting completed publication as failure. MCP partial errors
retain capture/path metadata if inline transfer fails after a file was saved.
The 30-test suite passed in 11.56s after the commit-point change; the focused
screenshot suite then passed all three cases, including post-publication lifetime
transition and a failed inline transfer preserving path/epoch. Native code is
unchanged; no native rebuild was needed. Final package evidence follows.

Publication follow-up final validation: all 31 tests passed in 11.52s. Built
artifacts/task-7-images-final/Ares.app and copied-app package verification passed
again: 36-tool inventory, native capture/worker/inline PNG, identical saved PNG,
GB memory restore/resume and clean shutdown (450 frames), bundled Node 24.22.0.

#### Profiler shared-tracer invalidation follow-up

Review found that profiling changes generated icache refill paths even when the
instruction debugger already keeps the shared prologue enabled. A profiler toggle
now invalidates compiled blocks in that case as well (one reset, without forcing
the interpreter). This corrects the earlier task-6 journal statement that only
prologue-gate changes require invalidation. A real JIT regression compiles and
executes debugger-only blocks, enables profiling with the tracer still enabled,
checks invalidation, then verifies the expected 32 cache-refill bytes. Host/core
fixture rebuilt; all 31 tests passed in 11.96s. Final combined package follows.

Combined session package: artifacts/session-checkpoint/Ares.app includes the
profiler shared-tracer fix and screenshot publication fixes. Build and copied-
app test:package passed: 36 MCP tools, bundled worker/native PNG/inline image and
matching file output, memory restore/resume/shutdown, 451 GB frames. Node 24.22.0 /
Electron 44.9.0; repository reads denied for native loader and MCP adapter. Final
source suite: 31/31 passed. Next task-7 slices are audio capture, recording and
screenshots attached to run operations; milestone-2 physical-device gate is still
pending. All verification emulator instances were isolated and closed.

### Task 7 — screenshots after bounded runs

Implemented `run_frames(screenshot_after={...})` using the existing bounded PNG
pipeline. Like the reference (`desktop-ui/program/debug-session.cpp` finish path
and `telemetry-server.cpp` run validation, reference commit
`1c9b9637476a83f7b80d3db1115a0f71529820d9`), this accepts host-file output and
rejects inline output before starting the run. Crop/scale/path options validate
before emulation. Every stop reason retains its execution result; failed capture
adds `screenshot_error` and `emulation_already_advanced` without rollback.

Adaptation: file work stays in Electron main, and the native pixel pin is requested
after operation polling completes. The original operation identity is retained;
a native paused/current-frame predicate rejects intervening advancement with
`capture_frame_changed`. Presented output retains its actual submitted-frame tag
and can lag the current VI. Pixel pinning is immutable, so later execution cannot
change a successful capture's bytes. This is not a reservation across the run and
capture requests; intervening clients can cause a reported capture failure.

Validation: rebuilt native host and CPU fixture; TypeScript build and all **32**
sequential tests passed (12.78 s). Real stdio tests cover run+decodable PNG output,
file-exists failure after advancing, inline rejection before advancing, and native
frame-race rejection. Unit coverage verifies old-epoch identity and preservation
of breakpoint results on capture failure. MCP inventory stays **36** tools.
Audio/recording and physical input acceptance remain pending.

### Task 7 — bounded native audio capture and atomic WAV export

Selective port of `audio.capture/status/save` semantics from reference commit
`1c9b9637476a83f7b80d3db1115a0f71529820d9`:
`desktop-ui/program/telemetry-server.cpp` SHA-256
`738abe2ed5dd789b7b9968bd0f77c45ab03ddcecdf81e9106e962d250799646b`,
`desktop-ui/program/platform.cpp` SHA-256
`0c1f8ec210b4d82727278f6efd28b2d8e51b2fc8950680597a4ea04e94a0bb8e`.
No legacy transport, viewer or continuously running ring was imported.

Implemented native post-mix stereo capture before sound/controlled-run output
muting, at the host resampler's 48000 Hz rate. Maximum 192000 frames; buffers
freeze at cap instead of overwriting their prefix on late save. Capture is opt-in
and connection-owned. Reset/disconnect revoke it; foreign mutations fail busy.
Status is read-only and reports unavailable streams without fabricated samples.
Partial/zero-frame saves work. One native immutable PCM pin has a 30-second lease;
main's export has a 25-second deadline and bounded 768044-byte WAV output.

Electron main builds 16-bit little-endian stereo WAV and uses the shared atomic
publisher. Reference `audio_save` replaces paths without an overwrite flag;
preserve that behavior with atomic replacement. Failed publication retains the
capture for retry. Successful publication consumes only its pinned capture; late
cleanup failure preserves file metadata and reports `cleanup_error`.

Validation passed:
- Native host/CPU fixture rebuild, strict TypeScript build, **35/35** sequential
  tests (18.57 s), including PCM clamp/endian, cap freeze, immutable partial pin,
  owner/range checks, atomic WAV headers/samples, failed save/retry, cancellation
  and publication lifetime behavior.
- Real GB tone through native host with host output muted, local endpoint and
  actual MCP stdio: full four-second nonzero PCM, WAV/hash/size, duration clamp,
  destination replacement, zero-frame WAV, reset and disconnect revocation.
- Real Electron MCP integration: exact **39-tool** registry, 960-frame native GB
  WAV, existing UI/memory/reload/execution/N64 checks, and synthetic unavailable
  audio behavior.
- `artifacts/task-7-audio/Ares.app` packaged and copied outside the checkout;
  native loader/MCP adapter denied checkout reads. All 39 tools, PNG worker/file
  output and WAV header/file/consumption verified; GB ran 444 displayed frames.

Task 7 video recording remains next. Milestone 2 physical keyboard/gamepad and
audible N64 gameplay acceptance remain **not run**. No full 68-tool parity claim.

### Task 7 — bounded presented-frame recording and streamed AVI export

Selective port from reference commit `1c9b9637476a83f7b80d3db1115a0f71529820d9`,
`desktop-ui/program/video-recording.hpp` SHA-256
`05d335af9d50c9726a4b52426a5e96d50bb34d6c283d9f921bb0882df30a9b35`.
Retained source frame/byte/time/fps defaults and ranges, padded bottom-up BGR24,
AVI 1.0 DIB stream/index and no-audio format. Native observes actual tagged frame
submissions independent of video credits; main streams immutable pages to atomic
file publication instead of constructing a second entire movie in memory.

Added connection ownership, export pins (30-second lease), a 25-second main
export deadline and 270000000-byte artifact ceiling. Failed exports retain stopped
frames for retry; successful exports consume only the pinned recording. Reset and
disconnect discard owned buffers. Presented capture also works on generic cores
with explicitly unavailable frame coordinates and on the isolated VI fixture.
Native frame taps share the screenshot source-pixel bound (4194304). Active
recording cannot be replaced; stopped data requires explicit discard_previous.

Validation passed:
- Native host and CPU fixture rebuilt. Fixture assertions cover exact frame and
  byte limits, wall deadline, dimension changes, old epoch rejection, ownership,
  immutable export pin/discard exclusion, read bounds and bottom-up BGR padding.
- Strict TypeScript build and **37/37** sequential tests (25.78 s). ffprobe and
  ffmpeg independently decode the AVI's dimensions, codec, playback fps, frame
  count, orientation and exact RGB pixels. Atomic overwrite/no-overwrite,
  failed-export retention, retry and revocation/temp cleanup pass.
- Real MCP stdio captures two full 640x480 synthetic presented frames while video
  acknowledgements are withheld; confirms actual frame tags, limits, timeout,
  no-frame failure, retry after existing-path refusal, hash, reset and disconnect.
- Real Electron integration verifies exact **42-tool** inventory and native GB
  AVI/WAV plus existing image/memory/CPU/ownership/reload checks.
- `artifacts/task-7-recording/Ares.app` packaged and copied outside the checkout;
  loader/MCP denied checkout reads. PNG, empty WAV and two-frame GB AVI exports
  pass alongside the 42-tool registry. GB displayed 434 frames and shut down.

Task 7's bounded implementation is complete. Task 8 states/experiments is next;
physical keyboard/gamepad and audible N64 gameplay acceptance remain not run.

### Task 8 — initial state compatibility audit (read-only)

Compared all 16 current N64 `serialization.cpp` files against the reference fork.
Fifteen are byte-identical. The system file differs only by refactoring header and
prefix serialization and adding state-version/RDRAM-layout observation; it keeps
the same field order. Both use `BST1` signature `0x31545342` and N64 version `v153`.
This is a useful starting point, **not runtime compatibility acceptance**: build
configuration, configuration-dependent devices and serializer helpers still need
verification, followed by disposable cross-binary save/load tests. The next
implementation should expose version/layout observations without loading the live
emulator, validate whole-file bounds before restoration, and keep offline state
inspection independent of emulator mutation. No state tools registered yet.

### Task 8 — native state save and offline inspection checkpoint

Implemented `state_save`, `state_info`, `state_list` and `state_peek`; registry is
now **46 of 68** tools. This is a task 8 slice, not completion of loading or
experiments. Selective reference commit:
`1c9b9637476a83f7b80d3db1115a0f71529820d9`.
- `ares/n64/system/serialization.cpp`: SHA-256
  `f19255ef37dd7ef482aee659d8892034a3a2acce9919ba8bb46a631bdefbf96c`
- `ares/n64/system/system.hpp`: SHA-256
  `07ad9f64c178dcf4981322ae20fd3df6ef51e28803a4944b46abf9534b2369b6`
- `desktop-ui/program/states.cpp`: SHA-256
  `9149868f5262eee42f2be3f65f4f789b887238979d5dd830a44dd967c1706fa7`
- Header/list/peek and metadata behavior uses the previously audited reference
  telemetry-server.cpp. Fifteen other N64 serialization.cpp files remain identical.

Refactored the serializer header/prefix without changing field order/version.
The native observer reports `v153`, `BST1`, 533-byte header and configuration-
dependent RDRAM offset/size. A fixture independently assembles the reference
header/prefix and checks layout equality and CPU/RAM nonmutation. Header support
is explicitly not machine/configuration/load acceptance.

Native state saving pauses at a safe point, rejects active execution/GDB and
pins one immutable snapshot (64 MiB max, 30-second lease). Main streams pages to
atomic file publication and separately publishes hash/ROM/layout metadata.
Sidecar failure preserves the state-file result as metadata_error. Main exports
are bounded to 25 seconds; owner release/reset revoke the snapshot.

Offline inspection reads only bounded regular files (including direct symlinks);
nonblocking open rejects FIFOs. Header-only info does not pause/serialize CPU or
restore a machine. Inventory scans at most 4096 regular directory entries, hashes
at most 64 MiB of file content and returns 48 KiB pages with next_offset. Native
slot paths match legacy names but do not call locate(), which creates directories.
Saved RDRAM reads use validated hash/layout metadata, or explicit current-
configuration assumption through prefix observation only. Ranges are lossless
physical/KSEG aliases, capped to 1024 bytes; saved CPU-cache view is unsupported.

Validation passed:
- Native host and CPU fixture build; strict TypeScript; **41/41** sequential tests
  (23.89 s final run). Covers stale/malformed sidecars, header/version/truncation,
  address/endian/range checks, 4096-entry scan and response bounds, huge/special
  files, cancellation/cleanup, atomic state+sidecar failure boundaries and retry.
- Actual paused N64 native save with real MCP stdio, followed by hash-validated
  saved-RDRAM reads agreeing with live backing-store bytes; registers unchanged.
  Foreign snapshot access, ranges, busy pin and disconnect revocation pass.
- Real Electron MCP: exact 46-tool registry, real N64 state save/peek plus prior
  CPU/input/trace/image/audio/recording/reload checks pass.
- Packaged `artifacts/task-8-state-inspection/Ares.app`; copied-app test passes
  with loader/MCP checkout reads denied, 46-tool registry, offline header/list/
  hash-sidecar reads and explicit non-N64 save rejection. GB displayed 155 frames.

Remaining task 8 work: preflighted safe load/new-epoch behavior, cross-binary
restore compatibility, multiwrite/delayed verification and bounded batch sweeps.
No existing fork state was loaded or accepted solely from its version/header.
Milestone 2 physical keyboard/gamepad/audible N64 acceptance remains not run.

### Task 8 — reserved state restore and cross-binary compatibility

Implemented `state_load(path, resume=false, keep_overrides=false)` and five
internal bounded native transfer methods. MCP inventory is now **47 of 68**.
Reference path-state preflight/backup semantics come from the previously hashed
states.cpp at `1c9b9637476a83f7b80d3db1115a0f71529820d9`; no legacy file I/O or
TCP bridge ships in the new host.

Main pauses through a native upload reservation, reads a regular file (64 MiB
max), and transfers four 1800-byte chunks at a time within the existing 4 KiB
request/16 KiB writable-buffer bounds. Native mutation dispatch is reserved for
the upload owner. Other connections cannot pause/resume its reservation; UI or
own-connection pause/resume, reset, owner disconnect and human takeover revoke
it. Raw keyboard takeover also resumes ordinary play. Native upload leases are
30 seconds and main jobs have a 25-second deadline. The paused emulation loop now
wakes on request enqueue rather than adding a 10 ms polling delay per batch;
queue and revocation-capacity limits remain unchanged.

Native commit preflights signature, synchronization byte, version, current
configuration-dependent serializer size, and available layout/ROM metadata
before restore. Main additionally rejects a present stale/invalid hash sidecar;
legacy files without sidecars still receive native header/version/size checks.
Preflight failures keep guest registers/RAM and epoch unchanged and remain
paused. Success starts a new epoch at frame 0, revokes old captures/history and
releases input, or preserves only the requesting owner's neutral input leases.
The restored VI phase is conservatively not an observed boundary until a real
VI event occurs. An error after core mutation attempts backup restoration and
invalidates the epoch even after recovery; recovery failure stays paused and is
reported distinctly. That post-mutation recovery branch was not fault-injected.

Runtime compatibility gate passed using an isolated reference executable:
`/Users/bschlenk/Developer/ares-64/build_macos/desktop-ui/RelWithDebInfo/ares.app/Contents/MacOS/ares`
SHA-256 `b1ea7d41e2b4d2cea7c53e91076358b454f5b50ab7bb4848146ac1772390abac`.
The optional `test:state-reference` script confines reference app writes to a
disposable directory, overrides settings/saves and verifies the debug listening
port belongs to that child before connecting. It never borrows an existing user
session. A generated original N64 fixture passed both directions: Electron's
native state loaded in the reference app, and its newly saved state loaded in a
fresh Electron host with exact GPR/PC/HI/LO agreement. Both files were 8513150
bytes, `v153`, with the default 8 MiB configuration. A real 4 MiB configuration
rejected that state without changing CPU registers or epoch. This is not a claim
for every peripheral configuration or cross-platform compatibility.

Validation passed:
- Native rebuild and strict TypeScript build; **44/44** sequential regression tests
  (19.04 s final run). New coverage includes restore/register equality, new/stale
  epochs, pause/resume and neutral holds, checksum/version/size/ROM rejection,
  incomplete/invalid native uploads, read-only availability during reservation,
  foreign mutation/pause/resume rejection, disconnect and simulated keyboard
  takeover cleanup. Preflight failure tests prove guest registers remain unchanged.
- Real Electron MCP integration: exact **47-tool** registry and native N64
  save/peek/load round trip with new epoch and stopped UI, plus prior checks.
- Optional cross-binary test above, including the actual 4 MiB rejection case;
  measurements recorded in ignored `artifacts/state-reference.json`.
- Final package `artifacts/task-8-state-load-final/Ares.app` copied outside the
  checkout; loader/MCP denied checkout reads. 47-tool inventory, offline-state
  paths, non-N64 load/save rejection and prior PNG/WAV/AVI/memory checks pass;
  GB displayed 457 frames and shut down.

Task 8 remains in progress: multiwrite prevalidation, delayed verification and
bounded baseline/load/write/run/crop experiments are next. Physical keyboard/
gamepad and audible N64 acceptance still remain not run.


## Task 8 continuation — multiwrite and delayed verification (2026-10-20)

Added `write_memory_many` (48 registered tools of 68) and enabled nonzero
`verify_after_frames` on `write_memory_va`. Reference behavior was audited in
`ares-64` commit `1c9b9637476a83f7b80d3db1115a0f71529820d9`, telemetry-server.cpp's
`mem.write_many` / `mem.write_va` handler and tools/mcp/ares_mcp.py. The port uses
its own bounded native service and existing execution ownership, not the legacy
transport or a second scheduler.

- One safe point prevalidates every payload, full address range, total byte budget
  and run/deadline before any write. 1–32 writes / 1024 total bytes fit the existing
  4096-byte native request budget (reference: 256 writes / 8 MiB).
- Ordered CPU/physical writes retain cache hazards and JIT invalidation. Immediate
  readbacks follow all writes; later overlaps can invalidate earlier expectations.
  Earlier effects are never described as rolled back.
- Optional 1–100000 VI verification uses the existing owned operation. Final
  readbacks/matches are frozen inside its native completion snapshot, before
  ownership release. Stops, timeouts, cancellation/disconnect and retention use
  existing execution semantics. MCP retains the starting identity through polls
  and registers the handle for operation_status.
- Integration found a real paused-post-restore crash: state power defers Vulkan
  setup, leaving hidden RDRAM null until the next CPU run. Extracted System's
  existing deferred setup into prepareVideo(), shared by run and memory mutation.
  This prepares backing without CPU execution; unavailable backing fails clearly
  before writes. No serializer fields/order/version changed.

Validation:

- Native host and CPU fixture rebuilt successfully.
- Full strict-TS/service/native/MCP suite: **44/44**, 20.41 seconds. CPU fixture
  covers invalid later ranges/hex/deadlines, no earlier write on failure, overlap
  ordering, owned exclusion, final readback immutability, frame completion and
  timeout. Early-watch completion is exercised through the native operation
  fixture; this is not a claim of a running game's post-write watchpoint gate.
- Isolated real N64/MCP fixture loads an original generated state with initialized
  RDRAM mapping, then performs successful many writes, rejects an invalid later
  range without altering the first byte, runs delayed verification to a real
  inactive-VI timeout and proves later writes cannot alter operation_status.
  Deferred video preparation plus immediate mutation leaves all CPU registers
  unchanged. No user ROM, emulator instance, state or saves were used.
- Actual Electron MCP integration: **48-tool** registry and prior N64 state/GB
  workflow pass.
- Fresh standalone `artifacts/task-8-memory/Ares.app`: copied app package smoke
  passes, Electron 44.9.0 / Node 24.22.0, 455 Game Boy frames; native loader and
  MCP denied checkout reads. This package gate is not a N64 game acceptance gate.

Task 8 remains in progress: bounded batch ownership/orchestration and the full
baseline-load/write/run/crop sweep are still pending. Physical gamepad/audible
N64 acceptance remains separately unrun. Cross-binary state format compatibility
was verified in the previous slice; no serialization change was introduced here.


## Task 8 completion — native-reserved batch experiments (2026-10-20)

Implemented `batch` (49 registered tools of 68), selectively adapting semantics
from `ares-64` commit `1c9b9637476a83f7b80d3db1115a0f71529820d9`:
telemetry-server.cpp `ctrl.batch` validation, debug-session.cpp's batch step/finish
logic, and tools/mcp/ares_mcp.py's signature/docstring. No legacy worker, transport
or UI coupling was copied.

Main sequences the existing state loader, memory service, native frame operation
and screenshot publisher. A native reservation spans all steps, including idle
file-I/O gaps. It excludes competing mutations and expires independently of main.
A private child generation distinguishes internal steps from concurrent requests
on the same authenticated parent connection. Late child mutations remain rejected
after revocation. Parent pause/cancel forwards only to its own child; cancelling
an immutable old operation does not revoke a newer batch. Native status exposes
batch_id separately from the active child's operation_id.

State loads stay paused, clear overrides and update only the batch's internal
identity after a successful commit. External reset/ROM changes interrupt future
steps. UI pause/reset, UI resume in an idle gap, simulated human takeover,
deadline and disconnect revoke ownership. Runs return their actual immutable
native stop/progress. stop_on_error=false permits ordinary step failures but not
execution interruptions. Committed files/loads survive cancellation and remain
reported; side_effects_rolled_back is always false. Native child runs keep their
normal result retention; a batch reservation is not a retained parent operation.

Explicit port bounds: 1–32 steps; 1M total requested VI frames; 16 KiB aggregate
read/write bytes; 8 file-only screenshots; 20-second maximum/default; 48,000-byte
UTF-8 request parameters and 48 KiB result. The deadline keeps a synchronous batch
inside the existing 30-second local transport timeout. Reference bounds/defaults
are 256 steps, 16 MiB, 64 artifacts and 60 seconds. Syntax and aggregate limits
validate before reservation; file/configuration compatibility and memory ranges
validate at each step. Results distinguish attempted/successful steps. Result
space is reserved before another effect; oversized individual output is explicitly
summarized while retaining committed path/operation information. No nesting,
parallel execution, automatic retry or rollback.

Validation:

- Host and CPU fixture rebuilt. Full strict-TypeScript/service/native/MCP suite:
  **49/49**, 23.52 seconds. Subsequent affected batch cases rechecked after final
  cancellation hardening: **5/5**, 6.23 seconds.
- New original hand-assembled IPL3 fixture configures a 4x2 RGBA5551 VI and loops.
  Its disposable state fixture initializes serialized RDRAM identity mapping;
  no commercial ROM, user emulator, state or save is used. Real stdio batches
  reload the same baseline for red and green candidates, write backing RDRAM,
  advance **two actual VI boundaries**, crop/scale and decode PNG pixels, then
  verify memory. Both candidates pass with exact expected 4x4 RGB output.
- Incompatible-state preflight leaves the epoch unchanged. Offline baseline
  reads leave live CPU registers/epoch unchanged. Partial failure retains an
  earlier write, skips later mutations and reports the failing step; continue-
  on-error returns the following read. Deadline, UI pause, parent cancellation
  across an owned-load epoch, simulated human takeover and parent disconnect
  prevent subsequent writes. Native idle-gap exclusion, lease expiry and late
  child rejection pass. Old immutable-operation cancellation leaves a new run
  active. Commit-versus-disconnect and result-limit cases preserve actual effects.
- Actual Electron MCP integration: exact **49-tool** inventory and N64 batch read,
  state workflow, stopped UI, renderer reload and prior GB/capture checks pass.

- Fresh `artifacts/task-8-batch/Ares.app`: copied package smoke passes with
  checkout reads denied to both native loader and MCP adapter, exact 49-tool
  registry, explicit batch unavailability on GB, 455 GB frames and prior
  offline-state/memory/image/audio/video checks. Electron 44.9.0, Node 24.22.0.
  This is a package isolation gate, not packaged N64 game acceptance.

Task 8's bounded acceptance workflow now passes. All 68 tools remain preservation
targets; generated FastMCP/full result parity and physical keyboard/gamepad/audible
N64 acceptance remain separate gates. The default-configuration bidirectional state
compatibility gate passed in the earlier restore slice; this batch slice changes
no serializer bytes/version. Next implementation task: 9, RSP/RDP and advanced
CPU mutation.


## Task 9 first slice — register mutation and machine observations (2026-10-20)

Added cpu_set_registers, disassemble, rsp_registers, rdp_state and
peripheral_registers: **54 of 68** MCP tools now registered. This is the first
bounded task 9 slice, not completion of the RSP/RDP/call milestone.

Reference semantics audited in `ares-64` commit
`1c9b9637476a83f7b80d3db1115a0f71529820d9`: telemetry-server.cpp's cpu.write_regs,
cpu.disassemble and jRsp/jRdp/jPeripherals helpers plus tools/mcp/ares_mcp.py.
The native service has no legacy transport/UI dependency. The core-only change
explicitly instantiates the existing non-filling D-cache Word debug read for
external service linkage; serializers, instruction scheduling and cache algorithms
are unchanged.

- Atomic register mutation prevalidates all fields/aliases/values before any
  architectural write, requires pause/no owned operation, and honors GDB/upload/
  batch exclusion. GPR zero is read-only. Values are lossless through 64 bits;
  LLbit is 0/1 and Status32 uses setControlRegister. Duplicate aliases reject
  rather than silently picking a winner. Changed fields normalize ABI names.
- Aligned PC edits canonicalize 32-bit KSEG spellings, reset pipeline/next PC/
  pending delay slot and stale stop/suppression state, while main remains paused.
  Non-PC edits preserve immutable stops. All valid sets drop JIT translations;
  host executor reporting updates immediately after a PC edit can release a gate.
- Disassembly validates full installed-RDRAM ranges and aligned physical/KSEG
  aliases. CPU view observes D-cache without allocation/counter changes; physical
  view/KSEG1 bypass it. It does not claim instruction-cache fetch coherence.
  Local disassembler hints/colors are off. Direct/conditional targets are
  lossless, and indirect targets label current register context explicitly.
  Bound: 128 instructions (reference 1024), fitting the native reply budget.
- Direct RSP observations include scalar32/vector32x8, raw 16-bit accumulator
  portions, hardware status/signals, DMA and current instruction. DMA clock is a
  signed decimal string, origin PC stays full64 (reference helper truncates it).
  RSP instruction-debug metadata explicitly reports unavailable. RDP copies DPC
  registers/flags/source; VI/AI/PI/SI/RI groups use reference names. Reads do not
  invoke MMIO, consume a semaphore, promote DMA or execute guest instructions.

Validation:

- Both native host and real CPU fixture rebuilt successfully.
- Full strict TypeScript/service/native/MCP suite: **51/51**, 27.54 seconds.
  Final affected task 9 checks after executor reporting/max-response verification:
  **2/2**, 0.84 seconds.
- Native fixture exercises a real pending delay-slot stop, invalid later PC/
  Status with no earlier GPR write, readonly zero/duplicate rejection, pause/busy
  gates, full64 GPR/HI/LO, architectural Status/LLbit, PC pipeline reset, preserved
  old stop snapshots and invalidation of an actual compiled JIT section. Dirty
  cached code and KSEG1 disassembly differ as expected, with unchanged cache
  counters/CPU PC and correctly attributed branch/register targets. Direct
  RSP vector/accumulator/DMA and RDP/peripheral observations preserve state.
  Native fixture payloads independently pass the TypeScript DTO decoder; malformed
  lane widths/flags/groups/words/zero-register mutations reject.
- Disposable N64 and real MCP stdio verify all five tools, unchanged CPU/RSP
  state after observations, full 128-instruction reply, atomic failure, normalized
  fp alias/full64 values, immutable completed operation and current-stop context.
  An original 3-instruction fixture is explicitly installed in RSP DMEM (IPL1
  has not copied IPL3 after two CPU steps), then PC redirection executes addiu
  v0,zero,7. Active run/upload/batch mutation exclusions pass. No user ROM,
  emulator state or saves were used.
- Actual Electron MCP integration passes exact **54-tool** inventory, N64 GPR
  edit/restore, RSP/RDP/peripheral/disassembly queries, prior state/batch/GB and
  renderer/disconnect checks.
- Fresh `artifacts/task-9-machine/Ares.app` copied-package smoke passes with
  native loader/MCP checkout reads denied, **54-tool** registry and explicit
  unavailability of the five N64 tools on GB. Electron 44.9.0 / Node 24.22.0,
  455 GB frames and existing memory/offline-state/image/audio/video checks pass.
  This is package isolation evidence, not packaged N64 game acceptance.

Next task 9 dependencies remain unimplemented: RSP debugger cooperative progress
and DMA handling; RSP command/task and RDP submission hooks; exact RDP VI-interval
capture; bounded function calls/recovery. The reference hooks were audited:
RDP::render observes submissions before dispatching to any backend; SP_STATUS
halt-clear snapshots task memory before reuse. Future capture must preserve those
actual hook points rather than reconstructing reused buffers after a run. RSP's
legacy blocking pump must be reconciled with native safe-point ownership and
DMA progress. Stack/dump/probe parity rows also remain in the 14-tool remainder.
All 68 remain preservation targets. Physical input/audible N64, real-game and
fault-injection milestone gates are still separately pending.

## Task 9 RSP debugger slice (2026-10-20)

Adds rsp_debug/rsp_break/rsp_step/rsp_run/rsp_stop: **59 of 68** MCP tools.
Task 9 remains in progress; this does not implement command/task capture or calls.

The reference `ares-64` commit remains
`1c9b9637476a83f7b80d3db1115a0f71529820d9`. Audited source SHA-256:

| Source | SHA-256 |
| --- | --- |
| `ares/n64/rsp/rsp-commands.cpp` | `3abfb3ca71448d67948faf2c92201db20be784246ac1afa379d820eab6796476` |
| `ares/n64/rsp/dma.cpp` | `0a91027e34f502dd8486502bbd0e6b98701b86fc158111bbd3b8426810c8643e` |

The blocking legacy transport pump is replaced with a host-only cooperative gate.
Before-instruction stops unwind RSP/CPU main loops to the existing host safe point.
The ordinary owned operation coordinator grants steps or run; no second scheduler
or transport exists. RSP debug mode disables dual issue and selects CPU/RSP
interpreters without modifying configured JIT flags. IMEM debugger writes now
invalidate RSP translations. Hardware HALT is preserved. Debug DMA promotes a
stranded full slot and drains at most two 256-row slots synchronously; ordinary
DMA timing is unchanged when debugging is inactive. No serializer field/version
change was made.

A 30-second owner lease and five-second per-operation deadline bound frozen or
nonprogressing sessions. Breakpoints are before-instruction, aligned IMEM PCs,
16 entries maximum; starting PC suppression occurs once per grant. Step grants
are 1..1000000 instructions. Completed operations freeze separate RSP counts,
PC and reason; generic executed remains the CPU count. Parent/UI pause snapshots
before releasing the gate. Owner release/disconnect, pause/resume/reset, human
takeover, lease expiry and GDB attachment release debugging. Stop/cancel/timeout
retain a frozen session. Competing mutations, CPU operations, uploads and batches
return busy; owner memory edits are allowed only while frozen.

Validation on the actual new binary:

- Native host and real-core fixture builds passed; strict TypeScript passed.
- Full sequential suite **52/52** passed in 31.27s; after adding negative RSP
  contract checks and native lease/table cases, affected machine/RSP tests **4/4**
  passed. Real interpreter checks cover exact single instruction, branch delay,
  before-breakpoint/starting-PC skip, hardware BREAK, unchanged JIT flag, pending
  DMA promotion and both maximum-row DMA slots. Native lease expiry freezes the
  result before releasing; foreign release/table mutation and table bounds pass.
- Disposable original N64 CPU/RSP programs and actual MCP stdio verify all five
  tools, actual counts/PCs/reasons, exclusive owner, CPU-operation exclusion,
  running memory-write exclusion, hardware-HALT timeout with zero instructions,
  immutable completed results, pause and idle disconnect cleanup. No user ROM,
  active emulator or saves were used.
- Actual Electron integration passed exact **59-tool** registry, RSP operation
  acknowledgments, stop/poll and UI pause revocation, plus prior regression gates.
- Fresh `artifacts/task-9-rsp/Ares.app` copied outside checkout passed with loader
  and MCP checkout reads denied; **59-tool** registry, explicit five-tool RSP
  unavailability on GB, previous memory/state/image/audio/video checks, 456 GB
  frames, Electron 44.9.0 / Node 24.22.0. Packaged N64 game acceptance is not run.

Still pending: exact command/task/submission hooks and capture, stack/dump/probe,
bounded calls/recovery (nine unregistered tools). GDB-attachment/human physical
input acceptance and real-game/fault-injection milestone cases remain not run;
all 68 tools remain preservation targets.

## Task 9 exact RDP submission/task capture slice (2026-10-20)

Adds bounded exact `rdp_capture`: **60 of 68** MCP tools registered. Task 9 remains
in progress; legacy viewer command capture and calls are not implemented.

Audited `ares-64` source at `1c9b9637476a83f7b80d3db1115a0f71529820d9`:

| Source | SHA-256 |
| --- | --- |
| `ares/n64/rdp/debug-capture.cpp` | `d9142849081b1e6de06cea9512ba9f613e2753fd868e4c0fed71f4b75272ee77` |
| `ares/n64/rdp/rdp.hpp` | `f84fe9b6c05a4c8febebbf75f85f7b465e9eb8cd2d20073e6af06e0d2e7c2282` |

The host-only capture hook runs at RDP::render entry, before software/Vulkan
backend dispatch. SP_STATUS HALT-clear snapshots task header, XXH64 hashes of both
RSP memories and optional full DMEM before reuse. The native frame coordinate
selects exactly one interval; capture requires an observed boundary and uses the
existing owned one-frame operation/deadline/cancel paths. Early stops report
complete:false. Completion is frozen, independent of later VI progress.

Storage is explicitly bounded: 65536 full 64-bit words, 512 DPC batches and 64
tasks. Overflow has word/batch/task counters and truncation; after dropped tasks,
batches use task=-1 rather than misattribute an older task. Full storage skips
remaining word reads/iteration. XBUS range retains the reference 4 KiB clamp.
Pages hold words/source addresses/batch IDs, DPC/source/CPU/RSP/task provenance,
and task bytes/hashes. They are operation/epoch scoped and invalidated by reset,
state load, owner disconnect or a newer capture. No serializer/version change.

The live Vulkan test exposed a reference bug: casting RSP DMEM to the generic
RDRAM reader applies word swizzling to byte-ordered DMEM. Capture and software
RDP fetch now use the actual RSP reader, matching Vulkan's two 32-bit reads.
The software fixture checks full XBUS values and no RDP crash; real Vulkan
capture checks the same original byte pattern. This is an intentional correction
to the audited implementation, not a claim that reference bugs were preserved.

MCP keeps reference parameter names/defaults; max_commands clamps effective word
storage to 65536 and inline words to 1024. Optional main-owned atomic
`ares-rdp-capture-v1` JSON exports every retained word/batch/task. Export jobs cap
at two globally/one per owner, 64 MiB and 25 seconds; reset/disconnect revoke.
Failed publication returns file_error without replacing an existing artifact.
Legacy next_frame=false returns legacy_capture_unavailable; decoded command
grouping and full reference schema/result equality remain pending.

Validation:

- Native host/real-core fixture and strict TypeScript builds passed.
- Full sequential suite **56/56** passed in 33.58s. Core tests exercise real
  software RDP submissions, source-buffer reuse, actual HALT-clear task bytes,
  exact next-VI exclusion, large dropped ranges, batch/task bounds and unknown
  association after task overflow. The final explicit no-crash XBUS assertion
  was rebuilt and affected machine/core tests passed **2/2**.
- Disposable original CPU/RSP programs configure real VI and continuously submit
  DMEM words to Vulkan. Actual MCP captures exactly one interval, full bytes,
  batches and task DMEM; tests assert truncation, all retained JSON data, failed
  overwrite preservation, immutable completion after a later VI, stale-capture
  rejection and boundary preconditions. No user machine/ROM/saves were used.
- Main artifact tests stream multiple word pages plus batch/task sections and
  verify disconnect/cancellation/stale-page failures leave prior files intact.
- Actual Electron integration passed the exact **60-tool** registry, incomplete
  inactive-VI timeout and atomic capture JSON plus prior session/debug/state
  regressions. The fixture explicitly resets before capture to establish a
  boundary; a mid-frame attempt is correctly rejected.
- Fresh `artifacts/task-9-rdp/Ares.app` copied outside checkout passed with native
  loader/MCP checkout reads denied: **60-tool** registry, explicit RDP/RSP N64
  unavailability on GB, prior memory/state/image/audio/video checks, 454 GB
  frames, Electron 44.9.0 / Node 24.22.0. Packaged N64 game acceptance is not run.

Eight tools remain unregistered: rsp_capture, capture_control, render_probe,
stack, dump_rdram, dump_range, call_function and call_recover. The registered
rdp_capture also retains an explicit legacy-viewer gap. Physical input/audible
N64, game acceptance and broader fault-injection gates remain pending. All 68
remain preservation targets; task 10 has not started.

## Task 9 stack/probe and immutable memory dumps (2026-10-20)

Adds stack/render_probe/dump_rdram/dump_range: **64 of 68** MCP tools registered.
Task 9 remains in progress; viewer command capture and bounded calls are pending.

Audited `ares-64` at `1c9b9637476a83f7b80d3db1115a0f71529820d9`:

| Source | SHA-256 |
| --- | --- |
| `desktop-ui/program/telemetry-server.cpp` | `738abe2ed5dd789b7b9968bd0f77c45ab03ddcecdf81e9106e962d250799646b` |
| `ares/n64/cpu/debug-control.cpp` | `6952c5c0d7898ff6f0dfd6d73c98e67ff34c12940ba6c2d57285d8fa637e1597` |

Stack analysis is a native safe-point observation using the reference bounded
prologue/epilogue algorithm: eight frames, 256 backward instructions per frame,
256 raw stack bytes. Saved SW/SD RA, linked callsite checks, caller-SP direction,
cycles and unreadable code/data stop the heuristic. Output includes confidence,
RA source/width/slot, scan bounds, hint use, epilogue and explicit assumptions.
The optional function_start must be aligned readable RDRAM within 1024 bytes
before PC. The port uses directly validated RDRAM-backed KSEG CPU D-cache byte
reads with KSEG1 bypass, avoiding MMIO, cache allocation and guest execution.
This is no replacement for symbol/unwind metadata or proof of the stack.

Render probes use direct VI active/origin, DPC clock and native VI coordinate.
Baselines are per connection, capped at 16 and reset/released with owner/epoch,
so another client cannot consume a delta. First observation has null deltas;
rendering is the reference origin/DPC/frame-change heuristic, using actual
VI::active(). Legacy presented_frame/VPS counters are explicitly null.

Memory dumps copy all bytes at one native safe point into an immutable owner
pin, then use bounded main-process streaming and the shared atomic writer.
Full dumps cover installed 4/8 MiB RDRAM; ranges are reduced to 1..1 MiB and
support physical/KSEG addresses, CPU dirty-cache view and KSEG1 bypass. Bytes
are guest big-endian with no header. Complete range/expected-size prevalidation
precedes capture, renderer writes flush first, and pause state is preserved;
no guest instructions run. Active operations, upload, batch, RSP gate and GDB
conflicts return busy. One global native pin has a fixed 30-second lease.
Main has two global/one owner jobs, four 16 KiB pages in flight, a 25-second
deadline and 8 MiB output bound. Files include no snapshot header; result metadata
reports SHA-256, size, source/physical address, view and capture epoch/frame.
Reset/state load/disconnect revoke and success/failure releases the pin. Native
contains no path handling or file publication.

Validation on the actual new binary:

- Native host/real-core fixture and strict TypeScript builds passed. No core or
  serializer fields/version changed in this slice.
- Full sequential suite **59/59** passed in 37.65s. Core fixture checks a saved
  64-bit RA/caller callsite, unexecuted allocation, executed epilogue, bad hint,
  MMIO refusal, raw-window bounds and CPU/cache-counter nonmutation. Native dump
  tests cover expected-size prevalidation, dirty D-cache/KSEG1 distinction,
  immutable bytes after source reuse, owner/read bounds and lease expiry.
  Probe tests cover changed coordinates and per-owner baselines.
- Disposable initialized N64 fixture and actual MCP stdio dump all installed
  RDRAM, verify full file size/selected guest-order bytes/SHA-256, unaligned KSEG
  range bytes, failed-overwrite preservation, range/size failures, no CPU advance,
  stack output/hints and active-operation exclusion. The fixture uses its own
  serializer data to initialize chip mapping; no user ROM, emulator or saves.
- Main dump tests verify multipage byte order, cancellation/invalid-page cleanup,
  native pin release and preservation of an existing artifact. Shared contract
  tests validate actual native stack/dump/probe DTOs and reject malformed bounds.
- Actual Electron integration passed exact **64-tool** registry, stack/probe,
  real 16-byte range file, full-dump size precondition and prior RSP/RDP/state/
  input/image/audio/video/session regressions.
- Added an ordinary-play dump check after the suite: affected live test passed
  **1/1**, confirming snapshot.stopped=false and pause state remains running.
- Fresh `artifacts/task-9-inspection/Ares.app` copied outside checkout passed:
  **64-tool** registry, explicit stack/probe/dump unavailability on GB, prior
  memory/state/image/audio/video checks, native loader/MCP checkout reads denied,
  454 GB frames, Electron 44.9.0 / Node 24.22.0. Packaged N64 game acceptance
  remains not run.

Four tools remain unregistered: rsp_capture, capture_control, call_function and
call_recover. Exact rdp_capture's legacy viewer mode remains unavailable. Task 9
still needs these native hooks/recovery semantics and remaining physical/game/
fault-injection acceptance; all 68 preservation targets remain in scope.

## Task 9 bounded CPU calls and recovery (2026-10-20)

Adds call_function/call_recover: **66 of 68** MCP tools registered. Viewer command
capture/control still remains; task 9 is not complete.

Audited `ares-64` at `1c9b9637476a83f7b80d3db1115a0f71529820d9`:

| Source | SHA-256 |
| --- | --- |
| `desktop-ui/program/debug-session.cpp` | `93c4061ac352f8fc887e793e2d8f5317446c17dabc4e3c7027043dd2c877f700` |
| `ares/n64/cpu/debug-control.hpp` | `b98961f629793d13ef1ec1939a645901b5a4536dd33a1a86454342393b8e7baf` |
| `ares/n64/cpu/debug-control.cpp` | `6952c5c0d7898ff6f0dfd6d73c98e67ff34c12940ba6c2d57285d8fa637e1597` |
| `ares/n64/cpu/exceptions.cpp` | `36844f06d62e7f4eca0562e373fba5d15ed5c54e1002dd8530d37e5d1cfbc15e` |

Native calls use the existing operation coordinator and interpreter hook. The
private return address is the final installed RDRAM word, never patched; the
before-fetch gate validates JR/JALR rd=0, completed delay slot and original SP.
Direct jumps/wrong SP are invalid returns. Interrupt/exception/NMI request a stop
before the handler executes. No nested transport pump or alternate scheduler.
Core call state is host-only and reset at power; serialization/version unchanged.

Preconditions: paused, operation-free, aligned RDRAM entry/stack, no delay slot or
EXL/ERL context, up to four lossless integer arguments; t9=entry. Defaults remain
100000 instructions/60 VI/5 seconds. Bounds reduce reference 10M instructions to
1M and 3600 seconds to 600; VI bound remains 10000. Call ownership is native.
The MCP method polls the acknowledged native operation. Operation results include
frozen call completion PC/stop, return values, restoration/recovery flags and an
interrupted caller snapshot, and remain immutable across later recovery.

Success restores GPR/HI/LO/PC, Status first (including FR), then raw FPR bank and
FCR31, clears LLbit/stale stop/suppression and resets JIT translations. Memory,
devices, interrupt/time and other COP0 effects remain. Interruptions preserve the
actual callee state plus one caller context until explicit restore/discard;
a return-delay watchpoint takes precedence over success. Recovery requires a
paused operation-free same-epoch machine, connected owner/UI, or a new
authenticated connection after owner disconnect relinquishes its claim. New
calls return recovery_pending until restore/discard. Reset/state load invalidate
recovery. No silent CPU rollback occurs on timeout/cancel/pause/disconnect.

Validation on the actual new binary:

- Native host/real-core fixture and strict TS builds passed. Full sequential suite
  **61/61** passed in 46.02s. After tightening progress-phase DTO validation,
  affected native/real MCP call tests passed **2/2**.
- Real CPU fixture covers return exactly at instruction limit, executed delay
  slot/v1, all GPR and raw FPR restoration across a callee FR toggle, Status/HI/LO,
  LLbit clear, retained dirty memory/elapsed CPU clock, wrong-SP/direct sentinel
  jumps without executing the sentinel word, entry breakpoint, return-delay
  watchpoint, syscall/interrupt/NMI before handler fetch, instruction/frame/time/
  cancel/pause/disconnect exits, explicit recovery, pending-context exclusion,
  owner/reconnect recovery and retained Cause/EPC. Preconditions reject invalid
  entry/stack and delay-slot context before mutation.
- Disposable initialized N64 and real MCP stdio cover full-width integer args,
  private returns, retained memory, exact-limit return, register restoration,
  immutable results after recovery, syscall, wrong SP, discard, timeout, owner
  exclusion and disconnect transfer to a new authenticated connection. The fixture
  restores its own baseline between variants, preserving I-cache semantics. No
  user ROM, active emulator or saves were used.
- Actual Electron integration passed exact **66-tool** registry, call/recovery
  native preconditions and prior RSP/RDP/inspection/state/input/artifact gates.
  This does not replace N64 game/function acceptance.
- Fresh `artifacts/task-9-calls/Ares.app` copied outside checkout passed with
  native loader/MCP checkout reads denied, **66-tool** registry, explicit
  call/recovery unavailability on GB and prior artifact regressions. Electron
  44.9.0 / Node 24.22.0; packaged N64 game/function acceptance remains not run.

Only rsp_capture and capture_control remain unregistered; exact rdp_capture's
legacy viewer mode is still unavailable. Physical input/audio, real-game and
broader fault-injection acceptance gates remain pending. All 68 remain in scope.

## Task 9 bounded F3DEX2 and RDP command viewers (2026-10-20)

Adds rsp_capture/capture_control and enables legacy rdp_capture(next_frame=false).
**All 68 MCP tool names are now registered.** This is registration coverage, not
full preservation parity or completion of task 9. RSPQ/ELF/config/custom display-
list descriptors, semantic argument decoding and the documented manual/schema/
result gates remain required work; no scope removal has been accepted.

Audited `ares-64` at `1c9b9637476a83f7b80d3db1115a0f71529820d9`:

| Source | SHA-256 |
| --- | --- |
| `ares/n64/rsp/rsp-commands.cpp` | `3abfb3ca71448d67948faf2c92201db20be784246ac1afa379d820eab6796476` |
| `desktop-ui/ui/rsp-overlays/f3dex2.json` | `e55793c96632cdc2dd2797e8655e45a2bb4303e7095c739f3796daca5a30fc2e` |
| `ares/n64/vulkan/vulkan.cpp` | `1906b8079516c483ab3116cd5b48620e1fd21d2b34c4c5f0bd87736746451745` |
| `desktop-ui/program/program.cpp` | `e9af3109fea007b7d912f04a68a92c60734275f90ecb7170469308277eded40a` |

Native viewer state is disabled by default and never serialized. Validated
F3DEX2 signature/layout hooks copy command words from live RSP dispatcher
registers before handler reuse, then close command/dispatch-loop segments at the
next boundary or halt. Segment timing uses the reference 32-bit wrap-safe clock
delta; open-segment DMA rows contribute byte counts. Shared sequence numbers
reserve order at segment start. Interpreter and RSP traced-JIT contexts invoke
the actual hook without changing configured JIT flags. HALT-clear re-detects
microcode and hook points revalidate the signature, so unknown/audio code cannot
emit fabricated F3D commands. Names come from the audited F3DEX2 descriptor;
args remain explicitly raw words. RSPQ/config is reported unsupported.

Software and Vulkan command decoders record legacy RDP word0/word-count rows.
Vulkan records only complete buffered commands; exact submission/full-word
capture remains independent. Host afterRun bookkeeping commits on changed
nonzero VI origin, matching the reference's origin-swap concept, not an exact VI
interval or proof of a presented canvas image. Double buffers fix reuse of live
storage underneath the committed frame. Each channel retains 4096 rows with
explicit observed/drop counts, 64-row native pages and 1024-row MCP inline cap.
Generation/epoch checks reject changed committed frames during pagination.

capture_control has independent channel ownership and prevalidates both requested
changes atomically. Null queries are read-only; changes require idle execution
and no upload/batch/GDB conflict. Disconnect disables owned capture; reset/state
load clear buffers/ownership. No frontend/transport pumping in the core, no paths
or JSON parsing there. Existing source/post-run data is never reconstructed from
reused command buffers.

Validation on the actual new binary:

- Native host/real-core fixture and strict TypeScript builds passed. Full
  sequential suite **64/64** passed in 46.56s. The subsequent actual DMA-row
  attribution extension rebuilt the fixture and affected viewer tests passed
  **2/2** under both RSP interpreter and traced JIT.
- Core fixture executes an original F3DEX2-shaped dispatch/handler program under
  real JIT and interpreter, validates signature-derived PCs, live word capture
  before DMEM overwrite, nonzero duration, actual eight-byte SP DMA attribution,
  configured-JIT preservation, software RDP decoder rows, immutable origin-swap
  commits, stale generations, capacity/drop counts, clock wrap, unknown-microcode
  rejection, atomic owner changes and owner release.
- Disposable original CPU/RSP programs and real MCP stdio launch the RSP and
  submit two DMEM no-ops to Vulkan, then change VI origin. Assertions cover raw
  command words/names/timing before handler overwrite, Vulkan legacy word0 rows,
  origin-swap metadata, zero-limit reads, preserved committed records on disable,
  owner exclusion and disconnect cleanup. No user emulator/ROM/saves were used.
- Shared DTO tests decode actual native records, bound rows/cycles/words and
  reject malformed pages. MCP aggregation checks inline bounds and fails a
  generation change rather than mixing frames.
- Actual Electron integration passed the exact **68-tool** registry, viewer
  enable/query/disable, RSP snapshot and legacy RDP snapshot alongside the
  existing call/RSP-debug/exact-capture/state/artifact regressions.
- Fresh `artifacts/task-9-viewers/Ares.app` copied outside checkout passed:
  **68-tool** registry, explicit viewer unavailability on GB, prior memory/state/
  image/audio/video checks, loader/MCP checkout reads denied, 452 GB frames,
  Electron 44.9.0 / Node 24.22.0. This is package isolation evidence, not real-game
  F3DEX2/RSPQ acceptance or proof of full parity.

Next task 9 work is RSPQ ELF/config/custom descriptor and semantic capture parity,
then the remaining generated-schema/result/bridge/game/fault/physical acceptance
gates. All 68 rows retain explicit evidence/limits. Task 9 stays in progress and
task 10 has not started.

## Task 9 generated FastMCP schema comparison (2026-10-20)

Added a real generated reference snapshot from `ares-64` commit
`1c9b9637476a83f7b80d3db1115a0f71529820d9`, using FastMCP 1.30.0's
`list_tools()` in an isolated disposable Python environment. The snapshot records
source SHA-256 and MCP version. No reference emulator or user session is used.
The exporter suppresses bytecode writes into the reference checkout.

The comparison launches this port's actual built stdio server with an unused
metadata path and requests `tools/list`; it does not connect to native services.
All 68 reference names exist, with no extra names. The signature projection
(parameter names, required fields, primitive types, nullability and defaults)
matches **67/68** tools. `batch.max_seconds` defaults to 20 here versus 60 in the
reference, consistent with the current 20-second native reservation limit. This
remains a recorded compatibility limit, not an accepted parity exception.

The full report retains bounds, nested schemas, unknown-field policy and union
representations. All 68 schemas differ: Electron rejects unknown fields, and
many tools impose bounded resources or more specific validation. Documentation
and irrelevant object/enum/required/union ordering are ignored; tuple order and
property names such as `title`/`description` remain significant. The checked-in
report makes these differences reviewable and guards against silent drift. It
does not establish result-shape or behavioral parity. Regeneration instructions
are in `tests/reference/README.md`.

The two new tests exercise the actual stdio inventory against the recorded
comparison and verify that required/default/type/nullability/bounds/unknown-field
changes remain visible. Reference inspection also confirmed that its bundled
F3DEX2 descriptor has command names but no argument fields: the general semantic
formatter is primarily used by RSPQ overlay descriptors. RSPQ ELF/config/runtime
overlay/custom descriptor capture and semantic decoding remain pending, together
with migrated bridge/paper-cut/game/result-shape/manual acceptance. Task 9 is still
in progress; no task 10 work or scope removal is implied.

Validation: strict TypeScript builds and the complete sequential suite passed
**66/66** tests in **45.90 seconds**, including the two new schema checks and all
existing native/live viewer, CPU/RSP/RDP, state, transport, audio and recording
fixtures. `git diff --check` passed. No production/native changes in this slice;
the existing Electron/copied-package evidence is not claimed as a new run.

## Task 9 remaining implementation and final acceptance (2026-10-20)

Closes the remaining RSPQ/config/semantic capture backend and adapter gaps after
`ddabc6d67`. Source is selectively adapted from `ares-64`
`1c9b9637476a83f7b80d3db1115a0f71529820d9`; the legacy Python/TCP server and
blocking debug pumps are not shipped. No tool was removed.

RSPQ capture now discovers bounded companion ELF files using the reference ROM,
ROM-base/build and parent/build candidates. The queue symbol must be file-backed;
all five hook signatures are validated against actual live IMEM, including on
subsequent hook entries. F3DEX2/custom display-list signatures support 1–4 words,
signed loop/dispatch offsets, register layout and command descriptors. The RSP
interpreter and traced JIT preserve configured flags. RSPQ command/loop/overlay-
load/fetch segments share sequence/timing/DMA attribution with the existing viewer.
Same-type transitions coalesce. Commands copy at dispatch from live DMEM using
the actual dispatcher size, command-buffer bounds and a 16-word cap; missing or
truncated words remain explicit. Unknown/replaced microcode is not fabricated.

A bounded embedded libdragon descriptor ships in the binary, with an optional
JSON override beside the ELF; a display-list descriptor can live beside the ELF
or ROM. Runtime ucode names come from ELF data ranges and the live table/idmap,
including multi-ID command offsets and stale-name removal after unregister.
DWARF shared-DMEM labels and rsp_ucode_t member offsets are optional, with safe
fallbacks for stripped metadata. ELF section/symbol/string bounds, DWARF depth/
work/string bounds, descriptor recursion/counts/transforms, missing words and
native reply byte limits are checked. Names and semantic arguments are frozen
at dispatch, so runtime re-registration cannot relabel an immutable old frame.
Formatting supports bit ranges, sign extension through 32 bits, integer and
floating transforms, enums, decimal/hex, DMEM labels and nested RDP descriptions.
Legacy RDP rows now include the reference semantic command descriptions.

CPU adapter results now preserve the reference compact/verbose register groups:
`gpr_named`, verbose GPR arrays, named COP0/Status/Cause and classified architectural
FR=0/1 FPR values (JSON-safe NaN/infinity/zero/subnormal classes). The native
scheduler retains immutable completion registers for later FPR/COP0 formatting;
`run_frames` and `operation_status` no longer reject those N64 flags. Native
safe-point status supplies PC/debug/input/save/recovery/operation metadata and
native presentation-rate observations. Render probes expose the actual origin-
swap counter and native video-callback VPS, independent of canvas credits.

Batch restores the reference 60-second default (bounded at 600 seconds); local
transport includes its bounded deadline plus cleanup margin. Generated FastMCP
comparison now verifies **68/68 names and parameter signatures**, including
required fields, primitive types, nullability and defaults. Strict unknown-field
policy, resource limits and structured validation differences are intentional
adapter changes recorded in the checked-in report. Result contracts use native
protocol v1 scoped identity/handles, bounded pages and explicit errors/truncation;
no legacy telemetry wire/result-byte equality is advertised. Separate stack and
disassembly tools provide diagnostics rather than mutating an old hit snapshot.

MCP cancellation is scoped per concurrent tool request. A cancellation aborts
only an operation started by that call, using its original ID/session/epoch;
read-only status cancellation cannot cancel the polled handle. Batch abort uses
a private token checked against the owning job, so stale/foreign cancellation
cannot pause a later operation. Migrated bridge checks cover mismatched IDs,
wrong sessions, fragmentation/coalescing, limits, error propagation, disconnect,
no replay, stdio cancellation and batch cleanup without following mutations.

Original generated ELF/DWARF/microcode fixtures validate RSPQ through actual JIT
and interpreter dispatch, command reuse, semantic fields, overlay remapping,
multi-ID offsets, signature replacement, command-buffer truncation, immutable
metadata and byte-bounded pages. Live native/MCP tests cover F3DEX2, a custom
three-word display-list descriptor and ELF/config-backed RSPQ; no user binary
serves as the instruction fixture. Existing calls tests cover Status/FPR restore,
private JR/JALR return/delay/SP rules, exceptions/interrupt/NMI, limits, disconnect,
cancel, recovery/discard and immutable completed results.

Final native/TypeScript validation passed **77/77** sequential tests in **83.71 s**
on the final native build, including all new bounds/config/metadata/cancellation
checks and existing CPU/RSP/RDP/cache/state/image/audio/recording/ownership tests.
The actual stdio coverage recorder reports **68/68 successful tool paths**, no
missing names; `tests/reference/task-9-tool-coverage.json` links each name to its
successful runtime test(s). Actual sandboxed Electron/MCP integration also passed
on its bundled runtime, including GB/N64, shared UI/service behavior, reload,
68-tool registry and expected disconnect/stale-instance failures.

The migrated isolated real-game runner passed **17/17** checks against the final
new host: copied EEPROM metadata, real-game boot/render, nonadvancing compact/
verbose groups, atomic invalid mutation, state/run/dump determinism, offline
nonmutation, disassembly/heuristic stack, legacy/exact RDP capture, bad-state pause,
wall lease expiry and bounded original function call/interruption/recovery.
Artifacts are under `artifacts/task-9-game-acceptance/`; `game.png` was visually
inspected (rendered Kirby title/intro scene, not claimed as an active-gameplay
manual test). Original ROM and EEPROM SHA-256 checks remained unchanged. The
runner verified its report's host hash matches the final native binary:
`e33dfc20a5ccaa0bf95555508796b7831bc3cc4178ba8092a57012d22203a954`.

These are migrated assertion families on the new binary, not claims that the
unchanged legacy TCP/Python tests ran against a compatible telemetry server.
Physical keyboard/gamepad takeover and audible output remain the separate
previous task 6/7 manual gates; they were not simulated and called physical tests.
Task 8's serializer rollback-failure branch remains a separately recorded
fault-injection case. No Windows/Linux, signing/notarization or universal
microcode decoding claim is added.

Fresh `artifacts/task-9-complete/Ares.app` built successfully and passed
`test:package` after copying outside the checkout: **PACKAGE_MCP_PASS**, all 68
tools, explicit N64-only unavailability on GB, existing memory/state/image/audio/
video checks, **455 GB frames**, Electron **44.9.0**, bundled Node **24.22.0**.
The package test denied checkout reads to both the native loader and MCP adapter;
embedded RSPQ data and bundled runtime dependencies require no checkout resource.

**Task 9 complete.** All planned native hooks/backends and preserved tool entry
points have runtime evidence, generated signatures/defaults match, migrated
bridge/transport/stdio/CPU/paper-cut/recording assertions pass, real-game checks
pass on the final binary, and actual Electron/copied-package gates pass. Intentional
bounded adapter contracts are documented together with tests; no tool was removed
or silently replaced by reading a reused command buffer. Separate earlier manual
checks above remain honest follow-ups, not claimed passes. Implementation commit:
`fa7be7742`. Task 10 (plugin loader/panel/game matching) is the next roadmap task.

## Task 10 — local plugin loader, panels and verified game profiles

Implemented API v1 manifests and a main-owned registry for bundled and explicitly
added/enabled local directories. Validation rejects unknown APIs/capabilities,
invalid built paths, duplicate contribution/plugin IDs and escaping symlinks.
Bundled IDs cannot be shadowed. Overlay/page/shell modes are reserved and report
that only panels/commands are implemented. Configuration persists in userData.

The sandbox preload exposes serializable plugin operations. Scoped resource URLs,
SDK calls, storage and logs are tied to a random plugin generation and native
session/epoch. Main checks both before and after async requests; disable, reset,
ROM replacement and renderer reload revoke tokens. Disposal releases only that
plugin's execution owner. Renderer scopes own panels, commands, styles, listeners,
intervals and async activation cleanup. Commands work without mounting a panel;
panels mount lazily in a left/right/bottom dock and expose visibility callbacks.
Failures appear in the Plugins manager, alongside bounded logs. JSON storage is
namespaced/versioned, limited to 16 KiB and atomically published.

Plugins are explicitly trusted renderer JavaScript, not an untrusted-code sandbox.
The capability SDK routes supported intent but cannot isolate raw same-renderer JS.
Watched immutable hot replacement and failed-replacement preservation remain task
11; this task's Refresh action disposes and reloads all discovered scopes.

Native session status now reports a cached SHA-256/size for normalized MIA
`program.rom`, not filenames or N64 word-swizzled backing memory. Profiles require
an exact system/content match. Bundled examples provide generic memory read/write
and the verified Kirby US revision 0 identity panel (gameplay tooling remains task
12). They default disabled. A standalone esbuild TS/TSX template bundles a tiny DOM
JSX factory; no custom UI framework or React dependency is imposed.

Validation:

- Native host build and all three strict TypeScript targets pass. Host SHA-256:
  `3753d98ce6519a040ca3159dcf6e22b7ec1402edfdd8e16599cdb56149c13636`.
- Manifest/registry tests cover exact hash matching, validation, capabilities,
  pending-call revocation, resource containment, storage limits/persistence,
  duplicate IDs and actionable missing-build errors.
- Real isolated paused N64/GBA fixtures enumerate and write/read/restore memory
  through the same scoped plugin service path; stale epoch calls reject.
- An isolated real Kirby test loads renamed big-endian and byte-swapped copies;
  both normalize to the verified hash and activate the profile. Wrong content
  revokes it. The original ROM hash remains
  `2f579751d7ad2824dfd8a6141570306bfaeda1cff40139ba231c30b8591d681c`;
  all saves/settings are disposable. No active emulator or original save is used.
- Real Electron plugin test: command before panel mount, memory reads, disable
  removes UI/rejects old token, reset revokes scope, compiled TSX panel renders,
  activation failure is actionable, delayed activation returns its cleanup after
  disable. Existing Electron session/inspector integration also passes.
- Real 68-tool MCP integration passes with the status addition.
- Fresh `artifacts/task-10-complete/Ares.app` includes bundled plugin resources and
  passes copied-out **PACKAGE_MCP_PASS**: 453 GB frames, Electron 44.9.0, Node
  24.22.0, checkout reads denied to native loader and MCP adapter.

Previous physical keyboard/gamepad takeover, audible-output acceptance and task
8 rollback-failure fault injection remain recorded follow-ups. The user accepted
manual verification later; this work does not claim they passed.

Final sequential suite with the optional real-game test explicitly enabled:
**81/81 passed**, no skips, **56.00 seconds**. Task 10 is complete; task 11 is
the next roadmap task.

## Task 11 — watched immutable replacement and cleanup

Implemented source watchers (180 ms debounce), host-side JS/TS/TSX bundling and
content-addressed immutable artifacts. Hidden trees, dependencies and declared
output directories are ignored. Snapshot files are bounded to 2 MiB each, 16 MiB /
256 files total; syntax/style/build errors retain the current artifact. Compiler
scripts from plugins are never run. esbuild is now a production dependency so
copied applications build source plugins without checkout/npm tooling.

Replacement validates the manifest, imports a fresh candidate module and optionally
runs pure `prepare()` while the working UI remains live. Main confirms release of
only the previous plugin's native owner before replacement proceeds. Old SDK
scopes abort immediately, invoke cleanup in reverse order and wait at most one
second for asynchronous disposal. A failed release blocks replacement. Candidate
registrations remain detached; listeners and timers arm only after activation and
panel restoration succeed. Activation/import/preparation have two-second bounds.
Main and renderer reject mutations before publication (`plugin_not_ready`); patches
must be explicit commands. Neither rollback nor restoring a panel replays writes.

Optional `stateVersion` / `saveState()` preserve compatible transient JSON (16 KiB),
with stable panel IDs restoring visibility/selection and existing dock placement.
Failed activation disposes the candidate and remounts the previous immutable
artifact under a fresh token. Failed rollback is reported explicitly; another valid
edit recovers. Build/import/prepare failures retain the actual old DOM/token. Rapid
edits invalidate obsolete candidates, watcher work coalesces, and monotonic catalog
revisions prevent late IPC snapshots from reviving a superseded generation.

Native session/epoch changes revoke candidates and active SDK access. Source edits
never restart/reset the emulator. Renderer interval callbacks suspend when their
plugin has no visible panel. The developer manager exposes bounded scope, listener,
timer and request counts plus per-plugin Reload and diagnostics. Disabled-source
edits are rebuilt when enabled. Artifact pruning retains current/previous/candidate
snapshots and protects in-flight captures; watchers close at application shutdown.
Chromium keeps imported ESM modules cached until renderer teardown: the acceptance
checks prove SDK effect/request cleanup, not individual-module unloading or a
universal flat-heap guarantee. Plugins remain trusted same-renderer JavaScript;
raw import-time effects outside the SDK remain the author's responsibility.

Validation:

- Strict main/browser/preload TypeScript compilation passes.
- Immutable artifact tests cover captured TSX/imports, custom output exclusion,
  content hashes, preserved previous bytes, failed builds and temporary cleanup.
  Registry tests cover staging-token denial, nonreplayed writes, rejected-candidate
  preservation, failed native release, disabled-source rebuild and existing task
  10 validation/storage/capability behavior.
- Full sequential suite passed **83/83**, no skips, including isolated real Kirby,
  N64 and GBA checks. The final repeat passed again in **99.36 seconds** (first
  run: 49.84 seconds); final targeted artifact/registry checks also pass.
- Real Electron reload test passes: invalid edit retains DOM/token; watched valid
  edit restores value 37; prepare failure keeps UI; failed activation remounts the
  previous artifact/state; activation writes reject without changing memory; stale
  delayed writes reject; **12** repeated reloads retain **1 scope / 2 listeners /
  1 timer** and a probe fires once; retained artifacts remain bounded; hidden
  polling stops; rapid edits discard an obsolete preparing candidate; hanging
  cleanup reports a bounded timeout and recovers; incompatible state versions
  start fresh; failed rollback is visible and a valid edit recovers; session switch
  during preparation rejects the stale candidate; disabling removes all effects.
- An operation started through **real MCP stdio** remains running across plugin
  replacement; its MCP owner cancels it explicitly. A plugin-owned run is released
  with `disconnected` on reload. Only an explicit patch command changes fixture
  bytes, and they are restored. Native session/epoch remain unchanged across JS
  edits; the session changes only in the explicit ROM-switch test.
- Existing task 10 Electron plugin/TSX/delayed-activation test passes.
- Fresh `artifacts/task-11-complete/Ares.app` copied outside the checkout passes
  **PACKAGE_MCP_PASS**, **574 GB frames**, Electron 44.9.0 / Node 24.22.0. The copied
  app's bundled compiler builds and watches an external TSX plugin from version
  1 to 2 with the same native session/epoch. Native loader and MCP adapter are
  denied checkout reads. Final package validation is recorded below.

Earlier physical keyboard/gamepad takeover, audible-output acceptance and task 8
serializer rollback-failure fault injection remain recorded later verification;
this task does not claim those checks passed.

Final validation: **83/83** sequential regression tests passed (no skips); strict
build plus final targeted artifact/registry checks passed, including invalid CSS
and an escaping manifest symlink. The final live reload test additionally injects
an old IPC catalog snapshot and confirms it cannot replace the current scope/DOM.
Existing Electron session/inspector and real **68-tool** MCP integration passed.
The final refreshed copied app passes **PACKAGE_MCP_PASS** with **504 GB frames**
and packaged TSX 1→2 hot replacement retaining native session/epoch. No checkout
compiler or resource is required. Original ROMs/saves were not modified.

**Task 11 complete.** Implementation, failure recovery and cleanup acceptance are
recorded above. Task 12 (actual agent-authored game tool and agent-facing plugin
commands) is next. Separate earlier manual/fault-injection checks remain pending.

## Task 12 — bundled tools, agent commands and Kirby power selector

Moved the temporary renderer-owned inspector into `ares.examples.memory` using
only the scoped plugin SDK. Region selection, offset/length validation, hex reads
and whitespace-tolerant writes remain available across the existing N64/GBA
backends; compatible UI state survives reload. Removed `services/inspector.ts` and
its separate page controls/script. `ares.tools.debugger` displays the native
immutable last stop and captured registers; live registers are read only when no
stop exists. A new read-only `debug.read` capability routes those two methods.

The user chose a power selector instead of the roadmap's possible animation tool.
`ares.examples.kirby` now offers None, seven singles and 28 combinations (IDs 0–35)
with the seven existing `gameplay/elements` PNGs copied unchanged from the Kirby
repository. Mappings/addresses come from `docs/kirby64-ability-ids.md` and
`docs/ares-power-cheat-and-tracing.md`; source/provenance and limits are recorded in
`bundled-plugins/kirby/README.md`. Activation still requires the verified US rev 0
normalized content hash, independent of filename.

Each explicit selection sends one prevalidated CPU/cache-aware big-endian batch:
`0x8012E850` = power ID, `0x800D6E90` = HUD ID, and `0x800D6EA8` = a different valid
ID to trigger the documented HUD redraw comparison. Immediate readbacks verify
all words. It requests no frames, resume/pause or controller overrides. Selection
is never replayed on activate/open/reload; other owners' operations return busy.
The UI explains use while Kirby is controllable and the known hat/body-palette
limitation. The user played a separate interactive instance with copied ROM/EEPROM
and confirmed that the selector works perfectly.

Added `dist/plugins/cli.js` and packaged `ares-plugin`: scaffold/build/list/add/
enable/open/reload/command, with JSON output and explicit connection/instance
selection. Scaffolding supplies the TSX SDK example; builds use bundled esbuild;
reload waits for the replacement generation. Agent open/command actions invoke
registered handlers directly and acknowledge through main-frame IPC. Main and
renderer validate the exact generation token, so stale requests never bind to a
new game or replacement. Commands are not retried after uncertain completion.
The authenticated private local endpoint retains shared replay/in-flight bounds;
plugin catalog responses omit log bodies and use bounded pages. The CLI reduces
oversized pages. The preserved MCP registry remains **68 tools**.

Fresh profiles enable the bundled tools; existing preferences remain respected.
Tests may suppress defaults with `ARES_DEFAULT_PLUGINS=0`; `ARES_BACKGROUND_TEST=1`
hides test windows without throttling rendering. No local plugin Node/filesystem
privilege or untrusted-code sandbox claim is added.

Validation:

- Strict main/browser/preload builds pass. The sequential regression suite passed
  **84/84**, no skips, in **58.66 seconds**, including isolated Kirby identity,
  N64/GBA memory behavior, native ownership and existing schemas. An additional
  real CLI/private-endpoint paging regression passes after the final paging fix.
- The original-API power test checks verified IDs, icons, CPU-view word batches,
  redraw invalidation, invalid IDs, wrong revision denial and busy propagation.
- Real isolated Kirby Electron test: all **36** buttons and icon loads pass; every
  power/HUD write is read back through the CPU view, detector words differ, and
  frame/session/epoch/paused state remain unchanged during selections. Reload
  does not replay a write; stale command tokens reject; explicit command power-14
  uses the same selector. Original RAM words are restored and ROM hash unchanged.
  Screenshot: `artifacts/kirby-powers.png`. The automated run boots normally to
  initialize RDRAM and verifies memory/panel behavior, not 36 visual attacks.
- Existing Kirby repository states are v152 and are rejected by the v153 loader;
  they were not rewritten. No serializer conversion was added. Optional compatible
  state support remains in the test harness. The original ROM, EEPROM and states
  were not modified by testing; interactive play uses separate copied save data.
- Real task-12 milestone test passes: MCP triggers a watchpoint in an original
  instruction fixture; its native stop equals the panel's full immutable stop;
  CLI scaffold/build/add/enable/open/reload edits TSX while the same game/session/
  epoch and watch snapshot remain loaded. Conflicting writes/runs return busy;
  human pause cancels the external run; stale agent commands reject.
- Existing Electron session/inspector integration passed with the migrated panel.
  Plugin UI/TSX/delayed cleanup, repeated reload/failure recovery and full 68-tool
  Electron/MCP integration also pass. Final GUI checks run in hidden windows from
  an isolated build so the user's play files, focus and watched sources are not
  rebuilt/reloaded underneath the play session.
- Fresh `artifacts/task-12-complete/Ares.app` was built from that isolated snapshot
  and copied outside both checkouts. **PACKAGE_MCP_PASS**: **872 GB frames**,
  Electron **44.9.0**, bundled Node **24.22.0**, same-session TSX 1→2 hot reload.
  Bundled CLI scaffold/build/list/add/enable/open/reload/command all pass with no
  developer Node/npm. Both checkout roots are denied to native loader, MCP adapter
  and plugin CLI. Bundled Kirby icons and templates are present. Result record:
  `artifacts/task-12-complete/package-mcp.json`.

**Task 12 complete.** The source, SDK examples, CLI usage, provenance and runtime
acceptance are documented. Task 13 (settings service/bundled settings plugins) is
next. Earlier physical takeover, audible-output and task 8 rollback-failure checks
remain explicitly pending; selector success does not claim those separate passes.


## 2026-10-20 — promotion audit and fresh-agent handoff

At the user's request, pause task 13 and treat the Electron architecture as
validated. Review temporary choices before further feature development.

Added `RESUME.md` with the current objective, document read order, task 12 baseline,
build/test reproduction, active-play isolation constraints, optional/private inputs
and remaining manual/fault-injection gates. Corrected the roadmap's stale opening
(which still said task 10 was next), made the current pause explicit, and fixed
the service README's reference to deleted inspector.ts.

Added `TECHNICAL-DEBT.md`: 30 findings across layout/identity, build isolation,
desktop coupling, persistence/profile ownership, settings, host lifecycle, plugin
recovery/API/builds, test reproducibility/CI/performance, packaging/platforms and
normal-play UX. Separately recorded intentional bounded contracts and deferred
features. Proposed `electron-ui/` alongside `desktop-ui/`, with identity/data and
active-play-safe build decisions before relocation. Proposed batches are awaiting
user triage; no runtime migration or task 13 implementation was performed.

Validation: source/documentation review and `git diff --check`. No runtime builds,
GUI launches or emulator mutations; the user's play instance was not used as a
fixture. Next: discuss triage with the user, then implement the agreed scope.


### 2026-10-21 — user clarification on promotion scope

The user explicitly waived preserving existing development data. Choose permanent
identity/locations without a migration framework or compatibility work for that
data. Updated the handoff/audit accordingly. Desktop UI is intended to be
superseded, but verified that the Electron target still compiles desktop sources
and uses their loader/registry, settings, input mappings, resources and program
helpers. Those are live dependencies, not merely a feature reference. No runtime
changes or deletion performed; extraction/retirement scope remains for triage.


### 2026-10-21 — promotion triage approved

The user approved the full recommended sequence: permanent electron-ui home and
identity; development/play build and profile isolation; desktop dependency
extraction/retirement before settings; repeatable acceptance/CI and plugin recovery;
then task 13. Existing development data needs no migration. Deferred release,
platform, broader API/peripheral and speculative optimization work stays deferred.
RESUME.md is the single fresh-agent entry point and authorizes implementation of
this sequence without re-requesting triage approval. Documentation-only change;
checked with git diff --check, no runtime/session changes.


## 2026-10-21 — permanent Electron frontend home (foundation slice 1)

Moved the frontend, native host, services, plugins, MCP adapter and tests from
`tools/electron-spike/` to `electron-ui/`. Root README links the new handoff.
CMake now uses `ARES_BUILD_ELECTRON_UI`; no old-option alias or data migration.
Adjusted native/test/package resource paths for the shallower directory.

Source package and copied app share `ares-workbench` / Ares Workbench identity,
`org.ares.workbench` bundle ID and package version 0.1.0. The packager reads the
source manifest for app and macOS bundle versions. Renderer storage uses the new
product key. The user waived protection of the previous Kirby play session.

Validation on macOS arm64:
- Three strict TypeScript targets pass.
- CMake configure with the new option and both native targets build successfully.
- Sequential regression suite: 84 passed, zero failures, one optional private-ROM
  identity test skipped (no verified Kirby input supplied for this run).
- Hidden actual Electron session integration passes stalled-renderer controls,
  reload identity, pause/resume and memory read/write/restore.
- Fresh `/tmp/ares-promotion-package/Ares.app` passes copied-package acceptance:
  868 GB frames, all 68 MCP tools, bundled CLI scaffold/build/list/add/enable/open/
  reload/command and same-session TSX replacement. Native loader, MCP and plugin
  CLI run with checkout access denied. Result: ignored `artifacts/package-mcp.json`.
- `git diff --check` passes. Sandbox denied local sockets on the initial suite;
  rerun with local-socket permission passed.

This completes the relocation/identity slice, not the entire foundation batch.
Next: explicit profile/data roots, per-profile writer ownership and independent
launch/build outputs; then desktop dependency extraction and acceptance/recovery.
Task 13 remains on hold. Existing physical-input/audio and fault gates remain open.


## 2026-10-21 — profile ownership and independent development runtimes (foundation slice 2)

Electron now chooses an explicit profile root: named profiles under the OS app-data
`Ares Workbench/profiles/` directory, or `ARES_PROFILE_DIR`. Existing explicit
Electron `--user-data-dir` / embedded `app.setPath` selections remain supported.
The canonical root owns saves, plugins, browser storage/cache and logs. An
OS-backed Electron single-instance lock refuses duplicate writers before plugin
initialization or native loading; no arguments/mutations are forwarded. Save
overrides must remain inside the owned profile, including after symlink resolution.
No old-data migration was added. Raw native test hosts retain their save-path API.

`npm start` builds a fresh ignored runtime snapshot, with copied bundled plugins,
native executable and databases. It defaults to the separate `development`
profile; copied apps default to `default`. Checkout JS builds/tests/package builds
and later native relinks no longer replace a development app's active files.
Snapshots clean up on shutdown. Dependency installation still requires stopping
development apps because node_modules is shared. Explicit author plugin paths
remain watched. Abnormally killed launchers may leave disposable snapshots.

Validation (macOS arm64):
- Three strict TS targets and sequential regressions: **85 passed, zero failures,
  one optional verified-Kirby-ROM test skipped**.
- New real Electron ownership acceptance passes duplicate/symlink-alias refusal,
  concurrent independent profiles and forced-crash recovery.
- New launch acceptance copies a distinct native executable, rebuilds checkout
  dist while the app runs, verifies unchanged snapshot output and MCP session/epoch,
  then confirms frames and automatic snapshot cleanup.
- Existing Electron session integration and plugin hot-reload/rollback acceptance
  pass with the new profile/browser/save layout.
- Fresh copied `/tmp/ares-profile-package/Ares.app`: **PACKAGE_MCP_PASS**, 875 GB
  frames, 68 MCP tools, CLI scaffold/build/list/add/enable/open/reload/command and
  same-session packaged TSX replacement with checkout access denied.
- Profile path/override/symlink tests and `git diff --check` pass.

The initial launch acceptance used an overlong macOS temporary-directory socket
path; switching that fixture to a short `/tmp` endpoint passed. This does not
change the existing transport path bound. The initial ownership helper waited for
startup during module evaluation; moving that wait after app readiness fixed the
helper. These failed checks were not recorded as runtime passes.

Next: extract loader/firmware/save/config/input/resource code from desktop-ui,
inventory remaining features and retire its frontend/Hiro coupling. Then add the
single acceptance entry point, CI and plugin startup recovery before task 13.
Physical input, audible output and serializer fault-injection gates remain open.


## 2026-10-21 — frontend-native loader extraction and desktop retirement (foundation slice 3)

Extracted the existing compiled-system registry/loaders, firmware, save/media
routing, peripheral setup, input mappings and BML configuration model into
`electron-ui/native/runtime/`. Replaced the inactive desktop Program global with
an explicit input mutex/capture context and companion-media queue; palette
application remains. Removed unused Hiro menus/dialogs, hotkeys and disc-tray
callbacks/timers, preserving actual host/core/service behavior.

`DESKTOP-RETIREMENT.md` inventories each desktop subsystem, its replacement or
explicit deferral, provenance (`b9f42e052`) and remaining gates. After extraction
and acceptance, removed the superseded desktop-ui tree. Git retains the reference.
The root CMake frontend now defaults to the native Electron host. Normal builds
have no desktop or Hiro target; optional standalone MIA/genius utilities can still
use the retained Hiro toolkit. Retired desktop-specific build/archive/sign/release
workflows, wrappers and the obsolete universal-app script; updated root/build docs
and format targets. The Electron macOS CI lane remains the next foundation slice.
No cores, Ruby backends or working native service implementations were removed.

MIA gains a narrow optional resource resolver; standalone MIA keeps its previous
lookup without that callback. Electron passes the selected runtime resource root
explicitly to probes and host starts. Snapshot launches copy its databases. The
host has no compiled source-root fallback or implicit desktop-settings lookup.
Default settings are `<profile>/settings.bml`, with explicit read-only BML override
still supported. Imported Paths/Home cannot redirect writable system data outside
the profile's save/system tree.

Validation on macOS arm64:
- Configure/build both native targets after physically removing desktop-ui, with
  optional utilities OFF. Generated target help, host link command and compiler
  dependencies contain no desktop/Hiro targets, libraries or headers.
- All **40 registry variants** compare byte-for-byte with the pre-extraction host.
- Three strict TS targets; final sequential suite: **87 passed, zero failures,
  one optional private-Kirby-ROM test skipped** (88 total).
- New original GBA BIOS/cartridge fixture verifies precise missing-firmware
  diagnostics, explicit firmware loading, virtual A press/release, SRAM flushing
  and persisted boot-byte increment on a second run. Media and foreign imported
  home paths are untouched. No private game/firmware is needed for this fixture.
- New resource-root check distinguishes missing databases from a supplied original
  fixture database and proves no checkout/user database fallback.
- Actual Electron session/renderer-reload and actual Electron/MCP integration pass.
- Independent launch/rebuild acceptance passes with the extracted host/resources,
  unchanged MCP session/epoch and snapshot cleanup.
- Fresh `/tmp/ares-retired-package/Ares.app` copied-app acceptance passes: 875 GB
  frames, 68 MCP tools, packaged CLI scaffold/build/list/add/enable/open/reload/
  command and TSX hot replacement with checkout reads denied. Ignored evidence:
  `artifacts/package-mcp.json`.
- `git diff --check` passes.

Next: one repeatable fixture acceptance command, macOS CI, startup plugin recovery
and explicit manual/fault-check dispositions. Task 13 remains on hold. Existing
physical keyboard/gamepad, audible-output and serializer rollback-failure gates
are not closed by this extraction; broader desktop UI features are inventoried,
not silently claimed as Electron parity.


## 2026-10-21 — repeatable fixture acceptance, CI and startup plugin recovery (foundation slice 4)

Added `npm run accept`: prerequisite checks, configure/build both native targets
with the full default core set and no Hiro/desktop frontend, three strict TS
outputs, sequential generated-fixture tests, native transport/GB/NES smoke, actual
Electron session/MCP/plugins/reload/agent-tool/recovery gates, profile/crash/rebuild
isolation and fresh copied-package acceptance. It clears inherited ARES_* test
inputs, requires ffprobe/ffmpeg, rejects failed/skipped fixture results, stops on
failure and records per-gate logs plus revision/tool/build metadata and exclusions.
The default native output is independent `build-electron-acceptance/`; package
copies are removed after checks. Individual scripts remain available for focus.

Added `.github/workflows/electron-macos.yml` for macOS arm64 on push/PR/manual
runs, using the same command and preserving concise logs/JSON on failures. The
workflow YAML and command wiring were parsed/checked locally. It has not been
pushed/dispatched or run on GitHub; no hosted CI pass is claimed. Node 24–26 is
declared for development; `.node-version` pins 24.22.0 for CI. The package lock
remains authoritative for JS dependencies; CMake uses the existing deps.json pins.
Optional GBA smoke now forwards explicit ARES_SETTINGS instead of assuming an
implicit desktop configuration lookup.

`--disable-plugins` / ARES_DISABLE_PLUGINS=1 is a per-launch recovery mode in main.
It preserves persisted startup selections while reading only bounded manifest
metadata: no plugin builds/imports, leases, activation or watchers. It ignores
bundled/default and environment auto-enablement, keeps built-in controls alive
and remains in effect after renderer reload. The manager can disable saved IDs;
CLI `disable` also handles missing/broken manifests. Recovery-only `reset-startup`
clears enabled IDs/local directories without deleting storage, saves or source.
Enabling/adding/reloading requires a normal restart. Malformed startup config is
preserved until explicit repair/reset.

Validation:
- Initial full lane using the existing native build override: **15/15 gates,
  89 fixture tests, zero failures/skips**. This intentionally excludes the optional
  private-ROM test file, with that exclusion explicit in the summary/docs.
- The unmodified default command, `npm run accept --prefix electron-ui`, then
  passed against a fresh `build-electron-acceptance/` native build: **15/15 gates,
  89 fixture tests, zero failures/skips**. Fresh copied-package evidence records
  874 GB frames. Logs/summary: ignored `artifacts/acceptance/run-m02q9e/` and
  `artifacts/acceptance/latest.json`. Local runner Node 25.8.0, CMake 4.4.4,
  Electron 44.9.0; copied tools use bundled Node 24.22.0.
- Standalone recovery unit checks pass missing-build/manifest and malformed-config
  preservation/reset cases. Actual hidden Electron recovery passes a persisted
  infinite-loop plugin, responsive UI/native controls, CLI enable rejection,
  disable/reset, retained plugin storage and same-session safe renderer reload.
- The initial lane's copied app passes 68 MCP tools, 876 GB frames, bundled CLI
  authoring and packaged TSX hot replacement with checkout access denied.
- Ruby's standard YAML parser validates the workflow and acceptance entry point.
  Python's optional yaml module was absent; no new Python dependency was added.

`ACCEPTANCE.md` records explicit deferrals and completion conditions for physical
keyboard/gamepad takeover, audible output and serializer rollback-failure fault
injection. These remain unpassed product gates, not blockers to task 13 code.
Private Kirby/reference checks, soak/latency, other platforms and broader support
remain separate lanes/scope. Startup plugin recovery does not claim state/game
rollback or crash-save restoration.

The approved foundation is complete. The handoff/roadmap now direct the next work
to task 13: real settings, configurable input and complete author SDK declarations.

### Task 13 — presentation persistence foundation (2026-10-21)

Added main-owned, profile-scoped `preferences.json`, stable typed descriptors,
atomic writes, serialized updates and saved/applied/pending state. Moved theme,
scaling, scanlines and the temporary panel position from renderer localStorage.
Unknown keys survive writes and descriptor removal/readdition. Invalid known
values are diagnosed; malformed/future-version files cannot be overwritten.

Validation: all three TypeScript targets passed; three new settings store tests
passed; actual isolated Electron settings check passed (provisioning, controls,
file writes, renderer reload, bridge updates, invalid-value rejection). Added the
Electron check to the acceptance lane; the full lane has not been rerun for this
checkpoint. Firmware/native application, input configuration, plugin settings
registration and complete SDK declarations remain task 13 work.

The BML import selection now also persists as `paths.importBml`; startup
acknowledgement records the exact loaded snapshot, preserving pending changes
made during load. Added fourth store regression and extended isolated Electron
check: selection persisted, next-load pending flag cleared only after native
status, imported BML unchanged. Both passed; all three TS targets passed again.

Preserved explicit `ARES_SETTINGS` launch precedence over saved selections without
rewriting the profile. Explicit UI changes replace the launch override. Fifth
store regression passed, as did the three TS targets.


### Task 13 — registry-backed firmware settings (2026-10-21)

Added native `--settings-info` discovery for all 36 compiled firmware requirements
and imported BML values. Firmware keys share the loader's path helper. Native
`--settings-overlay` applies a separate immutable profile launch snapshot without
rewriting the import. Descriptor-driven Paths/Firmware controls support manual
paths, file selection, Save and Reset; profile overrides take precedence over
imports. Next-load state is acknowledged against the exact launch snapshot.
Extracted original GBA fixture generation for reuse in Electron tests.

Validation: native host rebuilt; all three strict TS targets passed; nine targeted
settings/runtime tests passed; actual isolated Electron settings test chose a GBA
firmware file, loaded the original fixture, acknowledged pending changes, removed
launch snapshots and reset without restarting the game. Imported BML remained
byte-identical. Plugin startup recovery also passed after the startup change.
The full acceptance lane has not been rerun for this checkpoint. Native audio,
palette/core controls, configurable input, shared plugin registration/UI and
complete SDK declarations remain task 13 work.


### Task 13 — live native settings and core options (2026-10-21)

Added bounded native runtime.settings/runtime.configure methods (no new MCP tools)
for volume, speaker mute, luminance, saturation and gamma. Apply occurs on the
emulation thread, checks identity/epoch and refuses active/reserved execution or
state upload. Main applies only the edited field with other applied values;
saved-but-pending edits are never replayed by unrelated changes. Failed apply
errors are tracked per setting. Native startup acknowledges only native live
fields so concurrent renderer presentation changes are not reverted.

Added next-load descriptors for supported loader/core/system options and host
feature discovery for GPU quality/supersampling controls. Removed the four old
forced startup overrides; defaults retain fast boot and 40% output volume.

Validation: host rebuilt and all three TS targets passed. Fourteen targeted
settings/runtime/loader/session tests passed, including real GB black/restored
pixels, zero/restored PCM, native busy/stale rejection, invalid raw protocol
arguments, and persisted N64 interpreter/4 MiB Expansion Pak behavior. Existing
audio/MCP WAV regression passed with its required local socket permission. Actual
Electron settings check passed: live control/native readback, busy saved pending
value, unrelated changes do not replay it, retry applies, UI reload persists and
firmware GBA flow still works. Actual Electron MCP integration passed. Physical
audible-output acceptance remains deferred; no audible pass is claimed.

### Task 13 — configurable frontend input (2026-10-21)

Added profile descriptors for virtual-port-1 keyboard bindings, N64 A/B overrides,
keyboard analog directions, gamepad device/focus selection, button/axis indices,
inversion and deadzone. Empty keys and -1 indices disable mappings. Added key
recording to the shared descriptor form; binding changes clear held input before
polling the new map. Renderer mapping is now a pure shared function, preserving
prior defaults. Native direct-keyboard/peripheral BML bindings remain separate.

Validation: three strict TS targets passed; three input tests passed (defaults,
N64 exceptions, custom keys/devices, deadzone/inversion/finite bounds, defocus,
invalid bindings/indices and restart persistence). Actual isolated Electron
settings check recorded/saved a key, observed input clear, reloaded and retained
it; prior live audio, busy/nonreplay and GBA firmware checks still passed.
Physical keyboard/gamepad acceptance remains deferred, not inferred from these
virtual events. Shared plugin settings registration/UI and complete SDK types
remain task 13 work, followed by full acceptance.
