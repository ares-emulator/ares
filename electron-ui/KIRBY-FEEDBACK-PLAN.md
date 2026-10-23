# MCP compatibility and memory tooling implementation plan

Written 2026-10-21 following the
[feedback triage](KIRBY-FEEDBACK-TRIAGE.md) and user discussion. This is a plan,
not evidence of implemented features or authorization to claim acceptance.
It covers all reported findings, including the longer-term memory explorer.
Existing foundation/workspace work stays complete; deferred shell extensions
and unrelated platform work are not prerequisites.

## Decisions

- Batched memory reads belong in the universal region/offset API. N64 virtual
  address translation and cache-aware CPU views remain system-specific adapters.
- Reuse the decoded RDP export behavior in the local `../../ares-64` reference.
  Do not invent another command schema.
- Stable MCP names select an app across launches; fresh UUIDs/tokens authenticate
  each individual launch. Preserve explicit selection and session/epoch checks.
- Preserve immediate pause and connection-owned controller input. Document these
  semantics; do not silently advance debugging or detach input on disconnect.
- Implement one bounded slice at a time. Native data collection and queries should
  serve plugins and MCP through shared services, not duplicate execution loops.

## Sequence and dependencies

| Slice | Work | Depends on | Relative size |
| --- | --- | --- | --- |
| 1 | Request overload and error contract | None | S–M |
| 2 | Universal atomic ranged reads | 1 | M |
| 3 | Batch capacity and MCP response compatibility | None | M |
| 4 | Reference-compatible decoded RDP export | None | M |
| 5 | Named MCP discovery and shutdown cleanup | None | M |
| 6 | Plugin observability, UI follow-up and documentation | 1; current UI inspection | M |
| 7 | Native ranged per-frame recorder | 2 | L |
| 8 | Native snapshots and memory queries | 2; recorder scheduling from 7 for timed capture | L |
| 9 | RAM-diff explorer | 8 | L |

Slices 1–6 address current workflow friction. Slices 7–9 are separately reviewable
feature projects; non-stopping execution hooks are an additional investigation.
Start with 1, then 2. Remaining independent slices can follow in table order.

## 1. Make overload behavior observable and bounded

Feedback 14 and 16. Reproduce the 24-read `Promise.all` workload through the real
plugin bridge. Current scope and registry each reject at eight pending requests;
inspect both the host path and plugin refresh cleanup before attributing the hang
to transport. Also check native-client and local-endpoint limits.

Define documented error behavior: native service failures return `ok: false`;
SDK admission/lifecycle failures reject with a stable code and useful limit
details. Keep `busy` for execution contention distinct from request saturation.
Use one documented request-admission limit per plugin and ensure bookkeeping
unwinds on success, failure, cancellation, reload and disconnect. Avoid unbounded
queues or automatic replay of requests. Examples must use `try/finally` so a
rejected aggregate cannot permanently latch a refresh flag.

Touch points: `plugins/scope.ts`, `plugins/registry.ts`, bridge/preload, service
clients and `plugins/README.md`. Completion: all 24 promises settle within a bound,
accepted work completes, excess work reports overload, pending counts return to
zero, and a later refresh succeeds. Include an actual Electron plugin regression.

## 2. Add universal atomic ranged reads

Feedback 15 and 18. Extend `memory.read` with a separate `memory.readMany` operation:

```ts
// Proposed public contract; limits below are initial implementation targets.
memory.readMany({
  reads: [{ region: regionId, offset: 0x100, length: 0x6000 }]
})
// result: { reads: [{ region, offset, length, hex }],
//           epoch, frame, frame_kind, sample_id, at_frame_boundary }
```

Use region handles from `memory.regions`; support multiple regions in one request.
Prevalidate every range, readability and total size before any callback. Preserve
request order and allow overlaps. Start with at most 64 ranges and 64 KiB total,
including individual ranges above 1 KiB. Review native line/reply, IPC and endpoint
budgets for hex expansion and metadata; use bounded paging of immutable results
if a transport cannot accommodate the target. Never silently shrink the sample.

Execute all reads in one native emulation-thread service dispatch, with no guest
execution between them. Default to the next available service safe point, which
works while paused and does not require running another frame. Report whether it
was a frame boundary. Use an epoch-scoped monotonic sample identifier so multiple
samples in one frame remain distinguishable. Region callbacks must be checked for
side effects/unsupported views; promise consistency of supported memory views,
not synchronization of unrelated external devices.

Add `MemoryClient.readMany`, shared contract encoding/validation, native handling,
capabilities, execution read allowlists, plugin access and generated SDK docs.
For N64 CPU virtual addresses, expose a companion `n64.memory.readMany` backed by
the same admission/metadata model and existing cache-aware translation. Do not
pretend universal raw region offsets are CPU virtual addresses. The Kirby sample
can use either raw RDRAM offsets or the explicit N64 CPU-view adapter as needed.

