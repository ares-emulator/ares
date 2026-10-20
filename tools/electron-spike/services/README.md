# Shared session and memory services (tasks 1–6 input slice)

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
Browser and Node builds use this same code; `inspector.ts` is the temporary panel.
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
