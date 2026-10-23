# Shared session, memory and debugging services

`contracts.ts` owns the JS schemas, validation, identity and service client
interface. `native-client.ts` implements bounded request correlation over the
native process pipes. `session-router.ts` is owned by Electron main, and survives
renderer reloads. `main.ts` binds the router to a narrow preload IPC call. Inspector and MCP clients use these contracts and routing rather than opening
another emulator or implementing a second execution loop.

Build with `npm run build`; generated ESM and declarations go into ignored
`dist/`. The sandboxed preload is TypeScript source (`preload.cts`) compiled to
`dist/preload.cjs`. `bridge.ts` types the IPC surface, and
`renderer-globals.d.ts` types the renderer’s `window.ares` API. `npm start`, `npm test` and `npm run smoke` build them first.
The adapter entry point is `mcp/server.ts`; task 3 adds the opt-in Electron-main
local endpoint in `local-endpoint.ts`. See [MCP usage and distribution](../mcp/README.md). Native implementations belong in `native/` and
runtime checks in `tests/`. No workspace/package restructuring is required.

## Native protocol v1

Electron assigns a random session ID when it spawns the host and passes
`--control-session ID`. The host starts at epoch 1 (`--paused` supports isolated initialization tests); a completed reset increments
its epoch. Replacing the host/ROM produces a new session. Session and request IDs
are 1–64 ASCII letters, digits, underscores or hyphens. Epochs are positive safe
JS integers. IDs correlate requests within one connection; they are not durable
idempotency keys.

Requests use stdin, one whitespace-delimited line:

```
control 1 REQUEST_ID SESSION_ID EPOCH session.status
control 1 REQUEST_ID SESSION_ID EPOCH session.pause
control 1 REQUEST_ID SESSION_ID EPOCH session.resume
control 1 REQUEST_ID SESSION_ID EPOCH session.reset
```

The session methods take no payload. Task 2 adds these methods to protocol v1:

```
control 1 REQUEST_ID SESSION_ID EPOCH memory.regions
control 1 REQUEST_ID SESSION_ID EPOCH memory.read REGION_ID DECIMAL_OFFSET DECIMAL_LENGTH
control 1 REQUEST_ID SESSION_ID EPOCH memory.write REGION_ID DECIMAL_OFFSET HEX_BYTES
```

`memory.regions` returns `regions` descriptors (`id`, `name`, `size`, `readable`,
`writable`, `addressing: "region-offset"`, optional `unavailableReason`). IDs are
opaque and scoped to one native session. Reads return `region`, `offset`, `length`
and `hex`; writes return `region`, `offset` and `written`. No empty transfers are
accepted. A request transfers at most 1024 bytes; a write is even-length hex.
Every byte and the entire range are validated before invoking the first callback.
Offsets are nonnegative uint32 values; neither the offset nor length wraps at the
region boundary. Errors include `out_of_bounds`, `stale_region`,
`region_not_readable`, `region_not_writable`, and `unsupported_region`.

The emulation-thread service uses debugger nodes and their actual callback
metadata, with conservative overrides discovered in the audit:

- N64 `DCache`: unavailable, because it exposes cache-view semantics and a size
  unrelated to the installed RDRAM. Task 5 owns that view.
- N64 `RDRAM`: readable, but writes are unavailable in this service. Its existing
  debugger callbacks follow dynamic chip mapping and can ignore writes before
  initialization. It is not a linear backing-storage write API. Task 5 must
  supply explicit N64 physical/CPU-view semantics and fault reporting.
- Other enumerated nodes: callback availability determines readable/writable.
  This is debugger-region access, with no CPU virtual addressing, cache coherence,
  instruction patch invalidation or rollback promise.

`memory.ts` supplies `MemoryClient`, identity-bearing `RegionHandle`s, byte/hex
conversion and unsigned 8/16/32-bit helpers that require explicit little/big endian.
Browser and Node builds use this same code; the bundled memory plugin in
`bundled-plugins/memory/plugin.js` is the inspector UI.
A failed stale request is never automatically re-enumerated and retried.

Unknown versions, missing/extra arguments and
invalid request IDs yield `invalid_request`. Unknown methods yield `unavailable`;
wrong session or epoch yields `stale_session` or `stale_epoch`. Failed mutations
leave the machine untouched. Malformed IDs use the response ID `invalid`.

Replies are newline-delimited JSON on child fd 3, separate from binary AEF2 video
on stdout and diagnostics on stderr:

