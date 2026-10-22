# Profile settings (task 13 in progress)

The main process owns `<profile>/preferences.json`. You can provision this file
before launching Ares Workbench:

```json
{
  "version": 1,
  "values": {
    "presentation.theme": "green",
    "presentation.scale": "integer",
    "presentation.scanlines": false,
    "presentation.panelLeft": false
  }
}
```

Currently these four presentation settings use the service. Firmware, audio,
input and core settings are still being connected; their presence in this file
alone does not configure the native host yet. The `panelLeft` setting belongs to
the temporary UI and is expected to be removed.

Settings are profile scoped. Defaults, types, validation, UI grouping and apply
timing are defined in `settings/builtins.ts`. Stable IDs are independent of labels
and UI sections. Moving or relabeling a setting requires no file migration.
Removing a descriptor makes its saved value inactive; it remains in the file so
older/newer configurations remain usable. Unknown keys are preserved on writes.
Changing an ID or the meaning/format of its value requires an explicit migration.

Known invalid values use the descriptor default and produce a diagnostic. The
original value stays intact until explicitly changed. Malformed, oversized and
unsupported-version files are reported and never overwritten by UI changes;
repair the file and restart. Writes use a sibling temporary file and atomic
rename. The application reads provisioning at startup; live external file
watching is not implemented. Edit while the profile is closed.

The sandbox bridge exposes `preferences.catalog()`, `set(id, value)` and
`changed(callback)`. Entries distinguish saved `value` from `applied`, with
`pending` and the descriptor's `apply` timing. Failed application keeps the saved
value and reports the error. Native settings will explicitly acknowledge their
application through this service. Plugin access/registration is still pending.

Validation: `npm run test:settings` exercises real Electron controls and reload;
`tests/settings.test.cjs` covers persistence across reconstructed stores, unknown
keys, descriptor removal/readdition, validation, malformed files and failed or
deferred application.
