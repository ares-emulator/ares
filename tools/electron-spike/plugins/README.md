# Local plugins (API 1)

Open **Plugins** in the sidebar. Add a directory containing `manifest.json`, then
explicitly enable it. Fresh profiles enable the bundled memory explorer, N64
stop/register view and verified Kirby powers tool. Existing enable/disable choices
are preserved; use Plugins or the CLI to enable a new bundled tool in an older
profile. `ARES_DEFAULT_PLUGINS=0` suppresses these first-profile defaults for tests. Enabled roots
and IDs persist under Electron's userData/plugins; `ARES_PLUGIN_DIRS` (platform
path delimiter) explicitly enables development directories at startup. Refresh
re-reads manifests using the same replacement transaction as each plugin’s Reload
button. Enabled source directories are watched with a 180 ms debounce; dependencies,
hidden directories and declared build outputs are ignored.

Plugins are trusted renderer JavaScript, not a security sandbox. Install only
code you trust. Node integration stays disabled. Main routes declared service
capabilities and pins every SDK request to the plugin generation/session/epoch.
Disabling, renderer reload, reset or ROM switch revokes old tokens, pending
responses, resources and SDK calls, and releases only that plugin's execution
owner. Plugin commands work before any panel has mounted.

See `contracts.ts` for the manifest and bridge and `sdk.ts` for the author API.
Entries must export `activate(context, restoredState?)` and can return a disposer (including
asynchronously). Register only declared local panel/command IDs; host identities
are namespaced by plugin ID. Panels mount lazily and may return their own cleanup.
Use `scope.listen`, `scope.interval`, `scope.add` and `signal` for cleanup. SDK
listeners/timers arm only after activation succeeds. Interval callbacks run only
while the plugin has a visible panel. Disposers may return promises; cleanup is
bounded to one second and failures are reported. Raw
module side effects are the author's responsibility. Tabs toggle visibility;
`onVisibility` lets panels suspend their work. Select left/right/bottom to dock.

Capabilities: `session.read` (status), `session.control` (pause/resume and bounded
execution), `debug.read` (N64 frozen stop/register snapshots), `memory.read`
(regions/read and N64 CPU/RDRAM reads), `memory.write` (region writes and N64
single/batched writes). No filesystem or
arbitrary service methods are exposed through the SDK. Service replies preserve
native errors; check `reply.ok`. Stale scopes reject with `stale_plugin_scope`.
Logs retain the latest 128 messages (2 KiB each). Activation/command failures show
in the manager; `log.error` records a message without disabling the plugin.
Storage is JSON, namespaced by plugin ID and storageVersion, limited to 16 KiB.

Activation systems use exact native system names. Game profiles require a verified
SHA-256 of the normalized MIA `program.rom` content, reported as `session.status`
`game`. Filename and directory never participate. Unsupported revisions stay
waiting. Disc/multifile games without program.rom have no content profile yet.
The Kirby powers tool matches the verified US revision 0 hash and offers None,
seven singles and all 28 pairs using the previously researched addresses and HUD
icons. See [power mappings and provenance](../bundled-plugins/kirby/README.md). Overlay/page/shell modes are reserved
and produce an actionable unsupported-mode message.

For plain JS copy `bundled-plugins/memory`. For TS/TSX copy
`templates/plugin-tsx`, run `npm install` then `npm run build`, and add that directory.
The host can build the template directly from its manifest `build.entry`; npm
build remains available for standalone authoring. The template bundles JSX with a tiny DOM factory; no framework is imposed. Its
sdk.d.ts describes the subset it uses; the full API is in plugins/sdk.ts.

Validation: `npm test` includes manifest/registry and real N64/GBA scoped memory
checks; `npm run test:plugins` runs the real Electron UI test. Optional Kirby test:
`ARES_PLUGIN_TEST_ROM=/path/to/verified.z64 node --test tests/plugins-game-live.test.cjs`.
It uses disposable renamed and byte-swapped copies and saves, never the originals.

## Watched replacement

Plain-JS entries and optional `build: {entry:"src/plugin.tsx", jsxFactory:"h"}`
inputs are bundled by the host's esbuild into immutable content-addressed artifacts
under userData/plugins/artifacts. Relative imports and assets are captured; files
are limited to 2 MiB each, snapshots to 16 MiB / 256 files. Build/import errors keep
the current scope, DOM and styles intact. Host bundles include esbuild; no external
npm/watch process is required. Installing a plugin's dependencies remains an author
step; package scripts are never executed by the host.

Replacement imports the candidate under a fresh module identity and may call
`export function prepare()` for pure validation. No SDK is supplied during this
stage; manifest contributions are validated but not published. Raw top-level module
side effects remain the trusted author's responsibility. Avoid side effects during
import/prepare. Import, prepare and activation each have a two-second deadline.