```json
{"protocol":1,"id":"req1","session":"example","epoch":1,"ok":true,"result":{"paused":false,"system":"Synthetic","synthetic":true,"capabilities":["session.status","session.pause","session.resume","session.reset","memory.regions","memory.read","memory.write"]}}
{"protocol":1,"id":"req2","session":"example","epoch":1,"ok":false,"error":"stale_session"}
```

The native command reader only enqueues service requests. The emulation thread
validates and dispatches them between `root->run()` calls, also when paused.
Pause completion means the outer emulation loop is paused at that safe point;
it does not claim instruction-level or exact VI-boundary semantics. Reset reuses
the existing native power action and preserves pause state. Its inclusion keeps
the existing UI action ordered with status/pause and epoch validation.

A separate control writer drains replies. It never waits for video credits.
Request lines are capped at 4096 bytes, native request and reply queues at 64
entries each, client outstanding requests at 32, and JS buffered replies at
65536 characters. Oversize input/queue overflow closes the native connection
rather than silently dropping accepted mutations. The main client reports
transport/protocol errors and terminates a failed host; it never replays a
mutation. A 30-second request timeout allows native startup; a timeout may have an
unknown mutation outcome and provides no rollback guarantee.

The renderer supplies identity for memory reads and all mutations. Memory-region enumeration is safe discovery. It updates the pause label only
from successful native results and refreshes after stale-request errors without
retrying the mutation. An internal main-process client may use its current known
identity. Reload retains the process and frame credit; the latest outstanding
frame can be reattached to a fresh port. Old port acknowledgements are ignored.

Input/ack/quit remain the small existing stdin commands. The old `pause 0|1` and
`reset` commands are replaced by service requests. There is no legacy telemetry
compatibility claim. Only macOS arm64 has runtime evidence.


## Electron local endpoint v1 (task 3)

`ARES_ENABLE_SERVICES=1` or `--services` enables a user-private Unix socket.
Metadata is created mode 0600 in an owned mode-0700 directory and contains the
app UUID, socket path and random token. The client requires an explicit metadata
file and instance UUID. A hello checks both UUID and token, then assigns a fresh
connection UUID. Every subsequent request carries that connection UUID and a
unique request ID. Duplicate IDs and old connection UUIDs fail before dispatch.
App identity survives renderer/native-host replacement; native session and epoch
remain separate, independently validated identities.

Frames are newline-delimited JSON. A hello is
`{protocol:1,type:"hello",id,instance,token}`; success returns
`{protocol:1,type:"hello",id,instance,connection,ok:true}`. Requests are
`{protocol:1,type:"request",id,connection,method,identity?,params?}`. Replies are
`{protocol:1,type:"reply",id,connection,ok:true,reply:<native envelope>}` or
`{protocol:1,type:"reply",id,connection,ok:false,error:<code>}`. Methods/parameters
reuse the shared contract. Only status and region enumeration allow identity-free
discovery; other requests must bind a known native session/epoch. Native reset is
reserved for UI routing in this slice. No TCP listener or arbitrary command route
is exposed.

Limits are 8 clients, 16 in-flight requests per client, 1024 unique requests per
connection, 65536 characters per line and a bounded 65536-byte outgoing queue.
A connection that reaches its request limit ends explicitly; a subsequent new
call may reconnect. The native client's shared 32-request bound still applies.
The hello has a 3-second deadline and client requests have 30-second deadlines.
Timeout, invalid replies and disconnection reject pending requests and destroy
the affected connection. Only a new call reconnects; no request is replayed.
A mutation accepted before disconnect may have completed: no rollback or owned
long-operation cancellation is claimed before task 4.

`app_not_running`, `app_disconnected`, `wrong_instance`, `unauthorized`,
`stale_connection`, `duplicate_request`, `request_timeout`, `busy` and
`native_host_disconnected` are explicit failures. Native stale-session/epoch and
region failures retain their native result envelope. Successful discovery updates
the client identity; `MemoryClient` handles retain their original enumeration
identity, so reconnect does not silently rebind an old memory write.

The listener and metadata outlive renderer reload and native-host loss. Normal
app shutdown closes clients and removes its metadata/socket. A crashed app can
leave stale metadata; selecting it fails instead of picking another app. Windows
returns `unsupported_platform`; Linux/macOS Unix code is shared, but only macOS
arm64 is runtime verified. Platform-specific transport/runtime validation remains
pending before advertising broader support.


## N64-only services

`n64-contracts.ts` adds lossless architectural hex values, register/stop DTOs,
strict memory views and break/watch predicates. Native implementations are in
`n64-service.hpp`; the CPU hooks remain plain structs without transport or JSON.
`execution.stepInstructions` shares the existing owner/coordinator and retains
its frozen result. Debug stops return architectural snapshots, not later live
register reads. Definition lists use 32-entry pages to bound pipe replies.

