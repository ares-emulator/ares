# Local plugins (API 1)

Open **Plugins** in the sidebar. Add a directory containing `manifest.json`, then
explicitly enable it. Bundled examples are disabled until enabled. Enabled roots
and IDs persist under Electron's userData/plugins; `ARES_PLUGIN_DIRS` (platform
path delimiter) explicitly enables development directories at startup. Refresh
re-reads manifests and restarts their scopes. Watched hot replacement is task 11.

Plugins are trusted renderer JavaScript, not a security sandbox. Install only
code you trust. Node integration stays disabled. Main routes declared service
capabilities and pins every SDK request to the plugin generation/session/epoch.
Disabling, renderer reload, reset or ROM switch revokes old tokens, pending
responses, resources and SDK calls, and releases only that plugin's execution
owner. Plugin commands work before any panel has mounted.

See `contracts.ts` for the manifest and bridge and `sdk.ts` for the author API.
Entries must export `activate(context)` and can return a disposer (including
asynchronously). Register only declared local panel/command IDs; host identities
are namespaced by plugin ID. Panels mount lazily and may return their own cleanup.
Use `scope.listen`, `scope.interval`, `scope.add` and `signal` for cleanup; raw
module side effects are the author's responsibility. Tabs toggle visibility;
`onVisibility` lets panels suspend their work. Select left/right/bottom to dock.

Capabilities: `session.read` (status), `session.control` (pause/resume and bounded
execution), `memory.read` (regions/read), `memory.write` (write). No filesystem or
arbitrary service methods are exposed through the SDK. Service replies preserve
native errors; check `reply.ok`. Stale scopes reject with `stale_plugin_scope`.
Logs retain the latest 128 messages (2 KiB each). Activation/command failures show
in the manager; `log.error` records a message without disabling the plugin.
Storage is JSON, namespaced by plugin ID and storageVersion, limited to 16 KiB.

Activation systems use exact native system names. Game profiles require a verified
SHA-256 of the normalized MIA `program.rom` content, reported as `session.status`
`game`. Filename and directory never participate. Unsupported revisions stay
waiting. Disc/multifile games without program.rom have no content profile yet.
The Kirby example matches the verified US revision 0 hash; it only displays the
identity. Gameplay tools belong to task 12. Overlay/page/shell modes are reserved
and produce an actionable unsupported-mode message.

For plain JS copy `bundled-plugins/memory`. For TS/TSX copy
`templates/plugin-tsx`, run `npm install` then `npm run build`, and add that directory.
The template bundles JSX with a tiny DOM factory; no framework is imposed. Its
sdk.d.ts describes the subset it uses; the full API is in plugins/sdk.ts.

Validation: `npm test` includes manifest/registry and real N64/GBA scoped memory
checks; `npm run test:plugins` runs the real Electron UI test. Optional Kirby test:
`ARES_PLUGIN_TEST_ROM=/path/to/verified.z64 node --test tests/plugins-game-live.test.cjs`.
It uses disposable renamed and byte-swapped copies and saves, never the originals.
