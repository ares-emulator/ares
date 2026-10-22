# Fixture acceptance and remaining gates

From `electron-ui/`:

```sh
npm ci
npm run accept
```

This is the same command used by `.github/workflows/electron-macos.yml`. It
configures/builds both native targets, compiles three strict TS targets, runs the
fixture suite and native GB/NES/transport smoke, then actual Electron session,
MCP, plugin, hot-reload, agent-tool, profile settings, local plugin settings and startup-recovery checks. It also verifies
profile ownership/crash recovery, a running app surviving checkout rebuild, and
a fresh copied macOS app with checkout access denied. No private ROM, vendor BIOS,
reference checkout or live user profile is needed. The GBA BIOS/cartridge fixture
contains original instructions and verifies firmware loading, input and persisted
SRAM. Live settings checks verify native pixels/PCM, core options, persistence,
scoped plugin registration, reload/nonreplay and SDK type checking. Existing bounds, session/epoch ownership and nonreplay contracts remain.

Prerequisites: macOS arm64, Xcode command-line tools, CMake 3.28+, Node 24–26,
locked npm dependencies, and ffmpeg/ffprobe. CI pins Node via `.node-version`;
use that version for matching its toolchain. `brew install cmake ffmpeg` supplies
the additional macOS tools. Native dependencies download through CMake using
`deps.json` version/hash pins. No developer Node is needed by a copied app.

The lane selects the complete default core set and builds into
`build-electron-acceptance/`. `ARES_ACCEPT_BUILD_DIR` can select an existing
compatible Makefiles build; `ARES_ACCEPT_JOBS` selects 1–16 jobs. An absolute
`ARES_FFPROBE` can select ffprobe, with ffmpeg in the same directory. Other
inherited `ARES_*` variables are cleared so a caller's media, profile, enabled
plugins or service endpoint cannot become test inputs. Run one acceptance lane
per checkout at a time; normal `npm start` apps keep independent runtime snapshots.
The test suite is sequential and the fixture lane rejects failures and skips.

Each invocation keeps per-gate logs and a summary under
`artifacts/acceptance/run-*/`, plus `artifacts/acceptance/latest.json`. The summary
records platform/tool versions, checkout revision/dirty state, native build path,
gates and exclusions. The temporary packaged app is removed after its checks;
logs and concise package evidence remain. The command exits nonzero on a failed
command, timeout or missing result. Failed gates stop subsequent work.

The workflow uses the standard `macos-15` arm64 runner and read-only repository
permissions, runs on pushes/PRs/manual dispatch, and uploads logs/JSON even after
failure. [Runner labels](https://docs.github.com/en/actions/reference/runners/github-hosted-runners)
and [Node version files](https://github.com/actions/setup-node/blob/main/docs/advanced-usage.md)
are documented upstream. A configured workflow is not evidence of a hosted pass;
the execution log distinguishes local acceptance from actual GitHub runs.

## Separate checks and explicit dispositions

These dispositions implement the approved foundation triage. They do not rename
pending checks as passes and do not prevent implementing task 13.

| Check | Current disposition and evidence boundary | Follow-up / completion condition |
| --- | --- | --- |
| Physical keyboard takeover | Deferred manual acceptance. Generated-input/ownership checks pass, but cannot prove a physical keyboard/focus path. | During task 13 input acceptance, use an isolated profile and loaded N64 session; start a bounded external run/hold, press a physical mapped key in the focused canvas, and record host_input completion, cleared overrides, resumed play and key release. Repeat with lost/regained focus. |
| Physical gamepad takeover | Deferred manual acceptance. Native injection and virtual mapping tests do not establish a real browser gamepad/device path. | During task 13 input acceptance, repeat ownership takeover using a physical standard gamepad button and stick movement; record device/browser mapping, neutralization and disconnect behavior. Multiplayer/rumble remain outside the approved scope. |
| Audible output | Deferred listening acceptance. GB post-mix capture/WAV checks verify samples; they do not verify speakers or chosen output hardware. | During task 13 audio acceptance, play the original GB tone fixture through an isolated normal app with audio enabled; record actual audibility, mute/volume behavior, pause/resume and the output device. No listening pass is recorded. |
| Serializer rollback failure (task 8) | Deferred focused fault injection. Existing state rejection/load and transactional tests do not force both the attempted unserialize and backup restore to fail. | Before claiming rollback-failure recovery, add a test-only serializer seam/build that forces both failures, assert state_restore_failed plus paused/epoch/ownership behavior, then verify explicit recovery. Run when changing the state transaction/serializer; no public runtime fault switch is needed. |
| Private Kirby identity/power play | Optional user-supplied-media lane, excluded from fixture acceptance. Existing researched-revision evidence remains in the task 12 log; generated power/icon/matching unit checks still run. | Supply the verified ROM through ARES_PLUGIN_TEST_ROM for npm test and test:kirby. Keep original media/saves untouched. New revisions need separate researched evidence. |
| Cross-binary reference state tests | Optional external-input lane. Fixture serializer/layout checks run in acceptance; a vanished reference checkout is not a dependency. | Run test:state-reference with its documented reference executable/media inputs when compatibility claims change. |
| Soak, renderer heap, latency, other systems/peripherals | Deferred measurement and chosen support expansion. Loader availability is not universal game/peripheral acceptance. | Measure defined workloads before optimizing or expanding claims. Windows/Linux and public-release/notarization/update acceptance remain separate projects. |

Automated startup recovery is required and included in the fixture lane: persisted
infinite-loop code is never built/imported, renderer controls stay responsive,
CLI disable/reset works, reload remains safe and profile/plugin storage survives.
This differs from serializer rollback-failure fault injection above.
