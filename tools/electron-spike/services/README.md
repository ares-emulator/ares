# Session service boundary (task 1)

`contracts.ts` owns the JS schemas, validation, identity and service client
interface. `native-client.ts` implements bounded request correlation over the
native process pipes. `session-router.ts` is owned by Electron main, and survives
renderer reloads. `main.cjs` binds the router to a narrow preload IPC call. Future
inspector and MCP clients must use these contracts and routing rather than opening
another emulator or implementing a second execution loop.

Build with `npm run build:services`; generated CommonJS and declarations go into
ignored `dist/`. `npm start`, `npm test` and `npm run smoke` build them first.
The adapter entry point belongs in `mcp/` in task 3; no MCP server or external
listener is exposed in task 1. Native implementations belong in `native/` and
runtime checks in `tests/`. No workspace/package restructuring is required.

## Native protocol v1

Electron assigns a random session ID when it spawns the host and passes
`--control-session ID`. The host starts at epoch 1; a completed reset increments
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

All four methods take no payload. Unknown versions, missing/extra arguments and
invalid request IDs yield `invalid_request`. Unknown methods yield `unavailable`;
wrong session or epoch yields `stale_session` or `stale_epoch`. Failed mutations
leave the machine untouched. Malformed IDs use the response ID `invalid`.

Replies are newline-delimited JSON on child fd 3, separate from binary AEF2 video
on stdout and diagnostics on stderr:

```json
{"protocol":1,"id":"req1","session":"example","epoch":1,"ok":true,"result":{"paused":false,"system":"Synthetic","synthetic":true,"capabilities":["session.status","session.pause","session.resume","session.reset"]}}
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

The renderer supplies identity for mutations. It updates the pause label only
from successful native results and refreshes after stale-request errors without
retrying the mutation. An internal main-process client may use its current known
identity. Reload retains the process and frame credit; the latest outstanding
frame can be reattached to a fresh port. Old port acknowledgements are ignored.

Input/ack/quit remain the small existing stdin commands. The old `pause 0|1` and
`reset` commands are replaced by service requests. There is no legacy telemetry
compatibility claim. Local instance selection, external connections and packaged
Node launch remain task 3 work. Only macOS arm64 has runtime evidence.
