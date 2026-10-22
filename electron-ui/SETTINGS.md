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

Presentation preferences, the imported BML selection (`paths.importBml`) and
firmware paths for all compiled systems currently use the service. Expand
**Paths** or **Firmware** under **Native settings** to edit a path or choose a
file. Save persists immediately; pending changes apply on the next ROM load.
Reset removes the profile override and restores the imported value or default.
The running game is never restarted by a settings edit.

Firmware descriptors come from the native emulator registry. For example, a
provisioned GBA firmware override is:

```json
{
  "version": 1,
  "values": {
    "firmware.GameBoyAdvance.BIOS.World": "/absolute/path/to/boot.rom"
  }
}
```

Precedence is explicit launch override, profile value, imported BML, then default.
`ARES_SETTINGS` overrides the saved import-file selection for the current launch
without rewriting the profile; explicitly choosing another file saves the new
selection. Native overrides are written to a separate immutable launch snapshot
under `<profile>/runtime-settings/` and removed after host acknowledgement or
exit. Imported BML is never rewritten. Missing/invalid firmware is reported by
the existing loader when a ROM is opened. No firmware is downloaded.

Audio volume/mute and video luminance/saturation/gamma apply live. Native
acknowledgement determines applied state; during controlled execution a change
is saved and reported as pending rather than interrupting or replaying the run.
Save again when idle, or load the next ROM. Speaker mute keeps audio pacing;
debugger PCM capture remains before speaker mute but reflects the volume control.

Supported loader options are available under Core and system sections and apply
on the next ROM load. GPU quality/supersampling controls appear only when the
native host reports GPU support. The old forced boot, quality, supersampling and
interpreter overrides have been removed; fast boot remains the default.

Configurable input and plugin settings registration are still being connected. The
`panelLeft` setting belongs to the temporary UI and is expected to be removed.

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
