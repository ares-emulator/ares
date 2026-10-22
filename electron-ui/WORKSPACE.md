# Workspace

A fresh profile opens with only the video. Cmd+O opens a ROM. Cmd+, opens
Settings, including shared application and plugin settings and plugin management.
Cmd+Shift+L toggles workspace editing; Escape exits it. Game input is released
while editing or using Settings.

Each region has a panel chooser and controls to split side by side or top/bottom,
close the region, or add its panel as an overlay or popup. Drag a divider to resize
adjacent regions. There is one video canvas: choosing Video in another region
moves it there without replacing the emulator session. View → Video Only resets
the layout.

Settings → Workspace adds overlays or popup panels and configures their visibility,
mouse interaction and hotkeys. Drag a floating panel's title to move it and its
lower corner to resize it. Overlay headers appear while hovering or editing.
Turn off interaction to let mouse input pass through; editing makes the controls
accessible again. Popups can be hidden and reopened with their hotkey or settings.
Use Mod (Command on macOS, Control elsewhere) or Alt, optionally Shift, with a
letter, digit or F1–F12. Application/edit menu bindings are reserved.

Placements are independent panel instances. Hiding a popup retains its local
state and suspends scoped polling; removing it cleans up the instance. Disabled
or missing plugins leave a placeholder, preserving the placement for re-enabling.
Plugins do not automatically add panels during activation. Explicit panel-opening
commands reuse an existing placement or create a popup.

The main process atomically persists `<profile>/workspace.json`, independent of
`preferences.json`. It is readable and can be provisioned before launch:

```json
{
  "version": 1,
  "root": {
    "id": "main",
    "axis": "x",
    "ratio": 0.7,
    "children": [
      {"id": "game", "panel": "video"},
      {"id": "inspector", "panel": "plugin:ares.examples.memory:memory"}
    ]
  },
  "floating": []
}
```

Use the panel IDs declared by the plugin manifest. IDs for regions and floating
windows must be unique. Limits are 16 leaf regions, eight nesting levels, eight
floating windows and a 64 KiB file. Split ratios range from 0.1 to 0.9. A malformed
file is reported and preserved until manually repaired and the app restarted.
Unknown plugin references are valid and remain saved.