Main observes status every 250 ms with at most one poll pending and suppresses
callbacks from detached hosts. This catches ordinary-play CPU stops even with a
stalled/reloaded renderer or no new video. The observer does not drive execution.


`input-contracts.ts` adds four per-port connection owners, pending/latched states,
frame/wall leases and neutral-host mode. `InputService` runs on the emulation
thread, ahead of normal input mapping/focus filtering. The reader only requests
human takeover with an atomic flag. The existing reserved owner-release request
revokes input leases as well as owned execution. Default external resume releases
only its own holds; UI resume can release all. N64 keepOverrides preserves holds.

Task 7 adds a main-owned method, `artifact.traceExport`, alongside
NATIVE_METHODS. It is validated by the shared/browser-safe contract but routed
before native serialization. SessionRouter advertises that capability and owns
bounded export jobs and revocation. Native code remains responsible for immutable
trace pages. Bulk file writes are asynchronous; only final hard-link/rename
publication is synchronous, immediately following the main-thread lifetime guard.
Temporary files are private exclusive siblings, with byte/time limits and SHA-256.

Native video.begin/read/release pin immutable RGBA8 snapshots with connection and
epoch identity, independently of video credits. Main artifact.screenshot fetches
bounded pages, uses png-worker.js for crop/nearest-neighbor/PNG, and publishes via
the shared artifact writer. artifact.imageRead/imageRelease provide bounded PNG
transfer to MCP; the adapter assembles at most 8 MiB and verifies SHA-256 before
returning ImageContent. The frame observer is installed before execution and
removed after unload. Submission tagging avoids reading emulator epoch/frame
state on the screen worker thread and rejects delayed pre-reset output.

Audio capture is an emulation-thread service over the host's 48000 Hz post-mix
samples, before output muting. `audio.capture/status/begin/read/release` use bounded
PCM buffers and connection ownership. Electron main owns `artifact.audioSave`:
it downloads an immutable PCM pin, creates the bounded WAV header, and publishes
with the shared atomic writer. Consumption follows file commit; failed exports
release only the pin, and disconnect/reset revoke native capture. A published file
remains reported if subsequent native cleanup fails. Audio is independent of the
renderer and disabled until requested.

`video.record.*` owns bounded native BGR frame recording. Its render callback
receives the same immutable submission tags as screenshots and never reads core
coordinates. A separate mutex protects frame collection; service requests freeze
and pin it. Main's `artifact.videoStop` streams AVI headers, bounded native pages
and an index through the atomic publisher. Failed publication releases the pin
without consuming frames. Owner disconnect/reset revoke collection and export.

State inspection lives in main: `state.info/list/peek` read bounded regular files,
validate sidecar hashes and decode saved `lsb32` RDRAM. The only native reads are
compiled format/slot names and, on explicit legacy-file assumption, serializer
prefix layout. Neither touches CPU serialization or pause/restore. Slot names are
computed without the legacy locate helper's directory-creation side effect.
`state.save` uses one native immutable snapshot pin and main-owned streaming
publication plus a separately published metadata sidecar. The new native layout
observer refactors serializer header/prefix code without changing field order.

`state.load` reserves native mutation dispatch for a connection-owned upload,
then commits at one native safe point after full-file preflight. The transfer
stays within the 4 KiB request/16 KiB writable-buffer budgets using four 1800-byte
chunks at a time; the paused loop wakes from a request condition variable. UI or own-connection pause/resume, reset, disconnect or human input revoke pending upload, so late file I/O
cannot override a newer user action. Native commit alone advances the epoch and
resets capture/coordinate ownership. Main returns the commit's new identity and
never applies an old-epoch guard after successful mutation.

### Task 8 memory experiments

`n64.memory.writeMany` accepts 1..32 `{address, view, hex}` writes (1024 bytes
total), `verifyAfterFrames` (0..100000) and `timeoutMs` (1..600000). The native
safe point validates the complete list and run target before applying ordered
writes. The reply contains write hazard results, immediate readbacks/matches,
`side_effects_rolled_back:false` and an optional owned execution operation.
Final operation snapshots capture `memory_verification` before releasing execution
ownership; the last 16 remain immutable. UI pause, debug hits, deadlines and
owner disconnect use existing operation stop semantics. Native state uploads
exclude this mutation. Restore/power's deferred video initialization is prepared
without guest CPU execution before writing hidden RDRAM; absent backing rejects.