Before activation, main revokes the old token and confirms native release of only
its owner. The host aborts its SDK effects and invokes cleanup in reverse order,
then activates the candidate with registrations detached and listeners/timers inert.
Reads are available during activation; writes, pause/resume, execution and storage
mutations reject with `plugin_not_ready`. Put patches and other mutations behind
explicit commands. Restoring an open panel cannot implicitly replay a patch.

If activation fails, the candidate is disposed and the previous immutable artifact
is remounted under a fresh token. Failed rollback is shown explicitly and the
plugin can recover through another valid edit/Reload. Already executed commands
are never undone or replayed. Session/epoch changes invalidate an in-flight reload.
The emulator is never reset/restarted for a source/CSS change; an external MCP
operation retains its owner. Releasing an old plugin-owned run cancels that run.

Optional transient state:

```js
export const stateVersion = 1;
let selection = 0;
export function saveState() { return {selection}; }
export function activate(ctx, restoredState) {
  selection = restoredState?.selection ?? 0;
  // Register panels/commands using this selection.
}
```

State must serialize as JSON under 16 KiB and is restored only when both artifacts
opt into the same stateVersion. Durable data stays in namespaced storage. Stable
panel IDs retain selection/visibility and the dock retains its position. A hidden
panel is not eagerly remounted; its serialized state is available on activation.

The developer manager shows scope/listener/timer/request counts and build/reload
errors. `npm run test:reload` exercises repeated reloads, polling, stale writes,
invalid edits, failed activation/rollback, hanging cleanup, real MCP ownership,
plugin-owned operation release, rapid edits and a session switch during preparation.
Artifact pruning retains current/previous/candidate snapshots, rather than every
edit. Chromium caches imported ESM modules until renderer teardown; the checks here
prove SDK effect/request cleanup, not that Chromium unloads individual modules.

## Bundled tools and agent commands

The former renderer-owned inspector is now `ares.examples.memory`, using the same
scoped SDK as other plugins. It supports generic debugger regions on N64/GBA,
validated offsets/lengths, whitespace-tolerant hex writes and hot-reload UI state.
`ares.tools.debugger` displays the native immutable last stop and its captured
registers; live registers are used only when no stop exists. Neither tool writes
or changes emulation while opening/restoring a panel.

Build the checkout, start the app with services enabled, and explicitly select its
connection file and instance ID (reported by main). In a copied macOS app use
`Ares.app/Contents/Resources/ares-plugin` instead of `node dist/plugins/cli.js`.
The launcher uses bundled Node/compiler/templates and needs no developer runtime.

```sh
node dist/plugins/cli.js scaffold /tmp/my-tool --id local.my-tool
node dist/plugins/cli.js build /tmp/my-tool
node dist/plugins/cli.js list --connection "$CONNECTION_FILE" --instance "$INSTANCE_ID"
node dist/plugins/cli.js add /tmp/my-tool --connection "$CONNECTION_FILE" --instance "$INSTANCE_ID"
node dist/plugins/cli.js enable local.my-tool --connection "$CONNECTION_FILE" --instance "$INSTANCE_ID"
node dist/plugins/cli.js open local.my-tool hello --connection "$CONNECTION_FILE" --instance "$INSTANCE_ID"
node dist/plugins/cli.js reload local.my-tool --connection "$CONNECTION_FILE" --instance "$INSTANCE_ID"
node dist/plugins/cli.js command ares.examples.kirby power-14 --connection "$CONNECTION_FILE" --instance "$INSTANCE_ID"
```

`enable ID --disabled` disables a plugin. `open` may omit the panel name to select
the first declared panel. `command` requires a declared command ID. `--token`
allows explicitly pinning an existing generation; otherwise the CLI obtains the
current token immediately before sending an action. Stale tokens reject rather
than binding to a replacement/game. Reload waits for the new active generation
and reports build/activation errors; it never retries a command. `build --output`
selects the artifact cache; the default `.ares-plugin-build` is ignored by watchers.
The list is paged without log bodies; `--page-size 1` reduces response size and the
CLI automatically reduces oversized pages.

Management shares the authenticated, private local endpoint's instance selection,
replay protection, bounded requests and no-replay reconnect behavior. Commands
invoke registered SDK handlers directly, with main-frame acknowledgement; no UI
scraping/eval is used. Timeouts mean the action was not replayed, not proof that an
already accepted effect was undone. The existing MCP registry remains 68 tools.

`npm run test:plugin-tools` demonstrates an MCP watch stop matching the panel,
CLI scaffold/build/add/enable/open/reload, the loaded session surviving edits,
external-run busy exclusion and human pause. `npm run test:kirby` requires
`ARES_PLUGIN_TEST_ROM`; it verifies all 36 buttons/icons/readbacks using disposable
ROM/settings/saves, and leaves originals unchanged. `ARES_BACKGROUND_TEST=1`
hides test windows while retaining unthrottled rendering.
