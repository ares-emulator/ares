# Kirby feedback implementation handoff — 2026-10-22

The implementation now covers the major services and explorer workflows in all
nine slices. This is still **not a claim that every original completion check has
passed**. The original untracked plan/triage and the pre-existing roadmap edit
were preserved. Tests use disposable media, profiles and service directories.

## Implemented

1. Shared eight-request plugin admission. Renderer and host admission errors keep
   code/pending/limit through explicit IPC data, reconstructed into SDK errors in
   the renderer (Electron otherwise drops Error properties). Native failures stay
   resolved ok:false. Real Electron checks 24 settled promises, host saturation,
   zero pending bookkeeping, pending unload and a working replacement generation.
   Endpoint request saturation uses request_overload separately from execution busy.
2. Atomic ordered/overlapping region and N64 CPU-view readMany: 64 ranges/64 KiB,
   shared SDK/MCP/plugin validation, one native safe point, epoch/frame/sample IDs.
   Audited debugger read callbacks. Host RDRAM regions bypass chip degradation and
   guest RNG by reading raw physical storage; CPU-view atomic reads reject before
   chip mapping is initialized. N64/GBA live and initialized N64 CPU-view fixtures
   cover large reads and full 64 KiB replies.
3. 256-step batches and a measured, bounded 128 KiB result budget. Real 50-sample
   run/read workload completes; output exhaustion stops before later mutation.
   MCP keeps results plus reference steps, and named controller buttons plus mask.
4. Shared reference exact RDP decoder, frozen ROM/renderer/start/end envelope,
   OSTask decoding and latest-graphics-task batch associations, preserving raw
   fields and metadata aliases. Commit/attribution retained. Native live fixture
   verifies immutable completion points after subsequent guest execution.
5. Explicit named discovery using kernel claim locks and atomic private records.
   Source and copied apps passed actual SIGTERM/SIGINT cleanup, SIGKILL stale
   recovery, duplicate names, fresh credentials, old-token rejection and one MCP
   process reconnecting without replay. Inconclusive authenticated liveness fails
   closed rather than displacing a potentially live app.
6. Paged plugin logs/generation/drop counts, shared window/panel screenshots,
   settings tab keyboard/ARIA selection and floating expanded-state semantics.
   Copied-package stderr is retained. The macOS sandbox-extension warning was
   reproduced (see disposition below); no sandbox disable or stderr suppression.
7. Bounded native region rings, independent duration/capacity, per-sequence frame
   identity, drops/cursors and deterministic owner/epoch cleanup. N64/synthetic VI
   and non-N64 first-screen boundaries are labelled distinctly. Overlapping GBA
   owners pass. Native copy and fetched-byte metrics are exposed.
8. Native snapshots, freeze-from-ring, typed paged comparisons, incremental native
   candidate sets and narrowing, aligned ranges, grouped changed runs and 16-byte/
   4 KiB content maps. Queries examine at most 64 KiB per advance, return up to 256
   visible values, reserve 1 MiB each (eight globally), and explicitly fail above
   262144 candidates. Captures share a separate 64 MiB byte budget. Failed partial
   candidates are unusable. Typed/NaN/endian/delta, page continuation, empty results,
   byte/query overflow, stale/foreign handles and owner/epoch release are covered.
9. Explorer manual captures, native search/narrowing, typed and grouped content
   views, bounded frame recording, freeze/navigation by sequence, overwrite/stale
   messages, game/map-provenance labels and import/export. Actual Electron checks
   a known changing value, adjacent changed bytes, retained candidates, frame
   navigation and saved labels after plugin reload. Large data stays native.

## Evidence and measured limits

- Native/TypeScript/SDK builds passed.
- Final full lane: 18/18 acceptance gates, 119 fixture tests, no failures/skips,
  artifacts/acceptance/run-Rx5bZL/. This includes the IPC error fix and pending
  unload/replacement-generation regression. A subsequent small explorer abort-
  cleanup DOM guard passed its focused Electron regression and screenshot review.
- Source and copied-package signal/auth/nonreplay tests passed. Inventory is 85
  tools: all 68 reference names plus 17 explicit memory/diagnostic additions.
- A 25600-byte frame sample measured 45.958 microseconds maximum native copy
  on GBA and 44.875 microseconds on N64 in the final acceptance fixture run.
  Recorder transferred_bytes was zero until a reader requested a page. These are
  fixture timings, excluding renderer/IPC latency, not Kirby or zero-slowdown claims.
- MEMORY-SAMPLING-AUDIT.md records callback semantics and the interpreter/recompiler
  investigation for non-stopping call/write events. No event API was invented.

## Warning disposition

Reproduced on the copied package: sandbox_extension_issue_file_to_process failed
for its temporary Ares.app bundle with Operation not permitted. A development
Electron run also emitted a related missing Helper Resources extension warning.
The copied-package runner now saves artifacts/copied-package-stderr.log and reports
helperSandboxWarnings in package-mcp.json. No proven packaging cause was established;
no sandbox disabling, blanket stderr filtering or speculative packaging patch was
introduced. Package operation and sandboxed renderer checks still passed.

## Still unclaimed / follow-up

- Private Kirby ROM/memory-map and particle-pool acceptance, rdp_triangles.py and
  rdp_texrects.py, saved private reference-result parser fixtures and cross-binary
  comparison when their fixtures/media are available. Universal services do not
  depend on those assets.
- Exhaustive live-running/stale-epoch/fault/deadline/cancel matrices on every core,
  all memory sizes, GPU/external-device synchronization and broader performance/
  soak/platform work. Content maps deliberately do not promise write-event coverage.
- MCP address strings retain context-specific existing presentation and lossless
  native values. Exact reference presentation across every result shape remains
  an intentional, documented compatibility limitation, not blanket parity.
- Physical keyboard/gamepad and audible output, hosted CI, and previously deferred
  serializer rollback-failure injection. Current workspace replaced the old dock;
  no obsolete dock-position setting or detached input lease was added.
