# Plan: an agent-extensible emulation workbench

Implementation order and task-sized kickoff guidance: [EXECUTION-ROADMAP.md](EXECUTION-ROADMAP.md).

Status: proposed implementation plan, building on the native Electron spike.
The APIs and manifest below are design examples, not implemented contracts.

Follow-up audit: [MCP-PORT-PLAN.md](MCP-PORT-PLAN.md) inventories the ares-64 fork
and the existing 68-tool MCP bridge. Its migration sequence updates this plan:
preserve MCP behavior early through a TypeScript adapter and share native services
and the operation scheduler with plugins. The Python bridge is reference material,
not a runtime dependency. Do not rebuild debugging features independently for UI.

## Goal and first useful outcome

Keep native ares emulation running while a developer or agent adds, edits, and
reloads web-based tools. Tools should help discover game structures and inspect
actors, models, animations, and effects, then turn those discoveries into reusable
game-specific interfaces. Loading a new UI must not require rebuilding C++ or
restarting the game.

The first complete workflow is: load Kirby 64, discover memory regions, inspect
and edit a known value, create a compatible plugin panel, and change its code
while the game continues. An animation picker follows once the relevant game
structures are understood; discovering those addresses is research, not something
the plugin architecture supplies automatically.

## Decisions to start with

- Expose ares's existing debugger memory regions through a small typed SDK.
  Do not introduce a second memory map for every system.
- Support panels, viewport overlays, commands, settings sections, and full
  workspace pages. Plugins can supply arbitrary web UI inside their surfaces.
- Also support an explicit trusted shell extension entry for broader DOM/layout
  customization. A plugin can use both registered surfaces and shell code.
- Treat locally installed plugins as trusted code in the first version. Scoped
  APIs and lifecycle ownership prevent mistakes; they are not a security sandbox.
- Use the same plugin mechanism for bundled features and local agent-written
  tools. Keep session ownership, transport, plugin loading, and recovery in core.
- Make SDK operations callable without mounting a panel, so agents can use the
  same commands and inspect structured results.

## Existing building blocks

`ares/ares/node/debugger/memory.hpp` already defines named memory regions with
size, byte reads, and byte writes. `desktop-ui/tools/memory.cpp` enumerates them
from the running system and implements the native memory editor.

N64 exposes RDRAM, a cache-aware DCache view, RSP memory, and cartridge regions.
GBA exposes IWRAM, EWRAM, VRAM, palette memory, and OAM. The common access mechanism
already exists; available regions and their semantics differ by core.

The spike currently provides native loading, audio, video, input, pause, and
reset. It has no JS memory bridge or plugin host. Its dedicated video stream and
native stdout/stderr separation must remain intact as control traffic is added.
The separate ares-64 checkout already implements much of the advanced N64 debug
service we need; the linked port plan identifies native hooks and desktop coupling
that must be migrated rather than recreated from memory nodes alone.

## 1. Native services and the JS memory API

Deliver this before the plugin runtime, using a temporary developer panel.

### Session and memory contract

Expose session identity, system identity, game identity, capabilities, and a
region list. Region descriptors contain a session-scoped opaque ID, display name,
node path, byte length, and any verified access metadata. Names are for display;
do not assume names are unique or stable IDs across loads.

Proposed SDK usage:

```ts
const session = await emu.session.current();
const regions = await emu.memory.regions();
const ram = regions.find(region => region.name === "RDRAM");
const bytes = await emu.memory.read(ram.id, offset, 256);
const value = await emu.memory.read32(ram.id, offset, { endian: "big" });
await emu.memory.write32(ram.id, offset, value, { endian: "big" });
```

Reads/writes use region-relative byte offsets. Typed helpers interpret bytes;
`read32` does not claim to perform one native CPU bus transaction. Require explicit
endianness for multi-byte values initially. Offer bytes, integers, floats, and
bulk batches without inventing a universal CPU-address translation layer.