Completion: non-N64 fixture plus N64 coverage, the supplied ~25 KiB Kirby workload,
all-or-nothing validation, overlaps, exact limits, stale handles/epochs, paused and
running sampling, and two samples sharing a frame but differing in sample ID.
Verify native dispatch performs the whole copy without yielding to emulation.
An optional next-frame-boundary mode can use slice 7's scheduler; it is not needed
to ship instruction-safe atomic reads.

## 3. Restore practical MCP script compatibility

Feedback 1 and 2. Raise batch request/result step bounds to 256 across MCP schemas,
service validators and docs. Audit request byte limits, per-step omission and the
orchestrator's result-size early stop. A 50-sample run/read workload must complete;
larger outputs must stop with an explicit partial-result reason. Keep aggregate
frame/byte/artifact/deadline limits bounded rather than raising every cap blindly.

Compare saved reference results for batch, input and address formatting. Implement
reference-facing presentation at the MCP adapter: reference `steps`, named buttons
plus mask and context-appropriate address formatting. Preserve internal ownership
UUIDs and lossless addresses. Inventory current consumers before choosing additive
aliases or replacing fields; record every retained intentional difference in the
parity ledger. Avoid global case/sign-extension rewrites of native contracts.

Completion: representative saved-result parser fixtures, 50-sample batching,
256-step boundary and rejection beyond it, explicit output-budget behavior, and
existing interruption/deadline/disconnect ownership regressions.

## 4. Port the reference RDP command export

Feedback 5. Reference implementation:
`../../ares-64/desktop-ui/program/debug-session.cpp`, `rdpCaptureFinish`;
names: `../../ares-64/ares/n64/rdp/render.cpp`, `rdpCommandName`.
Record the reference checkout commit and preserve attribution when porting.

Port its 64-opcode command-length table and output fields: sequence, opcode, name,
address, source, words, batch IDs and frame; incomplete commands include expected
length. Commands may span submission batches. Retain raw words and existing
batch/task metadata, drop counters and capture completeness. Decode after capture;
do not add decoding overhead to submission. Distinguish reference command limits
from this implementation's word-storage cap in names/docs and result metadata.

Audit current artifact writer and viewer decoding before choosing the shared
decoder's location. Preserve exact-capture versus legacy-viewer distinctions.
Compare file envelope fields as well as `commands`, since scripts may use both.

Completion: opcode/length fixtures including triangles, texrects, cross-batch and
incomplete captures; reference-equivalent commands from the same retained word
stream; Kirby `rdp_triangles.py` and `rdp_texrects.py` on new exports when their
fixtures/media are available. Report unavailable private-media checks explicitly.

## 5. Make explicit MCP configuration survive app restarts

Feedback setup and 12. Add an opt-in stable name, for example app launch
`--instance-name kirby` and bridge/CLI selection `--instance-name kirby` within an
explicit service directory. Keep the existing file-plus-instance selection mode.
The name maps atomically to the current private connection record; each launch
still generates a fresh UUID, token and endpoint. Validate names, file ownership,
permissions and symlinks using the endpoint's existing checks.

Reject concurrent live claims of one name. Define stale-claim recovery using an
authenticated liveness probe/claim lock, not just PID existence. Cleanup removes
only the claim still owned by that launch. Never scan for the newest instance.

The bridge resolves the name at startup. On app loss, invalidate outstanding work
and report disconnection; reconnect only with a fresh handshake and identity.
If the bridge remains alive across restart, it may reconnect for subsequent calls
but must never replay an in-flight mutation or reuse old session/epoch state.
Clear adapter-owned operation/region handles. A missing app gets a clear
`instance_not_running`-style error rather than hanging or spawning another app.

Route app SIGTERM/SIGINT through idempotent bounded shutdown, alongside normal
Electron quit. Test actual child signals. SIGKILL cannot guarantee cleanup, so
stale-record detection remains required. Do not confuse MCP bridge cleanup with
the app's ownership of service files.

Completion: one saved MCP configuration across two launches, fresh credentials,
old credentials rejected, duplicate-name failure, stale-file recovery, graceful
signal cleanup and non-replay of a command interrupted by restart. Exercise the
copied package and explicit unnamed multi-instance mode as well as source builds.

## 6. Expose diagnostics and close smaller feedback items

Feedback 7, 10–13, 16 and 17, plus the client-restart setup note.

- Add bounded `ares-plugin logs <id>` with cursor, generation and dropped counts.
  Reuse registry logging and explicit instance targeting; avoid returning only
  the catalog's deliberately empty log arrays. Add read-only targeted storage
  inspection if needed for plugin debugging.
- Add explicit window/panel UI screenshots through shared CLI/agent services,
  distinct from game framebuffer screenshots. Report missing/closed panels clearly.
  Document opt-in CDP for development as a fallback, not a launch requirement.
