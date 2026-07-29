# Network Control Interface (NCI)

ares includes a UDP-based Network Control Interface (NCI) that allows external tools, scripts, and automation to control the emulator remotely. The protocol is compatible with RetroArch's UDP command interface (port 55355).

## Enabling NCI

NCI is disabled by default. Enable it via:

- **Settings UI**: Configuration > NCI > Enable
- **CLI**: `--setting NCI/Enabled=true`
- **settings.bml**: Set `NCI/Enabled` to `true`

### Configuration Options

| Setting | Default | Description |
|---------|---------|-------------|
| `NCI/Enabled` | `false` | Enable/disable the NCI server |
| `NCI/Port` | `55355` | UDP port to listen on |
| `NCI/UseIPv4` | `false` | Bind to all interfaces (IPv4) vs localhost only (IPv6) |

## Sending Commands

### Using netcat

```bash
echo -n "VERSION" | nc -u -w1 localhost 55355
echo -n "GET_STATUS" | nc -u -w1 localhost 55355
echo -n "PAUSE_TOGGLE" | nc -u -w1 localhost 55355
echo -n "READ_CORE_MEMORY 0000 10" | nc -u -w1 localhost 55355
echo -n "SHOW_MSG Hello from NCI" | nc -u -w1 localhost 55355
```

### Using the --command flag

A second ares instance can send commands to a running instance:

```bash
ares --command "VERSION"
ares --command "GET_STATUS"
ares --command "PAUSE_TOGGLE;192.168.1.5"
ares --command "GET_STATUS;localhost;55356"
```

Format: `COMMAND[;HOST[;PORT]]` (defaults: localhost, 55355)

## Command Reference

### System Commands

| Command | Response | Description |
|---------|----------|-------------|
| `VERSION` | Version string | Returns ares version |
| `GET_STATUS` | `GET_STATUS PLAYING\|PAUSED system,game` or `GET_STATUS CONTENTLESS` | Current emulator status |
| `GET_CONFIG_PARAM <path>` | `GET_CONFIG_PARAM <path> <value>` | Query config value by BML path |
| `SET_CONFIG_PARAM <path> <value>` | `SET_CONFIG_PARAM <path> <value>` | Set config value by BML path |
| `QUIT` | `QUIT OK` | Exit ares |
| `CLOSE_CONTENT` | `CLOSE_CONTENT OK` | Unload current game |
| `LOAD_CONTENT <path>` | `LOAD_CONTENT OK` | Load ROM from file path (auto-detect system) |
| `LOAD_CONTENT <system> <path>` | `LOAD_CONTENT OK` | Load ROM with explicit system name |
| `RESET` | `RESET OK` | Reset current system |

`GET_CONFIG_PARAM` and `SET_CONFIG_PARAM` accept any BML settings path using forward-slash notation (e.g., `Audio/Volume`, `NCI/Port`, `Paths/Saves`). These are the same paths used by the `--setting` CLI flag and `settings.bml`. Unknown paths return `UNSUPPORTED`. `SET_CONFIG_PARAM` writes the value to the BML tree, syncs it to the running configuration, and persists it to disk.

### Playback Control

| Command | Response | Description |
|---------|----------|-------------|
| `PAUSE_TOGGLE` | `PAUSE_TOGGLE PAUSED\|UNPAUSED` | Toggle pause |
| `FRAMEADVANCE` | `FRAMEADVANCE OK` | Advance one frame (pauses first) |
| `FAST_FORWARD` | `FAST_FORWARD ON\|OFF` | Toggle fast forward |
| `REWIND` | `REWIND ON\|OFF` | Toggle rewind (must be enabled in settings) |

### State Management

| Command | Response | Description |
|---------|----------|-------------|
| `SAVE_STATE` | `SAVE_STATE <slot>` | Save state to current slot |
| `LOAD_STATE` | `LOAD_STATE <slot>` | Load state from current slot |
| `LOAD_STATE_SLOT <n>` | `LOAD_STATE_SLOT <n>` | Load state from specific slot (1-9) |
| `STATE_SLOT_PLUS` | `STATE_SLOT_PLUS <slot>` | Increment state slot |
| `STATE_SLOT_MINUS` | `STATE_SLOT_MINUS <slot>` | Decrement state slot |