System-specific helpers, such as N64 virtual-address translation, can be added as
namespaced capabilities. They must describe cache and mapping behavior. Raw RDRAM
and DCache access are separate views, not interchangeable ways to patch a game.
Executable-code patches may also need core-specific cache/recompiler handling;
generic memory writes do not promise that behavior.

### Execution and transport

- Queue memory operations and execute them on the emulation thread at a defined
  safe point between runs, including while paused. Never touch core memory from
  the pipe-reader thread. A debugger batch observes one safe point; CPU stepping
  and precise mid-frame breakpoints are separate future capabilities.
- Tag requests with request ID, session ID, and plugin generation. Reject stale
  work after reload, ROM change, reset, or state restoration. Increment an epoch
  when the memory state is discontinuously replaced.
- Validate range, integer overflow, byte lengths, and request limits. Reject
  invalid accesses rather than relying on a region's wrapping behavior. Return
  explicit unsupported/unknown-region/stale-session errors.
- The current memory node has callbacks but no public access-permission flags.
  Audit exposed regions before claiming read-only/read-write metadata. Fail closed
  for unverified writes; add small upstream-style metadata if necessary.
- Version the native protocol and add typed response messages to its binary
  envelope. Keep video credit independent of control responses: a paused or
  stalled canvas must not prevent a memory request from completing. Use bounded
  queues, response priority, timeouts, and cancellation. Keep logs on stderr.
- Add bulk reads and sampled watches. Start around 10 Hz for visible panels,
  coalesce overlapping ranges, and stop subscriptions when hidden or disposed.
  A hex viewer should not make one IPC request per cell or read all RAM every frame.
- Writes can return previous bytes and accept expected-byte checks. A patch record
  is useful, but undo is conditional: do not restore stale bytes over later game
  changes. No automatic undo claim for arbitrary hardware or memory side effects.

Acceptance: discover regions on N64 and GBA; read a paused snapshot; modify and
restore a deliberately chosen scratch value; exercise endian/boundary cases;
reject stale requests; keep video/audio working during sampled reads. Use original
fixtures for deterministic write tests, not unknown live game addresses.

## 2. Plugin package, discovery, and SDK

A local plugin is a directory containing a manifest and bundled browser ESM.
Support plain JS/CSS without a build step and a TS/TSX template using a small
bundler. The host does not require React; templates can offer React as a convenient
choice. Keep SDK entry points and manifest schemas versioned.

```json
{
  "id": "local.kirby-animation-tools",
  "name": "Kirby animation tools",
  "version": "0.1.0",
  "apiVersion": 1,
  "entry": "dist/plugin.js",
  "styles": "dist/plugin.css",
  "activation": {
    "systems": ["Nintendo 64"],
    "games": [{ "profile": "kirby64-us-rev0", "sha256": "<verified-content-hash>" }]
  },
  "capabilities": ["memory.read", "memory.write", "session.control"],
  "contributes": {
    "panels": [{ "id": "animations", "title": "Animations" }],
    "commands": [{ "id": "show-animations", "title": "Show animations" }]
  }
}
```

The loader validates IDs, API version, entry paths, contributions, activation
rules, and capabilities before executing code. All contribution IDs are namespaced
by plugin ID. Bundled IDs cannot be silently replaced by a local folder.

Discover bundled plugins plus configured user/workspace plugin roots. Local
directories are the priority for agent development; npm/git distribution and a
marketplace can wait. Development sources are explicitly enabled by the user;
ordinary edits to an enabled source reload without a confirmation on every save.

The SDK supplies memory/session services, UI registration, commands, logs,
namespaced versioned storage, settings, and scoped resource disposal. Registration
returns a disposer. Plugin manifest capability declarations route API access and
make intent visible, but do not sandbox trusted same-renderer JavaScript.

Acceptance: load a plain JS plugin and a TS template; display useful manifest/build
errors; disable/remove cleanly; run a registered command without its panel open.

