# Electron frontend audit and triage

Audit date: 2026-10-20; user clarification 2026-10-21. Baseline: task 12 commit `539755659`.
The native/Electron architecture is validated. “Spike” no longer describes its
intended lifetime. This inventory reviews repository layout/builds, runtime
ownership, persistence, extension machinery, service compatibility, distribution,
UI and validation. It is a source/documentation audit, not a new runtime test or a
proof that every possible defect has been discovered.

**Task 13 is paused for this triage.** No relocation or runtime behavior was changed
by this audit. The user approved the triage order on 2026-10-21; implementation is authorized.
See RESUME.md for the concise approved sequence.
Effort: S = bounded local change; M = several interacting modules; L = separate
project/multiple reviewable changes. Estimates are relative, not time promises.

## Recommended first decisions

1. Give this a permanent frontend home. Recommend **`electron-ui/`**, beside
   `desktop-ui/`. Initially move host/native/services/MCP/plugins/tests together;
   splitting several packages or extracting a host library is a separate decision.
2. Agree stable app/profile identity and data locations as part of renaming.
   **The user explicitly does not require preserving existing development data:**
   no compatibility aliases, data copying or migration framework are needed for it.
   A directory rename still touches CMake, runtime lookup, scripts, tests and docs.
3. Make active play and development builds coexist safely, then establish one
   repeatable acceptance command/CI lane.
4. Resume settings work with explicit data locations and imported-setting rules.
   Defer distribution/platform expansion. Extract the live desktop dependencies
   before retiring that superseded frontend; see the proposed order below.

## Findings to triage

### Foundation and maintainability

| ID | Finding and evidence | Consequence / proposed disposition | Effort |
| --- | --- | --- | --- |
| F01 | Everything lives under `tools/electron-spike`; root `CMakeLists.txt` uses `ARES_BUILD_ELECTRON_SPIKE`; package name/description, HTML and app bundle ID still say spike/experiment. | Promote to `electron-ui/`; rename build option, product identity and docs deliberately. No old-option alias is required for existing development use. Inventory all path consumers, including native/core changes, before moving. **Before 13.** | M |
| F02 | Identity/versioning is inconsistent: source package `ares-electron-spike` 0.1.0; packager writes `ares-workbench` 0.2.0 and `org.ares.workbench.spike`; renderer persists `ares-spike`. Main uses Electron's inferred userData location. | A rename can appear to lose enabled plugins/preferences; dev and packaged profiles may differ already. Choose stable names/version source and explicit profile paths, start fresh; the user has waived preserving existing development data. **With F01.** | M |
| F03 | Native target compiles desktop presentation, tools, game-browser and program sources and links Hiro (`CMakeLists.txt`); host references desktop globals/settings/loaders. | Useful reuse avoided duplicate loaders, but headless frontend depends on GUI support and global state. Desktop UI is being superseded as the product, but these reused implementations are live dependencies. Extract the required loader/config/input/resource code before deleting the folder or dropping Hiro; retain other desktop code only as a feature reference until intentionally retired. | L |
| F04 | `host.cpp` owns input/audio/video, loading, service dispatch and N64 coordinator wiring; `main.ts` owns lifecycle, IPC, plugin actions and transport; `renderer.ts` mixes GL/input/settings/test reporting. Many native headers contain dense single-line method dispatch/JSON. | Changes have wide review surface. Split by ownership, format touched code, and isolate protocol encoding from emulator mutations. Do not rewrite working semantics wholesale. | M–L |
| F05 | Protocol/service declarations, native whitespace parser/manual JSON, TS validators and MCP schemas are maintained in separate places (`services/contracts.ts`, native services, `mcp/*`). `any` remains at several decoded-message boundaries despite strict TS. | New methods/bounds can drift. Centralize method/error/bounds metadata where practical, add shared conformance fixtures, narrow unknown inputs. Generated native schemas are an option, not a prerequisite. | M |
| F06 | `npm run build` deletes `dist`; start/test/package invoke it automatically. A live checkout app imports files from that same tree. Task 12 needed an ad hoc isolated snapshot. Native output also has a conventional shared build directory. | Development can invalidate a running app or replace its executable/assets. Introduce separate/staged build outputs or an isolated dev launch workflow with independent profiles. **Before more feature work.** | M |
| F07 | Roadmap opening was stale; supporting README still described removed `inspector.ts` and earlier incomplete tasks; log is nearly 2,000 lines of historical status. | A fresh agent can resume the wrong task. This audit adds `RESUME.md`, corrects the current entry point and links this inventory. Consolidating historical docs remains a separate cleanup. **Partially addressed here.** | S |

