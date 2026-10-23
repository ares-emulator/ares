# Kirby MCP and plugin feedback triage

Triaged 2026-10-21 against `31f2f5c24`. Source:
`/Users/bschlenk/Developer/kirby/docs/ares-electron-feedback.md`, reporting on
task-12 package `539755659`. This is source inspection, not a runtime reproduction.
Item numbers below refer to that report. No implementation changes are included.

The follow-up [implementation plan](KIRBY-FEEDBACK-PLAN.md) incorporates the user
discussion: atomic ranged reads are universal first, with an N64 CPU-view adapter;
RDP decoding follows the checked-out ares-64 exporter; named MCP discovery retains
fresh credentials per launch. Where sequencing differs, use that plan.

P1 means next implementation work; P2 means follow-up; P3 means deferred or
documentation only. None of the reported findings establishes a P0 blocker.

## Recommended order

1. Fix and test request overload behavior; document thrown versus returned errors.
2. Add atomic ranged reads, satisfying the user's explicit request in item 18.
3. Restore practical MCP script compatibility: batch capacity and decoded RDP export.
4. Add restart-safe explicit endpoint discovery, signal cleanup and plugin diagnostics.
5. Build the native recorder, then snapshot/query tools as separate features.

## Disposition of every finding

| Source | Priority / status | Action and completion evidence |
| --- | --- | --- |
| Setup: changing instance UUID | P2, confirmed design limitation | Add an explicit stable named endpoint/locator while retaining fresh authenticated instance generations. A fixed UUID alone must not let stale clients replay mutations after restart. Test restart, simultaneous-name collision and stale credentials; never choose the newest app implicitly. See `services/local-endpoint.ts` and RESUME.md continuation rules. |
| Setup: MCP client restart | P3, client behavior | Add a troubleshooting note; no host fix required. |
| Inventory; 3, 4, 6, 8, 9 | Positive evidence | Retain as reference acceptance scenarios: 68 tool names, matching values/RDRAM/screenshots/RDP words, debugger behavior and successful plugin authoring. These are reported results from the older package, not fresh validation of HEAD. Trace timing is not a demonstrated regression. |
| 1: batch 32-step limit | P1, confirmed compatibility gap | Target 256 steps. Update MCP schema, service request/result validation and docs together (`mcp/batch-tools.ts`, `services/batch-contracts.ts`). Audit request/result byte caps and the orchestrator's 32 KB early-stop guard: changing only `.max(32)` will not ensure a 100+ step sampling workflow completes. Test the actual 50-sample run/read pattern and explicit partial-result limits. |
| 2: result shapes | P2, confirmed contract divergence | Inventory differences in the parity ledger and add reference-shape fixtures. Prefer compatibility at the MCP adapter: `steps`, button names plus mask, reference address formatting where appropriate. Keep internal owner UUIDs/identity semantics intact. Avoid a broad internal schema rewrite merely for formatting. Decide aliases versus a versioned output contract before changing existing callers. |
| 5: RDP export | P1, confirmed compatibility gap | Add decoded `commands` alongside retained raw words/batches/tasks. Exact capture currently exposes words (`mcp/rdp-capture-tools.ts`); the ledger's exact-capture reference promises full commands. Use a shared decoder with correct variable-length triangle commands and explicit partial/truncated handling. Validate exports with the Kirby triangle/texrect scripts and reference fixtures. Keep truncation counters; the storage cap itself is not reported as an observed failure. |
| 7: pause boundary | P2, behavior needs deliberate contract | Preserve immediate pause for debugger/human stops. Document boundary state and the `run_frames(1)` capture prerequisite; consider an explicit boundary-pause option. Do not silently advance a paused debugging session by default. Test immediate versus boundary pause and exact-capture preconditions. |
| 10: agent observability | P2, confirmed logs omission | `main.ts` deliberately emits `logs: []`. Add bounded, cursor-based `ares-plugin logs <id>` first, with generation and dropped-line counts. Add explicit window/panel screenshots separately; framebuffer screenshot semantics remain useful. Document CDP as an opt-in development fallback. Storage inspection is optional and should be read-only and explicitly targeted. |
| 11: dock discovery / accessibility | P2, recheck current UI | Report describes the older shell; current settings/workspace replaced parts of it. Inspect current dock placement and panel toggle semantics before changing them. Provide accessible selected/expanded state appropriate to each control and an obvious dock-position action. Existing shell tab `aria-selected` does not establish plugin-panel accessibility. |
| 12: stale files | P2, likely lifecycle gap | Main app cleanup is on `before-quit`; MCP bridge signal handlers do not clean the app's service endpoint. Route app SIGTERM/SIGINT through idempotent shutdown and test child exit plus file/socket removal. Translate dead-endpoint errors clearly. Abrupt SIGKILL still requires stale-file tolerance; remove only endpoints proven to belong to the stopped instance. |
| 13: sandbox warning | P3, reproduce before fixing | Capture on a freshly built/copied current package and identify Electron/macOS packaging cause. Do not disable the sandbox or suppress arbitrary stderr to hide it. |
| 14: concurrent reads hang | P1, reported hang not reproduced | Current `plugins/scope.ts` and `plugins/registry.ts` each cap pending requests at eight and throw `busy`; lower layers also have bounds. Overload rejection exists, so do not assume a missing limit is the root cause. A rejected `Promise.all` can leave a plugin's refresh flag latched while other requests finish. Reproduce 24 reads, inspect plugin cleanup and transport completion, and require every accepted/rejected request to settle with pending counts returning to zero. |
| 15, 18: atomic `readMany` | P1, confirmed missing API; merge into one feature | Implement one native safe-point operation for ordered ranges, sharing one epoch/frame/sample identifier. Never implement it as sequential renderer requests. Support at least the supplied ~25 KiB sample; propose a bounded 64 KiB total and bounded range count, subject to transport-size review. Prevalidate all ranges and fail the whole request on invalid input. Distinguish instruction-safe snapshots from optional next-VI snapshots; frame alone cannot distinguish two samples in one frame. Wire contracts, plugin capability allowlists, SDK and docs. Test running/paused reads, overlapping ranges, limits, epoch changes and consistency. |
| 16: thrown `busy` | P1, confirmed documentation mismatch; combine with 14 | Document both failure channels: service replies require `reply.ok`; SDK/IPC admission, stale scope and lifecycle failures can reject. Expose stable codes/limit details and show `try/finally` refresh cleanup. Test SDK overload rejection rather than only native error replies. |
| 17: input ends on disconnect | P2 documentation, retain ownership | Explicitly document that timeout is an upper bound and disconnect releases the override. Add lifecycle information to the result if useful. Do not make leases survive disconnect implicitly: current bounded ownership and takeover guarantees depend on release. A separately requested detached lease would need explicit cancellation and ownership semantics. |
| 19: per-frame recorder | P2, new native feature | Start with bounded ranged VI snapshots into a ring buffer; include epoch/frame/sequence, overflow/drop counts, cursors, lifecycle cancellation and memory limits. Fetch asynchronously without taking execution ownership or pausing live play. Test wraparound, frame continuity, load/reset/reload and competing clients; measure host copy cost. Non-stopping call/write hooks are a separate larger project: assess recompiler instrumentation before promising no interpreter slowdown. |
| 20: RAM snapshot / diff explorer | P3, staged feature project | First bounded native snapshot handles with ranged reads/release, identity/lifetime and byte budgets; then paged typed compare/filter queries and persistent candidate sets; then per-frame change maps. A 32-snapshot 8 MiB pool is 256 MiB; a full 60-frame window is 480 MiB, so count alone is insufficient. Define float/NaN and changed-run semantics. Dirty maps need all write paths (CPU, DMA, RSP and state load), not just CPU stores. Build the explorer after these primitives; the known-range recorder does not require this project first. |

## Scope and validation

The request-overload fix and atomic ranged read are the first coherent slice.
Batch and RDP compatibility can follow independently. Keep each slice reviewable
with focused contract/native tests and its actual plugin or MCP workflow, then run
the existing acceptance lane for runtime changes. Do not claim the source report's
parity measurements as tests of a new implementation.

This triage does not supersede the existing execution roadmap or mark deferred
physical-input, audible-output, fault-injection or soak gates complete.