### Native-reserved batches

`experiment.batch` is main-only, with a normalized strict discriminated step
union. `Batches` allocates a private child owner for the authenticated parent.
`execution.batchBegin(timeoutMs)` acquires the native exclusive reservation;
`execution.batchCheck(batchId)` fences every next step. They are internal methods
and unavailable directly over the public local endpoint. Ownership and a maximum
20-second deadline stay native; main merely sequences existing services.
`execution.release` revokes child operations and resources. UI controls and human
takeover remove the reservation even without a child run. The batch-owner prefix
rejects late mutations after revocation. State upload is allowed for the child and
still excludes other owners. An owned successful restore updates the internal
epoch; external changes never rebind subsequent steps.

One batch at a time, 32 steps, 1M requested frames, 16 KiB memory, 8 artifacts,
48,000-byte request parameters and 48 KiB result. The result records attempted
and successful step counts, actual run snapshots and committed artifacts. No
rollback. Parent pause/cancel is forwarded only to that parent's child generation;
foreign owners cannot cancel it. Disconnect/timer/detach abort file workers and
issue native release. See MCP docs for schema/default reductions and cancellation
semantics. Tests cover native idle reservations/expiry, real active-VI N64 PNG
sweeps, partial failures, own-load epochs, external reset, parent cancellation,
disconnect and commit-versus-cancel reporting.

### Task 9 machine services

`MachineService` runs on the native emulation thread. Its five typed methods are
appended to native capabilities: n64.cpu.setRegisters, n64.cpu.disassemble,
n64.rsp.registers, n64.rdp.state and n64.peripheral.registers. The mutation accepts
at most 31 distinct indexed GPRs plus optional PC/HI/LO/LLbit/Status and validates
all fields before writing. Session pause, active execution, GDB, upload and batch
ownership rules still apply. Host executor reporting updates after PC edits can
release the CPU debug gate. Disassembly is at most 128 aligned instructions and
uses the existing non-filling D-cache Word debug read, now explicitly instantiated
for external native services. Serializer bytes and core run scheduling are unchanged.

Read-only observations are explicitly allowed during uploads/batch reservations.
Snapshots use direct core fields and a local disassembler, including the RSP
semaphore and DMA engine, with no MMIO reads. Machine contracts validate vector
sizes/lane widths, lossless addresses/registers, signed decimal DMA clock and
structured peripheral groups. RSP debug metadata remains unavailable until the
core gate is ported. See MCP docs for bounded/schema differences and evidence.

## Memory sampling and native storage

`memory.readMany` and `n64.memory.readMany` bound requests to 64 ranges / 64 KiB.
Hex expansion and metadata fit a bounded 192 KiB native/local reply line. Individual
legacy `memory.read` limits remain 1024 bytes. CPU-view DCache does not masquerade
as a raw region. All reads in a sample occur in one emulation-thread dispatch.

Native memory tools share the plugin/MCP service path:

- `memory.snapshot.capture {reads, scheduling: 'immediate'|'boundary'}` reserves
  up to 16 MiB of immutable bytes. Immediate works paused; a paused boundary
  request reports `paused_boundary_unavailable` and never resumes execution.
- `memory.recorder.start {reads, capacity, duration}` stores at most 64 KiB/sample
  in a ring, up to 1024 samples. Duration counts frames independently of capacity;
  zero records until stopped. Recording supports N64/synthetic VI boundaries and explicitly labelled primary-
  screen frame boundaries on other cores; absent coordinates reject.
- `memory.capture.status`, `memory.recorder.stop`, `memory.capture.release`
  accept `{handle}`. There are at most 32 captures, 4 recorders and 64 MiB of
  total reserved sample storage. Snapshots and rings share this budget.
- `memory.capture.read {handle, sequence, range, offset, length}` pages up to
  16 KiB of an immutable retained sample. Status includes first/next sequence and
  dropped count. `cursor_overwritten` requires restarting at first sequence.
- `memory.snapshot.compare {before, after, range, type, endian, filter, cursor,
  limit, value}` scans at most 64 KiB per call and returns up to 256 typed values.
  Types are unsigned u8/u16/u32 or f32, alignment is relative to the captured range,
  and endian is explicit. changed/unchanged compare bits (including NaN payloads);
  ordered/delta exclude either NaN; equals excludes a new NaN, float equality is exact, integer delta uses
  mathematical subtraction without wraparound. Nonfinite display values are null
  with exact raw hex retained. Follow next_cursor even for empty pages.