## 3. UI contributions and customization

Provide both structured contributions and broader trusted customization:

| Surface | Ownership and use |
| --- | --- |
| Dockable panel | Host owns tabs, docking, sizing, persistence; plugin owns content. Hex viewers, actor lists, timelines. |
| Workspace page | Plugin owns a large work area; host retains navigation and recovery controls. Model/effect workspaces. |
| Viewport overlay | Host provides viewport coordinates, scale, focus and input routing; plugin draws labels or controls above the canvas. |
| Toolbar/menu/command | Host renders discoverable entry points; plugin provides actions and enabled conditions. |
| Settings section | Host owns settings navigation; plugin supplies custom UI or a schema-rendered form. |
| Theme | Registered palette/style tokens, selectable by the user. |
| Trusted shell extension | Explicit `shellEntry` and `ui.shell` declaration; can modify shell DOM and layout and combine any of the above. |

Give ordinary surfaces a mount element and framework-neutral lifecycle. Use scoped
CSS or Shadow DOM where practical for style hygiene, not as a security claim.
Provide theme tokens, focus/shortcut handling, and UI components as conveniences.
Plugins can be completely custom inside their mount; they need not use a fixed
form DSL. An overlay must not accidentally steal game input, and closing or hiding
a panel releases its input capture.

Core owns the emulation canvas and exposes a viewport handle/layout API instead of
requiring plugins to recreate the video pipeline. Trusted shell code may customize
the surrounding experience. Its DOM hooks are explicitly experimental and more
fragile than registered surfaces. Provide a startup safe mode that skips local
plugins and restores the default shell, plus an always-available recovery command.

Catch plugin exceptions and show a per-plugin failure. A synchronous infinite loop
in a trusted renderer can still freeze the UI; only the separate native process is
isolated today. Strong isolation for untrusted plugins would require a separate
renderer/runtime and is not a prerequisite for this local trusted workflow.

Acceptance: a panel, overlay, and shell extension coexist; layouts persist; keyboard
focus behaves; a broken contribution can be disabled; safe mode recovers the shell.

## 4. Game matching and game profiles

Separate install/enable, compatibility, activation, and panel visibility. A plugin
can remain installed while inactive; a closed compatible panel need not unload
its explicitly registered commands. Hidden panels should stop their UI polling.

Matching rules:

- No game constraint: general-purpose tool; works without a ROM if its capabilities
  allow it. System-only constraint: tool for any game on that system.
- Game constraint: exact content hash/revision match by default. Hash the decoded
  primary media content, not ZIP bytes or the filename; document the algorithm and
  normalization. Multi-media sessions expose a media identity list. Do not assume
  MIA hashes are normalized identically across systems without checking.
- System and capability constraints are AND conditions; listed compatible game
  identities are OR alternatives. ROM title/code can suggest a profile but cannot
  authorize automatic writes to a supposedly matching revision.
- Evaluate on load/unload and session changes. Deactivate incompatible plugin
  scopes before delivering the new game, revoke queued operations, and remove their
  UI. Reset/state load invalidates observations even if game compatibility remains.
- Offer a manual development override that is clearly marked. Address-dependent
  write tools must additionally validate their expected signatures at use time.

Game profiles are versioned data modules: identities, region references, structures,
symbols, pointer resolvers, and named actions. Keep these separate from reusable
viewers. Profiles can initially live inside a plugin; a registry can follow once
two tools actually need to share one. No filename-based address assumptions.

Acceptance: generic memory viewer works on N64/GBA; Kirby-only panel appears only
for a verified Kirby revision; switching ROMs cannot deliver a late write to the
new game; mismatched revisions explain why the tool is inactive.

## 5. Hot replacement without restarting emulation

Define the lifecycle before adding filesystem watchers:

1. Watch an enabled source, debounce saves, and build into a new immutable,
   content-addressed output directory. Ignore build outputs and dependencies.
