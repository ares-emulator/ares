# Fresh-agent handoff

Updated 2026-10-21. Read this before building, testing or changing runtime files.

## Current objective

**Foundation and task 13 are complete. The requested minimal UI/workspace is implemented.**
See [WORKSPACE.md](WORKSPACE.md) for video-only startup, Cmd+, settings, split
regions, overlays and hotkey popups. The broader task 14 trusted shell-extension
work is deferred at the user’s request; resume from
[SHELL-EXTENSIONS-LATER.md](SHELL-EXTENSIONS-LATER.md).
Tasks 1–13 (including 4.5) are implemented. No further triage approval is needed.

Completed foundation:

1. Permanent `electron-ui/` home and Ares Workbench identity; shared package/bundle
   version 0.1.0 and `ARES_BUILD_ELECTRON_UI` build option.
2. Explicit owned profiles and development runtime snapshots that survive checkout
   builds. One app writer per canonical profile; saves/system data stay isolated.
3. Extracted native loaders/config/input, retired desktop-ui after
   [feature inventory](DESKTOP-RETIREMENT.md), and removed Electron's Hiro dependency.
   Core/Ruby/native-service implementations and compiled system support remain.
4. `npm run accept` builds/runs the complete fixture lane; macOS CI is configured.
   `--disable-plugins` recovery plus targeted CLI disable/reset is implemented.
   Manual/fault exclusions have explicit [dispositions](ACCEPTANCE.md).

Latest local acceptance: **18/18 gates, 109 fixture tests, no failures or skips**;
actual Electron/MCP/plugins/recovery, profile/crash/rebuild isolation and copied
package passed. GitHub-hosted CI has not run yet; do not infer a hosted pass from
the workflow file. Private-ROM/manual/fault checks remain separate, not relabelled.

The approved promotion/foundation triage is complete. Full original findings and
remaining scoped work are in [TECHNICAL-DEBT.md](TECHNICAL-DEBT.md). Further work should
build on the extracted runtime and owned profile configuration; do not restore a
desktop frontend dependency or turn deferred audit findings into prerequisites.

The user explicitly does **not** require preserving existing development data.
Do not build migration/compatibility machinery for old profiles, settings or saves.
The user subsequently waived protection of the old Kirby session; see below.

Deferred: public-release packaging/notarization/updates, Windows/Linux, richer
plugin commands/media matching, broad protocol/module rewrites, optimizations
without measurements, multiplayer/rumble and wider peripheral support. Preserve
intentional contracts: TS/ESM with sandbox CJS preload, bounded captures, explicit
instance selection, trusted plugins and copied video with backpressure.

Continue autonomously through the approved scope with sensible intermediate
commits. Update this handoff, roadmap and evidence as work lands, and preserve a
clear new entry point when moving this file. The documentation/audit baseline is
`2d4c27dfb`; task 12 implementation is `539755659`.

## Read order and sources of truth

1. This handoff: immediate objective and operational constraints.
2. [TECHNICAL-DEBT.md](TECHNICAL-DEBT.md): current audit and suggested triage.
3. [EXECUTION-ROADMAP.md](EXECUTION-ROADMAP.md): numbered task scope/dependencies.
4. The latest entry of [EXECUTION-LOG.md](EXECUTION-LOG.md): what actually passed.
5. [README.md](README.md), [plugins/README.md](plugins/README.md),
   [services/README.md](services/README.md), [mcp/README.md](mcp/README.md): commands/contracts.
6. [MCP-PARITY-LEDGER.md](MCP-PARITY-LEDGER.md), [MCP-PORT-PLAN.md](MCP-PORT-PLAN.md)
   and [PLUGIN-PLAN.md](PLUGIN-PLAN.md) for compatibility and design details.

The execution log is chronological: old entries saying “pending” may be superseded.
Do not infer a new pass from task completion or a registered tool name.

## Validation isolation (updated 2026-10-21)

The user waived protection of the prior Kirby play session and authorized builds
in this checkout. Continue using disposable test profiles, saves and endpoints;
never borrow original saves as writable fixtures. Use `ARES_BACKGROUND_TEST=1`
for hidden Electron test windows. `npm start` builds its own runtime snapshot and copies the native host. Tests and
packaging still clean checkout `dist`; keep direct `electron .` launches out of
concurrent validation. Development defaults to a separate `development` profile.

## Reproduction and evidence

Run `npm run accept` from `electron-ui/` for the fixture lane; prerequisites,
log locations and exclusions are in ACCEPTANCE.md. Build instructions are in
README; native targets are `ares-electron-host` and
`ares-electron-cpu-fixture`, enabled by `ARES_BUILD_ELECTRON_UI=ON`. Current
default native output is `build-electron-make/bin`; `ARES_HOST` and
`ARES_CPU_FIXTURE` override it. JS build does not rebuild C++.

