# Frontend-native runtime support

Extracted from `desktop-ui/` at `b9f42e052`; licensed under the repository ISC
license. See [the retirement inventory](../../DESKTOP-RETIREMENT.md) for feature
coverage and explicit deferrals.

- `emulator/`: compiled-system registry, loaders, firmware, peripheral configuration,
  save/media routing and per-system input nodes.
- `input/`: virtual ports and inherited/direct mappings, without desktop hotkeys,
  assignment windows or icon helpers.
- `settings/`: imported BML data and binding process, without desktop windows or
  implicit writing/import migration.
- `runtime.*`: cross-thread input mutex/capture flags, explicit companion-media
  queue and palette application. Core mutation/loading stays on the native host's
  emulation thread. The old desktop event loop and its `Program` global are gone.

This code includes no Hiro headers, timers, widgets or desktop resource compiler.
The host selects MIA resources explicitly; built-in core/MIA resources remain in
those libraries. Preserve the actual loader behavior when adding settings APIs.