2. Validate manifest and import the candidate with a fresh module identity. Stage
   registrations without effects or memory access. Build/import failures keep the
   current generation live and put diagnostics in the plugin developer panel.
3. At commit, revoke the old generation's API leases and cancel its queued native
   operations. Abort subscriptions/jobs, dispose resources in reverse order, then
   remove old UI/styles. Await cleanup with a bounded timeout; never overlap writers.
4. Activate the new generation, restore compatible serialized UI state, and publish
   registrations. If activation fails, dispose the candidate and attempt to remount
   the previous artifact. Report a rollback failure rather than claiming recovery.
5. Never restart or reset the native emulator for a JS/CSS update. Stable panel IDs
   retain docking/selection. Changes to native APIs or core binaries still require
   a host restart; this is plugin hot replacement, not native code hot patching.

The plugin context owns timers, watches, event handlers, commands, subscriptions,
and mounted UI, with an AbortSignal and idempotent cleanup. Trusted shell scripts
must explicitly undo DOM changes. Durable state lives in namespaced storage;
transient state is optional, JSON-serializable, and schema-versioned.

Reload never repeats a patch or command automatically. Already executed memory
writes and exports are not rolled back by reloading UI. Late async results from
an old generation are ignored. Bound memory/watch traffic so one plugin cannot
starve the game. Pause background plugin work when no consumer needs it.

Acceptance: repeatedly reload a polling/writing tool without accumulating timers,
listeners, or requests; a syntax error preserves the previous panel; a failed
activation cleans up; ROM switches during reload stay safe; native session and
audio continue throughout. Keep an explicit Reload button as well as file watching.

## 6. Bundled tools and native settings as plugins

Prove the same public SDK by moving the generic memory viewer, presentation controls,
and plugin developer tools into bundled plugins. Bundled plugins get the same
registration/lifecycle machinery as local plugins, not private renderer shortcuts.
Core retains recovery even if a bundled UI plugin is disabled.

Port useful ares settings incrementally after this vertical slice:

| Bundled plugin | Initial features | Apply behavior |
| --- | --- | --- |
| Firmware and paths | BIOS configuration, file selection, validation, saves location | Usually next ROM load; explain affected session. |
| Presentation | Theme, fit/integer scaling, scanlines, layout, supported color options | Live where supported. |
| Audio | Volume, mute, device/latency options supported by the host | Live or explicit audio reinitialization. |
| Input | Keyboard/gamepad mappings, focus behavior, controller selection | Release held input before applying changes. |
| System settings | N64 expansion/render options, GBA model, other supported core options | Per-setting live/reset/reload metadata. |

The settings service, not each settings plugin, owns validation, persistence, and
application. Describe keys with type/default/range, scope, supported systems, and
`live`, `audio-reinit`, `reset`, or `next-load` behavior. Plugins can use schema forms
or custom controls against the same service. Hide unsupported settings rather than
copying native options whose behavior the host does not implement.

Keep imported desktop `settings.bml` read-only by default, with Electron-specific
overrides persisted separately. Make import/export explicit and retain firmware
paths. This avoids two apps silently overwriting one another's settings. Remove
the spike's hardcoded overrides as their real settings APIs are implemented.

Do not advertise native rewind, run-ahead, shaders, or debugger controls merely
because those keys exist: each requires a working backend and appropriate tests.

Acceptance: bundled firmware plugin configures GBA BIOS; presentation/audio controls
change actual behavior; restart-required options say so; settings survive restart;
a local plugin can contribute a settings section using the same API.

## 7. Agent workflow and first game tool

Ship a small scaffold command, SDK declarations/examples, plugin validation/build
commands, and a developer panel showing activation, errors, logs, requests, and
reload status. An agent should create a plugin in an enabled workspace directory,
build it, request its panel to open, and read a structured success/error result.
Auto-open a newly added matching tool once in development mode; a file save must
not repeatedly steal focus. Persist user dismissal.

