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