### Audio

| Command | Response | Description |
|---------|----------|-------------|
| `MUTE` | `MUTE ON\|OFF` | Toggle mute |
| `VOLUME_UP` | `VOLUME_UP <level>` | Increase volume by 0.1 (max 2.0) |
| `VOLUME_DOWN` | `VOLUME_DOWN <level>` | Decrease volume by 0.1 (min 0.0) |

### Display

| Command | Response | Description |
|---------|----------|-------------|
| `FULLSCREEN_TOGGLE` | `FULLSCREEN_TOGGLE OK` | Toggle fullscreen |
| `SCREENSHOT` | `SCREENSHOT OK` | Capture screenshot |
| `SET_SHADER <path>` | `SET_SHADER <path>` | Load shader preset |
| `SHOW_MSG <text>` | `SHOW_MSG OK` | Display OSD message |

### Memory Access

| Command | Response | Description |
|---------|----------|-------------|
| `READ_CORE_MEMORY <addr> <len>` | `READ_CORE_MEMORY <addr> <hex bytes>` | Read memory (hex address, max 256 bytes) |
| `WRITE_CORE_MEMORY <addr> <bytes...>` | `WRITE_CORE_MEMORY <addr> <count>` | Write memory (hex address and byte values) |

#### Address interpretation

A plain hex address is interpreted through the emulated system's memory map when one is known, so clients can use the same addresses the running program does. For the Nintendo 64, KSEG0/KSEG1 addresses (`0x80000000`-`0xBFFFFFFF`) are translated to physical, and physical ranges route to the matching debugger memory node:

| Physical range | Node |
|----------------|------|
| `0x00000000`-`0x03EFFFFF` | RDRAM |
| `0x04000000`-`0x04000FFF` | RSP DMEM |
| `0x04001000`-`0x04001FFF` | RSP IMEM |
| `0x08000000`-`0x0FFFFFFF` | Cartridge SRAM |
| `0x10000000`-`0x1FBFFFFF` | Cartridge ROM |
| `0x1FC007C0`-`0x1FC007FF` | PIF RAM |

Unmapped addresses return `-1`. For systems without a memory map, the address is a raw offset into the first debugger memory node.

#### Explicit node targeting

Any debugger memory node can be addressed directly with a `<node>:<hex offset>` address token, which bypasses the memory map. Node names are matched case-insensitively with spaces and underscores ignored, so `Cartridge_ROM:0`, `cartridgerom:0` and `RSP_DMEM:100` are all valid. This reaches nodes with no bus mapping, such as Cartridge EEPROM or DCache.

```bash
echo -n "READ_CORE_MEMORY 80000318 4" | nc -u -w1 localhost 55355
echo -n "READ_CORE_MEMORY Cartridge_ROM:0 16" | nc -u -w1 localhost 55355
echo -n "WRITE_CORE_MEMORY Cartridge_EEPROM:0 FF FF" | nc -u -w1 localhost 55355
```

### Disc Management

| Command | Response | Description |
|---------|----------|-------------|
| `DISK_EJECT_TOGGLE` | `DISK_EJECT_TOGGLE EJECTED\|CLOSED` | Toggle disc tray |
| `DISK_LOAD <path>` | `DISK_LOAD OK <path>` | Load disc image from file path |

## Error Responses

Commands return `-1` on failure. For example:
- `READ_CORE_MEMORY -1` — no content loaded or invalid parameters
- `SAVE_STATE -1` — save failed
- `UNKNOWN_COMMAND <name>` — unrecognized command

## Thread Safety

NCI runs on the emulator thread (same as the GDB debug server). Most commands execute directly since the emulator thread owns the state. Commands requiring UI thread access (fullscreen toggle, quit, unload) use atomic request flags that are processed on the main thread.
