# Kirby 64 powers · verified US revision 0

The power selector uses the mappings and addresses already researched in the
Kirby viewer repository:

- `docs/kirby64-ability-ids.md`: IDs 0–35, canonical single/pair order, power/HUD writes.
- `docs/ares-power-cheat-and-tracing.md`: cache-aware virtual addresses, last-drawn
  HUD detector and the controllable-player / palette limitations.
- The seven PNGs are copied unchanged from `gameplay/elements/` in
  `/Users/bschlenk/Developer/kirby`, as requested. Combinations pair the same
  element sprites used by `viewer/components/play-list.tsx`.

The loader requires normalized ROM SHA-256
`2f579751d7ad2824dfd8a6141570306bfaeda1cff40139ba231c30b8591d681c`
and profile `kirby64-us-rev0`. Filenames do not identify a revision.

Each explicit click uses one prevalidated `n64.memory.writeMany` request with
CPU/cache-aware u32 big-endian writes to `0x8012E850` and `0x800D6E90`. It sets
`0x800D6EA8` to the opposite valid ID (1 for None, otherwise 0), so the documented
redraw comparison sees a change even when reapplying a power. Immediate readbacks
verify the batch. No frames, pause/resume or controller overrides are requested.
Use while Kirby is controllable; the game applies attacks on B. Hat/body palette
changes are outside this documented route. A running external operation returns
busy; the tool does not cancel it.

Power writes never run during activation, panel opening or reload. Agent commands
`power-0` through `power-35` invoke the same explicit selection handler; `open`
opens the grid. The host hides these extra power commands from the developer
button list because the grid already presents them all.

Validation includes the exact table/batch unit test and `test:kirby`, which loads
a disposable copy of the verified ROM, clicks every button, checks CPU-view
readbacks and icons, confirms no frame/session/pause changes and restores the
original words. It also checks reload nonreplay and stale command rejection.
An optional compatible state can be supplied via `ARES_PLUGIN_TEST_STATE`; old
v152 files are rejected by the current v153 loader rather than rewritten.