- Reinspect the current workspace/settings UI before applying the old dock report.
  Put a discoverable position action near the dock when applicable and implement
  selected/expanded keyboard/accessibility semantics for current panel controls.
- Keep immediate pause; document `at_frame_boundary` and exact-capture preparation.
  Add explicit boundary pause only if needed, with cancellation/deadline semantics.
- State that controller timeout is an upper bound: disconnect, takeover and reset
  can release earlier. Keep connection ownership; detached leases are out of scope.
- Reproduce the helper sandbox warning on the current copied package. Fix a proven
  packaging cause if found; do not disable sandboxing or blanket-filter stderr.
- Document that some MCP clients need restart to discover newly registered tools.

Completion: CLI/runtime log and screenshot checks, generation/disconnect behavior,
keyboard/accessibility inspection of current dock controls, updated examples, and
an explicit reproduced/fixed/not-reproduced disposition for the warning.

## 7. Record selected ranges at every frame boundary

Feedback 19. Build a universal native recorder over region ranges, with optional
system-specific address adapters. Define start/read/stop/release contracts shared
by plugins and MCP. Copy at each completed frame boundary into a bounded ring;
do not pause the game or reserve exclusive execution for the recorder's lifetime.

Return recording ID, session/epoch, frame kind, sequence/cursor, frame coordinates,
capacity and dropped/overwritten counts. Separate requested duration from ring
capacity. Bound bytes per sample, total storage, recorder count and read-page size.
Readers fetch immutable paged samples; document handling of a cursor overtaken by
the writer. State load/reset/ROM replacement terminates or invalidates recordings
explicitly; owner disconnect and plugin unload release resources by default.

Completion: consecutive frame samples, live play without pauses, wraparound and
slow readers, overlapping clients, cleanup and epoch changes. Measure copy cost
and renderer transfer volume for the Kirby ~25 KiB sample before making latency
claims. UI graphs use frame gaps/sequence rather than polling wall-clock guesses.

Separately investigate non-stopping call/write event buffers. Determine interpreter
and recompiler coverage, instrumentation cost, overflow and register-capture
semantics before committing to an API or claiming no slowdown. The recorder does
not depend on these hooks.

## 8. Keep snapshots native and query them there

Feedback 20. Build in separate stages:

1. Universal supported-region snapshots with native handles, ranged reads and
   explicit release. Provide immediate-safe-point and next-boundary scheduling;
   paused requests must not accidentally resume play. Apply byte budgets as well
   as handle counts: 32 full 8 MiB snapshots consume 256 MiB. Use recorder scheduling
   for bounded multi-frame captures; a 60-frame full-RAM window needs 480 MiB.
2. Native paged compare/scan queries over immutable snapshots: aligned u8/u16/u32/
   f32, declared endian, ranges, changed/unchanged/increased/decreased/delta/equals,
   then candidate-set narrowing. Specify NaN, signedness, overflow, exact float
   equality and changed-run grouping. Bound query work, results and candidate
   memory; use query handles/cursors for large results rather than blocking pipes.
3. Per-frame page/line change maps. A snapshot comparison reports changed contents;
   a write map reports writes even when values return to the original value.
   Pick the promised semantic explicitly. A true write map must cover CPU, DMA,
   RSP and other writers, and handle state load/reset. Start with correctness before
   selecting instrumentation or comparison as the implementation strategy.

All handles are scoped to session/epoch and owner with deterministic release on
disconnect/unload. Completion includes budget exhaustion, stale handles, immutable
readback, query paging/candidate correctness, all writer paths for write maps, and
measured native cost on representative RAM sizes.

## 9. Build the RAM-diff explorer

Use the universal snapshot/query services for before/after samples and iterative
candidate narrowing. Show frame/sample identity, typed old/new values, adjacent
changed runs and known labels. Allow per-game labels to be saved through plugin
storage and imported/exported with ROM identity and memory-map provenance.

Begin with manual snapshots and filters; then add bounded action/frame recordings
and navigation. Keep large copies and scans native, fetching only visible results.
Use the Kirby memory map as optional game data, not a dependency of the universal
host. Completion: narrow a known moving value, inspect a particle-pool change,
save/reload labels and handle reset, stale results, overflow and empty candidates.

## Evidence and handoff

For each slice update service/plugin/MCP documentation, parity ledger where
applicable, execution log and handoff with actual commands and outcomes. Run
focused fixture/native/real Electron tests for the changed behavior; run the full
acceptance lane at milestones affecting shared runtime, endpoint or packaging.
Use disposable profiles and endpoints and preserve original media/saves.

The feedback's matching RDRAM, screenshots, debug results and 68-tool inventory
are valuable baseline scenarios, not new test passes. Carry forward separately
deferred manual, private-media, fault and hosted-CI checks. Documentation-only
planning requires no rebuild and does not mark any implementation slice complete.