### Persistence, settings and host lifecycle

| ID | Finding and evidence | Consequence / proposed disposition | Effort |
| --- | --- | --- | --- |
| D01 | Checkout saves default to `artifacts/saves`; packaged saves go to userData; raw host falls back to `electron-data/saves`. Plugin config/storage is under userData while presentation/dock settings use localStorage (`main.ts`, `host.cpp`, `renderer.ts`, `plugins/runtime.ts`). | Important saves live in a directory labelled disposable; profiles do not form one obvious backup/export unit. Define data/config/cache/artifact roots; existing development data does not need migration. **Before/with F01 and 13.** | M |
| D02 | Host reads desktop settings but forces fast boot, N64 SD/no supersampling and interpreter preference; audio is SDL/48kHz with fixed mix gain; input mappings are overwritten (`host.cpp`). Selected settings path is only a process variable. | UI cannot explain or persist many actual behaviors. Implement descriptors/apply/restart rules and explicit Electron overrides. **Task 13**, with current defaults documented until replaced. | M–L |
| D03 | No explicit profile locking/single-instance policy in `main.ts`. Separate processes can use the same save tree, plugin enabled.json and storage files; atomic rename prevents partial files but does not coordinate independent writers. | Multiple instances can overwrite each other's persisted state/saves. Decide single instance per profile or isolated named profiles with a lock; retain multiple explicitly isolated agent instances. **Early foundation work.** | M |
| D04 | ROM identification happens before `stop()`, but actual host/media/firmware initialization happens after the prior host stops (`main.ts:start`). | A file that identifies successfully but fails to load still ends the current game. Decide preflight limits and explicit replacement/error UX; don't promise fully transactional ROM changes. | M |
| D05 | Graceful EOF unload flushes saves, but shutdown kills a stuck host after two seconds; no periodic save/backup policy or crash restoration UX is defined (`main.ts:stop`, `host.cpp` unload). | Crash/forced termination can lose changes since the last flush; application lifetime contract needs documentation and focused failure tests. Add safe flush/checkpoint support if the loader supports it. | M |

### Plugins and agent tooling

| ID | Finding and evidence | Consequence / proposed disposition | Effort |
| --- | --- | --- | --- |
| P01 | Plugins import into the UI renderer and can create global side effects/access shared DOM; SDK revocation only governs SDK-managed work (`plugins/README.md`, runtime/scope). Timeouts cannot preempt a synchronous JS infinite loop. | This is a deliberate trusted-code model. Keep it explicit; provide startup recovery outside the renderer before broader use. Process isolation is a separate product/security decision. | M for recovery; L for isolation |
| P02 | No dedicated startup safe mode that bypasses all persisted enabled plugins; `ARES_DEFAULT_PLUGINS=0` only suppresses fresh-profile defaults. Broken rollback has diagnostics but a wedged renderer cannot use its manager. | A faulty plugin can make the UI difficult to recover. Add a documented `--disable-plugins`/safe-mode launch path and targeted disable/reset commands. Task 14 planned recovery; recommend bringing this piece forward. | S–M |
| P03 | API v1 SDK is repository-local; template has a manually maintained subset of types; host esbuild transpiles TS/TSX without performing TypeScript type checking (`plugins/sdk.ts`, template `sdk.d.ts`, `plugins/artifacts.ts`). | Standalone authors lack a complete versioned type contract; successful build is not a typecheck. Generate/distribute declarations and add an optional author typecheck command before adding more APIs. | M |
| P04 | Source artifacts are captured, but dependency resolution uses the source `node_modules`; dependencies and hidden directories are not watched. `fs.watch` failure asks users to Reload (`plugins/artifacts.ts`, `registry.ts`). | Installed dependency changes can require manual reload; mutable dependencies make a build less reproducible than a full locked input snapshot. Document this, test watch failure/recovery, decide whether lockfile changes should rebuild. | S–M |
| P05 | Disk artifact pruning and SDK effect counts are tested, but repeated dynamic ESM imports have no unload primitive. Twelve reloads prove bounded tracked effects, not a long-run renderer heap ceiling. | Long authoring sessions may accumulate module/runtime memory. Measure a longer reload/heap soak; consider controlled renderer replacement or isolation if measured growth matters. **Measurement needed; no measured leak claimed.** | M |
| P06 | Memory/Kirby production tools retain `ares.examples.*` IDs; Kirby CSS hides its generated command buttons using host DOM selectors. Commands accept no argument/result schema, and management waits have fixed small deadlines (`scope.ts`, `main.ts`, CLI). | Public API and internal UI are already coupled. Keep IDs stable or migrate them; add declared command presentation metadata instead of CSS conventions. Decide command input/results/timeouts before expanding automation use cases. | M |
| P07 | Game identity is a hash of MIA `program.rom`; unsupported disc/multifile content has no profile identity; one Kirby revision is supported. | Deliberate limited matching, not universal game identification. Preserve fail-closed matching; extend identity model only for concrete new games/media. | M when needed |

