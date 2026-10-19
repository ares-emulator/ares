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
