# Fresh-agent handoff

Updated 2026-10-21. Read this before building, testing or changing runtime files.

## Current objective

Promotion is in progress: sources have moved to `electron-ui/`, the build option
is `ARES_BUILD_ELECTRON_UI`, and the product is Ares Workbench. Package version
0.1.0 is the shared source/bundle version. Profile ownership and independent
development launch snapshots are implemented; native desktop extraction is next.

Tasks 1–12 (including 4.5) are implemented. Task 12 landed in `539755659`.
**The user approved the promotion/cleanup triage on 2026-10-21.** Task 13 stays
on hold until the foundation work below is complete. Begin implementation; no
further triage approval is needed. Full findings and acceptance criteria are in
[TECHNICAL-DEBT.md](TECHNICAL-DEBT.md).

Approved order:

1. Promote the former `tools/electron-spike/` to **`electron-ui/`**. Normalize product/build
   names, versioning and data locations; remove spike branding.
2. Make development independent of active play: separate build outputs/test
   profiles and define per-profile ownership so instances cannot overwrite each
   other's data.
3. Extract live loader/firmware/save/config/input/resource functionality from
   `desktop-ui/`; inventory remaining desktop features, remove Electron's desktop
   and Hiro dependencies, and retire the superseded frontend after that inventory.
   Git history can preserve the reference. Preserve working native services,
   compiled system support and emulator cores; this is not a core rewrite.
4. Establish one repeatable acceptance command, macOS CI, startup recovery with
   plugins disabled, and explicit dispositions for outstanding manual/fault checks.
5. Resume task 13: real settings, configurable input and complete plugin SDK types.

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

Build instructions are in README; native targets are `ares-electron-host` and
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
work. Triage is approved in the order above. Finish that foundation before task 13;
do not turn deferred audit observations into additional prerequisites.
