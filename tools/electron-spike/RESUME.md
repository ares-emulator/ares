# Fresh-agent handoff

Updated 2026-10-21. Read this before building, testing or changing runtime files.

## Current objective

Tasks 1–12 (including 4.5) are implemented. Task 12 landed in `539755659`.
**Task 13 is on hold at the user's request.** The current work is auditing the
validated Electron frontend's temporary decisions and triaging them with the user.
Read [TECHNICAL-DEBT.md](TECHNICAL-DEBT.md); its proposals are not approval to
implement every item. The user explicitly does not require preserving existing development data: do not
build a data migration/compatibility layer. Desktop UI is superseded as the intended
product, but its loader/config/input/resource code is still compiled and used by
the Electron host; extract those live dependencies before deleting it.

The user wants a permanent home instead of
`tools/electron-spike`; the destination and migration scope remain to be agreed.

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

## Preserve the user's play session

The user is playing Kirby in an independently launched interactive instance and
confirmed the power selector works. Do not use it for tests, pause/reset/reload it,
replace its host, or modify watched bundled plugin files while it is active.
Do not assume a recorded PID is still valid or kill a process by that PID alone.

The ignored play directory is
`tools/electron-spike/artifacts/kirby-play-20261020-211352`; `/tmp/ares-kirby-play-path`
also records it. `session.json` and `play.log` describe that launch. It uses copied
ROM/EEPROM and isolated profile/saves. These files are local conveniences, not
required repository inputs. Do not commit them or expose connection tokens.

The checkout's ignored `dist` can still be in use by that app. `npm start`,
`npm test`, most Electron test scripts and packaging run a clean build which removes
`dist`. Do not run them here during play. Documentation/Git operations are safe.
Validate runtime changes in a separate checkout/snapshot with its own output,
profile, saves and endpoint. Never borrow live saves as writable test fixtures.
Use `ARES_BACKGROUND_TEST=1` for hidden test windows.

## Reproduction and evidence

Build instructions are in README; native targets are `ares-electron-host` and
`ares-electron-cpu-fixture`, enabled by `ARES_BUILD_ELECTRON_SPIKE=ON`. Current
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
work. For this detour, first present/agree triage; do not silently resume task 13 or
turn every audit observation into an implementation requirement.