### Validation and reproducibility

| ID | Finding and evidence | Consequence / proposed disposition | Effort |
| --- | --- | --- | --- |
| V01 | Existing GitHub workflows do not build/test this optional Electron target; acceptance is a collection of npm scripts and manually recorded commands. `npm test` does not run all real Electron/package gates. | Core/desktop changes can regress this frontend unnoticed. Add one documented validation entry point plus a macOS CI lane with fixture-only tests; keep optional game/manual gates separate. **Early foundation work.** | M |
| V02 | Physical keyboard/gamepad takeover, audible-output acceptance and serializer rollback-failure fault injection remain open in the log. Automated button/readback checks and the user playtest cover different claims. | Close these specific checks or explicitly accept/defer them in triage. Do not relabel milestones fully accepted based on nearby passes. | S–M |
| V03 | Reproduction assumes developer tooling and conventional paths; no Node engine/toolchain pin declared in package.json, tests use modern `require(ESM)`, ffprobe defaults to Homebrew. Game/reference checks require private media/reference binaries; ignored results and `/tmp` snapshots can disappear. | Fresh agents can rebuild code but cannot reproduce every historical test without inputs. Declare tool prerequisites/version policy; unify fixture/config helpers; document optional inputs and persist concise acceptance summaries in Git. | M |
| V04 | Long soak, idle/ordinary-play/capture overhead, end-to-end latency and audio underrun instrumentation remain unevaluated (README final evaluation notes). Services-enabled mode starts capture/provenance from boot; video copies traverse native/main/renderer. | Functional success is not a sustained performance budget. Establish representative measurements before optimizing or adopting shared-memory transport. | M |
| V05 | Registry advertises all compiled systems, but runtime acceptance primarily covers synthetic, GB/NES, GBA and N64. Most firmware/peripherals, multiplayer, discs and unusual displays are untested. | Distinguish loader availability from supported product configurations; choose a smoke matrix and explicit support tier. | M–L |

### Distribution and user experience

| ID | Finding and evidence | Consequence / proposed disposition | Effort |
| --- | --- | --- | --- |
| R01 | Packaging manually copies Electron.app, dependencies and SDL/MoltenVK, rewrites rpaths and ad-hoc signs; only macOS arm64 is accepted. No notarization, installer, update channel or formal release artifact/version policy (`scripts/package-macos.cjs`). | Validated development distribution is not a public release pipeline. Decide intended distribution before investing in release tooling; avoid unsupported release claims. | L |
| R02 | Local endpoint explicitly rejects Windows; Unix socket permissions, fd3 control channel, macOS package/test Seatbelt and framework assumptions have not been ported. | Cross-platform frontend claims need actual transport/build/package work. Treat Windows/Linux as separate acceptance projects, not a folder-rename side effect. | L |
| R03 | Native runtime retains compile-time source-root lookup fallbacks; development host/tests assume build-electron-make. CMake and packager duplicate Database into two locations for platform path behavior. | Checkout coupling is contained by copied-package denial tests but still obscures resource resolution. Define explicit resource roots, make dev paths configurable/centralized, and remove source fallback from release builds. | M |
| R04 | Experimental synthetic/stall/metrics controls and startup synthetic pattern are the default app surface (`index.html`, renderer/main). No normal library/recent-media/start-screen flow, standard menu/file associations or polished load/error UX. | Promote the frontend's user-facing identity along with its folder; move test controls behind a developer mode. Prioritize a minimal normal-play shell before broad feature parity. | M |
| R05 | Renderer requires WebGL2 and does not implement context-loss/restoration handling; uses preserveDrawingBuffer for diagnostics. Hardcoded input polls the first standard gamepad into player one; no remapping/device selection/rumble path is exposed. | Rendering recovery and real controller UX are unfinished. Input settings belong in 13; explicitly scope multiplayer/rumble and add GL recovery tests when hardening normal play. | M |
| R06 | Source provenance exists for ported native behavior, but Kirby icons are described by a local path without a source revision/license record; packager collects some licenses but there is no complete third-party asset/notice manifest. | Capture reproducible icon provenance and decide whether game art belongs in the default distributable, a demo, or user-installed tool. Review shipped notices before release. | S–M |

