# Execution roadmap: how to tackle the implementation plans

Status: tasks 1–5 and the task 4.5 TypeScript/ESM detour implemented; milestone 1
integration gate passed on macOS arm64.
Runtime evidence and pending checks are recorded in
[EXECUTION-LOG.md](EXECUTION-LOG.md). Task 6 is in progress: input ownership, bounded PC/call traces and PI DMA provenance are implemented;
profiling and controller taps are implemented; automated and package checks passed.
Milestone 2 physical keyboard/gamepad acceptance remains pending. Task 7 is in progress: atomic artifact publishing, trace-file export and native screenshots are implemented.
Audio/recording and later tasks remain planned. Start here when
choosing the next task.

## Which plan owns what

| Document | Use it for |
| --- | --- |
| This roadmap | Task order, dependencies, scope boundaries and completion gates. |
| [MCP-PORT-PLAN.md](MCP-PORT-PLAN.md) | Source provenance, the 68-tool inventory, native debugging semantics and MCP parity. |
| [PLUGIN-PLAN.md](PLUGIN-PLAN.md) | Plugin manifests, UI contributions, game matching, hot replacement and bundled settings. |

Architecture decisions already made: native C++ emulation, Electron/web UI,
TypeScript MCP adapter, shared typed service contracts and one native execution
coordinator. The Python bridge and legacy TCP protocol are references, not shipped
dependencies. Preserve useful agent-facing behavior without reproducing the old
desktop architecture. If an implementation reveals a conflict, update the affected
plans explicitly rather than quietly changing semantics.

## Working approach

Implement one bounded task at a time, as a reviewable change with actual runtime
evidence. A task may require several commits; do not start by merging the entire
fork or building the entire SDK. Extract only the native dependencies needed for
the current slice, recording their source commits and preserving attribution.

Keep the existing emulator usable after each task. Do not use the active user
session as a test fixture: use one isolated test instance, copied saves and
disposable states/artifacts. Close that instance when verification finishes.

Create a parity ledger during task 1, with one row per existing MCP tool: reference
schema/defaults, required backend, implementation status, test/evidence, and known
limits. A tool registration alone is not a passing result. Record checks as passed,
failed or not run, and carry forward outstanding manual checks.

After each task, record what landed, relevant commands/results and the next task.
Run focused checks appropriate to the change; broaden verification at milestone
gates. Do not repeat a full-core rebuild for documentation-only work.

## Milestone 1: prove one service path end to end

Maps to MCP phase A and plugin-plan step 1. Complete tasks 1–3 before adding the
full plugin loader or advanced N64 debugging.

### Task 1 — contracts and a minimal session round trip

Deliver shared TypeScript schemas/client interfaces, a versioned native control
protocol, request IDs, session/epoch identity and native safe-point dispatch.
Implement status and pause/resume through it. Preserve the dedicated video/log
separation and keep control replies independent of video acknowledgements.

Establish proposed modules for contracts/client code, Electron main routing,
native services, MCP entry point and tests. Keep boundaries clear without turning
this into a monorepo/package reorganization prerequisite. Create the 68-tool ledger
from the audited source and annotate existing tests that will be migrated.

Done when: requests work while running and paused; a stalled canvas cannot block
status/pause; invalid messages and stale session IDs fail explicitly; existing
ROM loading/video/audio still work. No plugin system or CPU instrumentation here.

### Task 2 — native memory regions and bounded access

Build on task 1. Enumerate ares debugger regions using opaque session-scoped IDs;
implement bounded bulk reads/writes on the emulation thread and explicit-endian
typed helpers on the JS side. Audit writable metadata and reject unsupported
access rather than silently wrapping. Add a temporary inspector against this API.
The task 2 audit found that N64 `RDRAM` follows chip mapping and can ignore writes
before initialization; this view is read-only here. `DCache` is unavailable. Their
explicit N64 write/cache services remain task 5 work and remain parity targets.

Done when: N64 and GBA regions can be inspected; deterministic fixture memory can
be written/read back/restored; boundary and stale-session checks pass. Label
region offsets clearly. N64 virtual-address/cache APIs come in milestone 2; do
not promise those semantics through raw region access.

### Task 3 — TypeScript MCP adapter and first integration gate

Build on tasks 1–2. Implement a small Node stdio MCP entry point, sharing schemas
and service client code with the inspector. Connect to Electron main's local
endpoint with explicit instance selection. Keep adapter stdout MCP-only. Verify
how the distributed app supplies its Node runtime; do not introduce a Python
fallback or depend on a developer checkout as the finished launch mechanism.