Handles are connection/plugin-owner and epoch scoped. Reset/load invalidates
captures; disconnect/unload releases resources. Compare reports content changes,
not writes that later restore a value. Candidate handles, content maps and non-N64 screen scheduling are described below.
Non-stopping call/write buffers remain an investigation.
No performance or zero-slowdown claim is made for the recorder.

Batches accept up to 256 steps, 16 KiB aggregate memory transfers, 1M total frames,
8 artifacts and a 600-second deadline. The bounded result budget is now 128 KiB:
the real 50 run/read sample fixture exceeded the old early-stop budget. The
orchestrator reserves room before effects and reports `result_limit` explicitly.
MCP adds reference-facing `steps` while retaining `results` for existing consumers.
MCP controller set/state present named `buttons` and lossless `buttons_mask`;
native/SDK numeric button contracts and owner UUIDs remain unchanged.

RDP artifacts now include exact-capture decoded commands (`seq`, opcode, name,
address, source, words, batches, frame, incomplete/expected_words). The table and
names are ported with attribution from ares-64 commit
`1c9b9637476a83f7b80d3db1115a0f71529820d9`. Decode happens after capture; raw words,
batches/tasks, limits, drops and completeness are retained. The envelope uses
`kind: ares-rdp-capture`, epoch/interval/complete/truncated/dropped_words and the
existing detailed summary. ROM/renderer/start/end envelopes and OSTask association enrichment now follow
the reference rules, as detailed below. Legacy viewer lengths are separate.

Pause remains immediate at the next native service safe point. Check
`at_frame_boundary` before exact next-VI capture; do not assume pause advances to a
VI. Controller timeout is an upper bound: disconnect, reset or takeover can release
earlier. Some MCP clients need restarting to discover newly registered tools.
Development CDP is an optional Electron `--remote-debugging-port` launch flag; UI
screenshots do not require CDP or disabling Chromium sandboxing.

## Native candidates, content maps and recorder navigation (continuation)

`memory.query.start` accepts `{before, after, range, type, endian, filter, value,
candidates}`. `candidates: 0` scans all aligned values; otherwise it narrows a
completed query with matching region/offset/length/type/endian provenance. Both
range start and length must align to the type. `memory.query.advance {handle}`
processes at most 64 KiB of values and reports running/done/failed, scanned/total,
candidate_count and an explicit error. Up to eight queries reserve 1 MiB each,
independent of the 64 MiB capture budget. More than 262144 matches fails with
candidate_budget_exhausted; failed partial sets cannot be narrowed or paged.

`memory.query.page {handle,cursor,limit}` returns up to 256 typed matches after
completion. Query cursors index candidates, not bytes. Snapshot bytes must remain
alive while advancing or paging values. Completed candidate offsets remain usable
for narrowing after those old snapshot handles are released. Release with
`memory.query.release`; a source used by a running descendant returns query_in_use.
Owner disconnect/unload and epoch changes clear both captures and candidate sets.
`equals` compares the new value and excludes a new NaN; ordered/delta comparisons
exclude either NaN. Changed/unchanged compare bits, including signed zero and NaN
payloads. All returned finite floating values preserve double round-trip precision.

`memory.snapshot.changes {before,after,range,cursor,limit,granularity}` reports
changed contents as adjacent byte runs, 16-byte lines or 4096-byte pages. At most
64 KiB is examined per request, with up to 256 groups. Cursors are range-relative
bytes; line/page groups are anchored to absolute region offsets. Run continuation
flags identify a group cut by the work bound. These are content maps; writes that
restore the original value are intentionally invisible. Freeze consecutive ring
samples to obtain per-frame maps through the same service.

`memory.capture.freeze {handle,sequence}` deep-copies a retained sequence into a
budgeted native snapshot. It preserves the source frame and survives ring wrap.
Non-N64 systems with a screen now use the first registered screen's submission
boundary, labelled primary-screen-frame rather than N64 VI. Other screens do not
double-count. Immediate snapshots still work without a screen. Paused boundary
requests reject without resuming. Status includes first/last frames, native
last_copy_ns/max_copy_ns, copied_bytes and fetched transferred_bytes. Copy metrics
exclude renderer transport and are measurements, not a zero-slowdown guarantee.

The RDP envelope now freezes ROM/renderer/start/end at capture time and enriches
OSTask descriptors and batch associations using the reference rules. Native raw
header/current/end/task indices remain available under additive aliases. The
software renderer name is implementation-specific; it is not labelled angrylion.
The core debugger callback audit and non-stopping instrumentation investigation
are recorded in MEMORY-SAMPLING-AUDIT.md.