Task 12 validation used `/tmp/ares-task12-APJFCt/tools/electron-spike` (also recorded
in `/tmp/ares-task12-validation-path`). That directory is disposable and may vanish;
it is not a dependency. Its native build was shared read-only; no native changes
were made for task 12. The checkout's `dist` was intentionally left alone.

Recorded task 12 evidence:

- Three strict TypeScript targets; sequential 84/84 test suite, no skips with the
  verified ROM supplied; additional CLI paging regression passed separately.
- Actual Electron session/plugin/reload/MCP/tool-edit/Kirby checks.
- All 36 selector memory batches/readbacks and icons; user confirmed interactive play.
- Copied macOS arm64 app: 68 MCP tools; bundled CLI scaffold/build/list/add/enable/
  open/reload/command; TSX hot reload preserves session/epoch; checkout access denied.
- Ignored packaged app/result: `artifacts/task-12-complete/Ares.app` and
  `artifacts/task-12-complete/package-mcp.json`. Rebuild if these are absent.

Optional game tests require user-supplied media. Kirby identity, researched addresses,
icon provenance and limitations are in `bundled-plugins/kirby/README.md`.
Reference audit source is ares-64 commit `1c9b9637476a83f7b80d3db1115a0f71529820d9`;
Python/TCP are reference material, not runtime dependencies. Original ROMs/saves
and reference checkouts must remain untouched.

Still pending: physical keyboard/gamepad takeover acceptance, audible-output
acceptance, task 8 serializer rollback-failure fault injection, and broader soak/
latency/platform coverage. Kirby play does not close those separately defined gates.

## Continuation rules

Use explicit connection file **and** instance ID for CLI/MCP; never auto-select the
user's app. The tool inventory is **68**, not the earlier 36. Preserve generation,
session/epoch, bounded ownership, immutable stops and nonreplay on reload.
The CJS preload is required by Electron's sandbox; application sources use TS/ESM.
Plugins are trusted same-renderer code, not sandboxed extensions.

User authorizes sensible intermediate commits and autonomous completion of agreed
work. Foundation and task 13 are complete; task 14 is next. Keep the explicitly
deferred manual/fault checks visible and do not add unrelated prerequisites.



## Task 13 implementation and final evidence

- `settings/store.ts` owns profile `preferences.json`: validated stable IDs,
  atomic bounded writes, default/import/profile/launch precedence, saved/applied
  state, Reset and unknown-key preservation. No migration of old development data.
- Descriptors: `settings/builtins.ts` (presentation/audio/palette), `native.ts`
  (registry-discovered firmware/core options/BML launch overlays), `input.ts`
  (virtual-port-1 keyboard/gamepad mapping). Plugin dock position also moved out
  of localStorage. Imported BML stays read-only; old forced native overrides gone.
- `settings/form.ts` provides a shared form/file picker/key recorder. Recovery UI
  uses the host bridge; bundled `ares.tools.settings` uses the same scoped SDK as
  local plugins. `settings/plugins.ts` stages declared, namespaced plugin fields,
  commits on activation and revokes metadata/writers with the lease, retaining
  saved values. Capabilities: settings.read/write/register.
- Runtime settings apply between native runs, respect identity/execution/state
  ownership, and never replay pending changes on unrelated edits. Native tests
  verify GB pixels/PCM and N64 interpreter/Expansion Pak behavior; actual Electron
  tests cover GBA firmware, persistence, key capture/clear, plugin lifecycle and
  reload. Physical hardware/audible acceptance remains separately deferred.
- Complete browser SDK declaration graph generated by scripts/generate-sdk.cjs
  during build and shipped with the template/CLI scaffold. Template supports
  npm run typecheck. Generation output is ignored, regenerated on build. Launcher
  snapshots now include settings/ and scripts/; copied app/CLI remain independent.
- Fixed reload ordering races: no intermediate lease while awaiting owner release,
  no duplicate replacement activation from an older catalog, and UI session
  actions obtain a missing identity during renderer synchronization without replay.

Final command: `ARES_ACCEPT_BUILD_DIR=build-electron-make npm run accept` from
`electron-ui/`: **17/17 gates, 107 fixture tests, no failures/skips**. Evidence:
`artifacts/acceptance/run-606q3R/` and latest.json. Copied package passed all 68 MCP
tools and CLI/TSX reload with checkout read denied (873 GB frames); Electron44.9.0,
bundled Node24.22.0, macOS arm64. Temporary bundle removed by the lane. Hosted CI
was not run. Initial run-FpEKbA exposed the paused-reload identity issue; fixed and
rerun fully, not claimed as a pass. See SETTINGS.md and plugins/README.md for APIs.
