# Resume later: trusted shell extensions

Deferred by the user on 2026-10-21. Do not start this automatically.

The minimal video-only UI, Cmd+, settings, split workspace regions, overlays and
hotkey popup panels are implemented; see [WORKSPACE.md](WORKSPACE.md). The remaining
piece of task 14 is allowing a trusted plugin to customize the surrounding shell.

Proposed scope:

- Explicit manifest `shellEntry` and `ui.shell` declaration, with experimental DOM
  or layout hooks. Keep ordinary panels on the existing stable mount API.
- Reuse the core-owned video canvas and native session; shell customization must
  not recreate the video transport or take native session ownership.
- Define focus, shortcut, input-release and cleanup behavior. Integrate with
  generation revocation, hot reload and persisted workspace placements.
- Keep startup recovery and a way to disable a broken extension outside the
  replaceable shell. Same-renderer trusted code is not sandboxed; an infinite
  loop can still freeze its renderer.

Done when a panel, overlay and shell customization coexist, reload repeatedly
without accumulating effects, hidden panels stop polling, and a broken extension
can be disabled/recovered without losing the native session. Measure reload and
hidden-panel costs. Add profiler/asset tools only when a concrete use warrants them.

Start with [roadmap task 14](EXECUTION-ROADMAP.md#task-14--overlays-custom-workspaces-and-trusted-shell-extensions)
and [plugin UI contracts](PLUGIN-PLAN.md#3-ui-contributions-and-customization),
then inspect `plugins/contracts.ts`, `plugins/scope.ts`, `plugins/runtime.ts` and
`workspace/`. No implementation of shell-entry hooks has been claimed complete.