Expose session inspection, memory operations, plugin lifecycle, and registered
commands through a TypeScript MCP adapter and a small local CLI using shared
TypeScript service contracts/client code with the UI. A lightweight Node stdio
entry point connects to Electron main's local endpoint; it does not launch another
app or emulator. Preserve useful MCP tool behavior, not the Python runtime or old
telemetry TCP format. Keep native operations and ownership in one coordinator;
plugin/renderer reload must not disconnect external agents. See the MCP port plan
for runtime packaging, app-instance selection and transport validation. Do not add
an agent framework or remote service just to get started. Log actor/plugin/request/
session identity for writes.

First game milestone: a Kirby panel with a few verified fields, then a labeled
animation selector when the addresses are known. Keep write actions explicit;
hot loading a UI should not itself mutate the game. Extend to actor lists, pointer
inspection, animation timelines, particle controls, and asset export as discoveries
justify them. Reusable viewers should consume profile data rather than hardcode
game logic into the shell.

Acceptance: an agent can create and revise a tool while the game runs, show it to
the user, query the same data without UI scraping, and leave behind a usable plugin.

## Delivery order and scope

1. Shared native memory/session service and typed service contracts, TypeScript
   MCP adapter/Node stdio entry point, and temporary JS inspector (MCP port phase A
   and step 1 here). Python is reference/test material only.
2. Port controlled N64 execution, input ownership and the remaining MCP contract,
   including captures/states/advanced tools (MCP port phases B–C).
3. Minimal plugin SDK/loader and dockable panel, game matching, then hot replacement
   and lifecycle stress checks (steps 2–5; MCP port phase D).
4. Generic memory/stop/capture panels as bundled plugins, then one Kirby tool.
5. Settings service and bundled firmware/presentation/audio/input plugins (step 6).
6. Broader workspace/overlay/shell contributions, useful fork debug views, and additional system settings,
   using the lifecycle established above. Panel/manifest design reserves these
   modes from the beginning; they need not all ship in the first milestone.

Avoid a marketplace, remote plugin installation, a custom rendering framework,
untrusted-code sandbox, and universal CPU bus model in the initial implementation.
Revisit the host's desktop support dependency separately; it does not block the
first memory bridge or agent-authored panel. Measure CPU/memory with one active
emulator, hidden panels, and repeated reloads before adding high-frequency tooling.

## Reference: ideas from bb

Reviewed 2026-10-17. These are inspirations, not a plan to import bb's runtime.

- bb's [plugin SDK documentation](https://github.com/get-bb/bb/blob/main/packages/plugin-sdk/README.md)
  describes typed UI contributions, overlays, content scripts, and lifecycle-aware
  testing. Adopt clear surface contracts and disposable registrations; keep our
  emulation API independent of the UI.
- Its [app contract](https://github.com/get-bb/bb/blob/main/packages/plugin-sdk/src/app-contract.ts)
  distinguishes panels/pages/settings/overlays. Use similarly explicit extension
  points instead of making DOM selectors the main integration API.
- The [content-script example](https://github.com/get-bb/bb/blob/main/examples/plugins/content-script/README.md)
  documents trusted shell code, AbortSignal-driven cleanup, and removal of listeners
  and DOM modifications. Use that model for the optional unrestricted UI mode;
  do not describe it as isolated code.
- bb's [configuration docs](https://github.com/get-bb/bb/blob/main/docs/configuration.md)
  describe bundled plugins and per-plugin data/logs. Reuse the general idea of
  first-party features on the plugin mechanism, with clear storage ownership.
- bb's [system overview](https://github.com/get-bb/bb/blob/main/docs/system-overview.md)
  treats the CLI as a first-class client. Apply that principle to agent access
  without adopting bb's distributed server/daemon topology.

Hot-reload staging, emulation safe points, session epochs, game identity rules, and
memory write semantics above are proposals for this project, not claims about bb.