Initially expose only implemented status, pause/resume and region-memory tools.
Port their useful MCP names/defaults from the reference and test the real stdio
entry point, not just direct function calls.

**Milestone gate:** MCP and the temporary inspector see the same paused bytes in
one native emulator. Reloading the renderer keeps the external MCP connection
alive. Wrong-instance, disconnected-host and stale-request cases are explicit;
reconnect never replays a mutation. No second emulator is launched.

First useful kickoff prompt:

> Implement task 1 of EXECUTION-ROADMAP.md, following MCP port phase A. Establish
> typed service contracts and native safe-point dispatch for status/pause/resume,
> preserve video/audio/loading, create the MCP parity ledger, and verify the
> round trip. Do not implement the plugin loader or port the entire fork yet.

## Milestone 2: restore controlled N64 debugging

Maps to MCP phase B. Keep N64-specific services separate from generic region APIs.

### Task 4 — core coordinates and execution ownership

Port the minimum VI/epoch and bounded-run hooks, operation state machine,
deadline/cancel/disconnect behavior and owner identities. Implement exact-frame
runs and operation polling. Renderer frame counts are not emulator coordinates.
Make UI pause/stop work through the same coordinator. Test servicing while paused
and controlled execution without normal audio/focus/frame pacing interference.

Done when: a bounded operation has one final result, competing owners get explicit
busy results, cancellation/timeout are responsive, and ROM changes revoke stale
work. Use a synthetic fixture to verify exact frames and partial-frame behavior.

### Task 4.5 — TypeScript and ESM detour

Requested after the task 4 backend checkpoint. Move Electron main, renderer,
preload and the frame parser to strict TypeScript; compile application services,
main and MCP to native ESM, and the renderer to browser ESM. Keep a single emitted
CJS preload because the renderer sandbox is enabled. Add shared IPC/window/frame
types, explicit relative import extensions, compiled-entry asset paths and matching
packaging/test entry points. This detour reuses the task 4 native binary.

Done when: clean TypeScript builds, real sandboxed Electron UI, real MCP stdio
(including task 4 ownership/reload/cancel cases) and copied-package checks pass.
Keep task 4’s outstanding integration failures visible until these gates pass.

### Task 5 — CPU debug engine and N64 memory semantics

Port the fork's required CPU/cache/exception hooks and debug engine. Deliver
instruction stepping, break/watch/stops, register inspection and strict N64
virtual-address/CPU-cache memory services. Include the minimum dependency hooks,
not every fork view. Preserve immutable hit snapshots and debugger-induced
interpreter switching with explicit executor reporting.

Done when: fixture assertions cover delay slots, before/after-instruction stops,
watch access ranges, alias/cache behavior and debug disable/recompiler restoration.
No claim of generic instruction stepping across every supported system.

### Task 6 — input ownership, traces and provenance

Port four-port injection, VI latching, leases and human takeover. Add PC/call
traces, paging/counters, DMA provenance and the required profiler backend. Port
relevant reference assertions instead of treating plausible-looking output as
evidence. Keep profiling/capture work disabled when unused.

**Milestone gate:** execute the migrated CPU fixture and ownership cases against
the new host. Verify real keyboard/gamepad takeover separately and record its
result. MCP and UI report the same stopped state. Ordinary play remains available
when instrumentation is disabled.

## Milestone 3: preserve the remaining MCP workflow

Maps to MCP phase C. Tasks 7–9 are separate changes; use the parity ledger to track
dependencies and omissions rather than calling the whole phase complete early.

### Task 7 — artifacts, images and audio

Extract bounded native frame/audio taps and artifact publishing. Preserve
framebuffer versus presented screenshots, epoch freshness, crop/scale, inline
MCP images, dump output and recording. Keep captures independent of canvas
readback. Test output bounds, overwrite protection, failed exports and cleanup.

Done when: real MCP stdio returns decodable images, native captures agree with
their documented sources, and recording limits/exports pass focused checks.

### Task 8 — states and repeatable experiments

Port path states, header/inventory/offline inspection and verified layout metadata.
Check fork/upstream state compatibility before accepting existing files. Deliver
multiwrite validation, delayed verification and bounded batch experiments through
the existing owner/scheduler. Never load/restore the active emulator to fake an
offline read or imply rollback of earlier batch side effects.

Done when: a disposable baseline-load/write/run/crop sweep works; incompatible
states fail clearly; offline reads leave live machine state unchanged; partial
batch failures report actual completed work.

### Task 9 — RSP/RDP and advanced CPU mutation

