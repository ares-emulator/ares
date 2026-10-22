# Fresh-agent handoff

Updated 2026-10-21. Read this before building, testing or changing runtime files.

## Current objective

**Foundation work is complete. Resume task 13:** real validated/persisted settings,
configurable input and complete author SDK declarations. Tasks 1–12 (including
4.5) remain implemented. No further triage approval is needed.

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

Latest local acceptance: **15/15 gates, 89 fixture tests, no failures or skips**;
actual Electron/MCP/plugins/recovery, profile/crash/rebuild isolation and copied
package passed. GitHub-hosted CI has not run yet; do not infer a hosted pass from
the workflow file. Private-ROM/manual/fault checks remain separate, not relabelled.

The approved promotion/foundation triage is complete. Full original findings and
remaining scoped work are in [TECHNICAL-DEBT.md](TECHNICAL-DEBT.md). Task 13 should
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
work. Foundation is complete; implement task 13 next. Keep the explicitly
deferred manual/fault checks visible and do not add unrelated prerequisites.


Task 13 checkpoint (2026-10-21): profile settings foundation implemented in
`settings/`; presentation controls use profile `preferences.json` instead of
localStorage. The BML file selection also persists (`paths.importBml`) and is
acknowledged against the exact startup snapshot after native host status succeeds.
Descriptors define stable IDs, types/defaults/validation/group/apply timing.
Unknown keys survive descriptor removal; malformed files are never overwritten.
See SETTINGS.md. Three strict TS targets, five settings tests and isolated actual
Electron provisioning/write/reload/native-start acknowledgement passed. Full
acceptance has not been rerun for this checkpoint. Next: connect firmware and
native audio/presentation/core descriptors, configurable input, plugin settings
registration/bundled UI and complete generated SDK declarations. Task 13 remains
in progress; its done gates are not yet fulfilled.


Task 13 firmware checkpoint: `settings/native.ts` maps registry-discovered firmware
requirements to descriptors and writes launch-only BML overlays. `settings/form.ts`
provides reusable descriptor-driven controls currently mounted in the temporary
sidebar. Paths/Firmware have file pickers, Save/Reset, import inheritance and
next-ROM pending state. Native discovery/overlay support lives in host.cpp and
native/settings-service.hpp; firmware BML key helper shared with loader. All 36
compiled requirements discovered. Original GBA fixture moved to tests/fixtures/gba.cjs.
Host build, three TS targets, nine targeted tests, Electron settings/GBA and plugin
recovery checks passed. Next connect live audio/palette and supported core options,
then input and plugin SDK/settings work. Task 13 remains in progress.


Task 13 live/core checkpoint: runtime.settings/runtime.configure added to native
and TS protocols without adding MCP tools. Volume/mute/palette live controls use
native acknowledgement and busy/nonreplay protections. Next-ROM core descriptors
come from settings/native.ts; GPU-only controls depend on host features. Old four
hardcoded host overrides removed; defaults fastBoot true, volume .4. Fourteen
targeted tests plus existing audio regression, actual Electron settings and MCP
checks passed. Next: configurable input, scoped plugin settings registration and
bundled settings plugin, complete generated SDK declarations, final acceptance.