## Explicit limits to keep distinct from accidental debt

These are documented implementation contracts, not automatic bugs to “fix.”

- One native execution coordinator, explicit instance selection, stale-scope
  rejection and no mutation replay are deliberate invariants to preserve.
- Copied latest-frame video with backpressure is validated. Zero-copy is an
  optimization requiring measurements (V04), not a promotion prerequisite.
- Sandbox preload emits CJS; application sources are TypeScript/ESM. This is an
  Electron runtime constraint, not unfinished migration.
- No Python bridge, legacy TCP compatibility, marketplace or untrusted-plugin
  sandbox is promised. Same-renderer trust is explicit (P01).
- N64 controlled execution/debugging is system-specific. Generic memory regions
  have different semantics; do not turn raw RDRAM region writes on for symmetry.
- Bounded transfers, trace/capture/result sizes, audio freeze at four seconds,
  uncompressed AVI without audio and batch limits differ from the reference by
  design. Inventory is in MCP-PARITY-LEDGER.md. Triage product needs before
  increasing caps; 68 matching names/signatures do not imply identical result shapes.
- State files depend on serializer/configuration. Offline peeks explicitly encode
  v153/layout assumptions; old v152 files reject. Raw states without metadata
  cannot offer the same ROM identity assurance as verified sidecars. Plan a
  compatibility/version policy before a serializer upgrade; never patch headers
  to pretend old files are compatible.
- Experimental function calls restore a documented subset of CPU context;
  memory/device/time side effects remain. No generic rollback is promised.
- Profiler raw-address output/wait classification limits, heuristic stack unwinds,
  renderer-specific viewers, runtime disc swapping and broader desktop feature
  parity remain explicit scope choices. Check the ledger before promising them.
- Kirby immediate readback validates the writes, not every power's attack animation;
  hat/body palette is outside the researched route. Keep the current limitations
  visible; the user confirmed the selector is useful as implemented.

## Suggested batches for discussion (updated 2026-10-21)

The user clarified that existing development data needs no preservation and that
Electron replaces the desktop frontend. This raises desktop dependency extraction
from a speculative later refactor to an explicit retirement prerequisite.

| Order | Proposed scope | Findings | Acceptance |
| --- | --- | --- | --- |
| 1 | Permanent home/identity, predictable data roots, isolated development builds and per-profile ownership | F01/F02/F06/F07, D01/D03, configurable paths from R03 | `electron-ui/` builds/runs/packages; play and tests have independent outputs/profiles; no old-data migration; handoff points to new paths. |
| 2 | Extract the native loader/config/input/resource functionality still under desktop-ui; inventory remaining features; remove desktop frontend/Hiro dependencies from Electron | F03, focused ownership cleanup from F04, native resource roots from R03 | All supported registry entries retained; exercised loading/firmware/saves/input still work; Electron no longer compiles/includes desktop UI or links Hiro. Delete obsolete frontend after feature inventory; Git preserves reference. |
| 3 | Repeatable automated acceptance and recovery, close or explicitly defer outstanding checks | V01/V02/V03, P02 | One documented fixture acceptance command and macOS CI lane; safe-mode startup works with broken enabled plugins; manual/fault gates have explicit dispositions. |
| 4 | Resume task 13 with real settings, input configuration and complete author SDK declarations | D02/P03, input part of R05 | Settings persist/apply with explicit scope; runtime overrides become descriptors; plugins can typecheck against the supported API. |
| Later hardening | Lifecycle failures, sustained performance/reload memory, supported game/peripheral matrix and GL recovery | D04/D05, P04/P05, V04/V05, GL part of R05 | Address measured failures and chosen supported workflows rather than claiming universal support. |
| Later design/release choices | Wider module/schema cleanup, richer plugin commands/media identity, public distribution, extra platforms and asset notices | Remaining F04/F05, P06/P07, R01/R02/R04/R06 | Separate scoped decisions; minimal product branding/developer-controls cleanup from R04 can accompany promotion. |

The user approved this order on 2026-10-21, including native dependency extraction
before task 13. Deferred items remain deferred; listing a finding does not make
it an immediate implementation prerequisite.
There is no proposal to rewrite the emulator cores, introduce multiple JS packages,
generate all protocols, or optimize video copies as a prerequisite.