Port the required command/task/submission hooks, RSP debugger progress handling
and exact RDP interval capture. Add register mutation and experimental bounded
function calls/recovery with the reference's precise retained/restored state rules.
These may be split into smaller dependency-driven changes; do not shortcut native
hooks by reading reused command buffers after the fact.

**Milestone gate:** all 68 parity rows have runtime evidence or an explicitly
accepted scope change. Run migrated bridge/stdio/transport, CPU fixture,
paper-cut, game acceptance and recording checks on the actual new binary. Record
pending manual/fault-injection cases honestly. A previous fork binary's pass is
not a pass for the port.

## Milestone 4: make tools hot-loadable

Maps to MCP phase D and plugin-plan steps 2–5. Now build the extension machinery
on services that already support both agent and JS clients.

### Task 10 — minimal loader, panel and game matching

Implement local manifests, validation, enabled plugin directories, SDK registration,
scoped disposal/storage/logging and a dockable panel mount. Support plain JS first
and provide a TS/TSX template. Match game profiles by verified content/revision,
not filename. Reserve extension modes for overlays/pages/shell scripts without
requiring all of them in this task.

Done when: a generic memory panel works across N64/GBA, a Kirby-only panel activates
only for the right identity, errors are actionable, and disable/ROM switch revokes
the plugin's API scope. No marketplace or custom UI framework.

### Task 11 — hot replacement and cleanup

Implement watched builds into immutable artifacts, candidate validation, generation
leases, old-generation cleanup and state restoration. Exercise the failed-build
and failed-activation paths. Plugin reload must not replay writes, reset the game
or cancel an operation owned by the external agent.

Done when: repeated reloads do not accumulate listeners/timers/requests; stale
async work cannot write; an invalid edit keeps the previous working UI; recovery
behavior is demonstrated rather than merely described.

### Task 12 — the first actual agent-authored game tool

Move the temporary inspector into a bundled plugin. Add shared stop/register views,
then a small Kirby panel using verified structures. An animation selector is useful
once addresses are established; unknown addresses are a research task, not a reason
to invent them or declare the architecture blocked. Ship scaffold/build/reload/open
commands and SDK examples that an agent can use without UI scraping.

**Milestone gate:** MCP stops on a watch, the panel displays the same immutable hit,
and an agent edits/hot-reloads a matching tool while the game remains loaded.
Conflicting runs return busy and human control still works.

## Milestone 5: bundled settings and broader customization

Maps to plugin-plan step 6, remaining UI surfaces and MCP phase E.

### Task 13 — settings service and bundled settings plugins

Add validated settings descriptors with persistence/scope and apply behavior.
Port firmware/paths, presentation and audio first, followed by input and supported
core options. Use the same plugin API as local plugins. Keep imported desktop
settings read-only by default and Electron overrides separate. Remove spike
hardcoded overrides only as their real settings implementations land.

Done when: firmware configuration enables GBA, actual runtime settings change,
restart-required behavior is clear, settings survive restart, and a local plugin
can register its own settings section.

### Task 14 — overlays, custom workspaces and trusted shell extensions

Implement the remaining UI contributions with explicit input/focus and cleanup
contracts, plus safe-mode recovery. Allow a plugin to combine panels and broad
trusted UI customization. Do not call same-renderer plugins sandboxed. Build
profiler/flamechart/RSP/RDP/TMEM and asset workflows as demand justifies them,
reusing the already ported backends and declaring renderer-specific requirements.

**Milestone gate:** demonstrate a panel, overlay and shell customization together,
recover from a broken extension, and measure repeated reload/hidden-panel costs.
Keep minimal recovery and native session ownership outside replaceable plugins.

## Final validation and scope discipline

Use milestone gates to broaden testing and compare idle, normal play, controlled
debugging and active capture with one emulator instance. Track CPU/memory, bounded
queues and subscriptions. Confirm a non-N64 system still loads after native ports.
Develop on the established macOS arm64 setup; add platform-specific transport and
runtime packaging checks before claiming Windows/Linux support.

Do not make full ImGui parity, embedded QuickJS, a marketplace, remote access,
untrusted plugin sandboxing, or complete desktop settings parity prerequisites.
Conversely, none of those deferrals removes an existing MCP tool from the parity
target without an explicit scope decision. A clean host-library extraction can
follow when concrete coupling impedes a task; it is not an initial rewrite mandate.

The immediate next action is **task 5**. Task 4 owns bounded native execution;
task 4.5 supplies typed ESM application entry points and a sandbox-compatible
preload. Runtime evidence and remaining manual checks are in the execution log.
