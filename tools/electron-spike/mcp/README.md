# MCP adapter (tasks 3–8, partial)

The Node stdio entry point is `dist/mcp/server.js`, compiled as native ESM from `server.ts`.
It attaches to an already-running Electron main process through the shared
`services/` contracts, `LocalServiceClient` and `MemoryClient`. It never spawns an
emulator. The verified registry has 54 tools: eleven generic/session/run tools,
thirteen N64 CPU/VA tools, five controller tools (including controller_tap), three
trace tools, overlay_log, three CPU profiler tools, screenshot, three audio tools and three video recording tools and five state tools, plus batch and five task 9 machine/disassembly/mutation tools. CPU/VA/input/
trace/profiler/framebuffer features are N64-specific; presented screenshots also
support other cores, as does post-mix audio capture when streams are available. Diagnostics go to stderr; stdout carries MCP JSON-RPC only.

## Start and explicitly select an app

Development:

```sh
ARES_ENABLE_SERVICES=1 npm start --prefix tools/electron-spike
```

Or pass `--services` to the app executable. Service exposure is opt-in. Electron
prints a connection-file path and instance UUID to stderr. Default metadata lives
in `/tmp/ares-services-UID/` on macOS, in a private directory. `ARES_SERVICE_DIR`
can select another private directory with a short Unix-socket path. Metadata
filenames contain the UUID, so multiple apps remain distinguishable. No client
chooses the newest/first app or launches a game automatically.

Configure your MCP client with the exact file and UUID for the intended app:

```json
{
  "mcpServers": {
    "ares": {
      "command": "/Applications/Ares.app/Contents/Resources/ares-mcp",
      "args": ["--connection", "/tmp/ares-services-501/INSTANCE.json", "--instance", "INSTANCE"]
    }
  }
}
```

Replace all example paths/IDs with the printed values. After a cold app restart,
select the new app UUID explicitly. Renderer reload retains the existing app
instance, socket, native session and MCP connection. Replacing a ROM revokes old
session handles; `emu_status` and `memory_regions` are explicit discovery calls.
Stale memory writes fail and are not retried against the new game.

For development only, the command can instead be `node` with
`tools/electron-spike/dist/mcp/server.js` before these same flags. Use the copied
app's launcher for a self-contained distribution; it supplies its own runtime.

## Compatibility and current bounds

The initial memory/session tools retain the reference parameter names/types and defaults:
`verbose=false`, `keep_overrides=false`, and `read_memory.length=64`.
`region` accepts an exact debugger name or an opaque ID returned by
`memory_regions`; ambiguous names fail. Addresses accept unsigned integers or
hex/decimal strings (also binary/octal and underscore separators), as region
offsets, with no VA/cache interpretation. Read/write results retain reference
`region`, `addr` (`0x` uppercase hex), `len`/`written`, and `hex` fields, and add
session/epoch plus `region_id`. Region descriptors add IDs and access metadata.

This is incremental compatibility, not full reference-result parity:

- `emu_status` includes system/paused/session/epoch/service capabilities, native
  VI coordinates, frame kind, boundary state, configured executor and active
  operation ID. CPU registers, hit snapshots and input ownership remain pending. `verbose=true` includes an explicit limits list. The
  capabilities field describes native services; `tools/list` is the available
  MCP tool inventory, not the legacy telemetry inventory.
- `run_frames(n, max_seconds=60)` and `run_until_frame(k, max_seconds=60)`
  support N64 and the explicitly labelled synthetic fixture. `step()` is one VI
  boundary. Relative runs cap at one million frames; deadlines are 0.001–600
  seconds. Zero frames advances nothing; past absolute targets reject. These
  calls poll internally while permitting concurrent status/cancel requests.
  Requested register/capture options return `unavailable` until their backends land.
- `operation_status(operation_id)` returns live progress or an immutable final
  result (last 16 retained). `cancel_operation(operation_id=null)` cancels this
  connection’s operation; UI pause can interrupt any owner. Recent handles keep
  their original session/epoch. Disconnect finishes owned work; renderer reload
  preserves it. Competing starts/resume/writes/cancellation return `busy`.
- Transfers are limited to 1–1024 bytes per request. Oversized requests fail;
  the adapter does not silently truncate or split a mutation. `hex_bytes` must
  be even-length hex (outer whitespace accepted); its input string is bounded.
- Raw N64 `RDRAM` region writes and `DCache` region access remain unavailable.
  Explicit VA tools now provide CPU/physical views. Raw region reads do not add
  renderer flush/cache guarantees. Other read/write support follows
  debugger-node metadata and native bounds checks.
- `resume(keep_overrides=true)` preserves N64 input leases. Default resume
  releases the caller's holds; UI resume releases all. keep_overrides remains
  unavailable on other systems.
- Error results have `isError=true` with structured JSON text containing an error
  code. Native stale/range/unavailable codes are retained. No fabricated frames,
  register values or legacy telemetry-protocol compatibility are advertised.

`parity-ledger.json` still tracks all 68 tools and the remaining differences.
The other 37 tools are not registered. No tool was removed from the target.

## Bundled runtime and distribution

```sh
npm run package:mac --prefix tools/electron-spike
```

This produces an ad-hoc-signed development bundle at
`artifacts/distribution/Ares.app`, including the native host, databases, SDL3,
MoltenVK, built JS, runtime dependencies and licenses. It refuses to overwrite an
existing destination. An alternative unused destination can follow `--`.
This is a local development artifact, not a notarized release or a Windows/Linux
package. Keep ordinary release signing/entitlements and per-platform checks as
separate gates.

`Contents/Resources/ares-mcp` invokes the copied Electron executable in
`ELECTRON_RUN_AS_NODE=1` mode, using relative bundle paths and clearing inherited
Node module/options overrides. Packaging verifies the runtime (Electron 44.9.0,
Node 24.22.0 in the tested artifact); no user-installed Node or Python is required
to launch it. The `RunAsNode` fuse must remain enabled for this launch mechanism.
See [Electron's documented Node mode](https://www.electronjs.org/docs/latest/api/environment-variables#electron_run_as_node)
and [fuse behavior](https://www.electronjs.org/docs/latest/tutorial/fuses).

The server uses the official SDK's factory-based stdio transport; see
[official stdio guidance](https://github.com/modelcontextprotocol/typescript-sdk/blob/main/docs/serving/stdio.md).
The pinned lockfile contains SDK 2.3.1. A real legacy MCP `initialize` exchange
negotiating `2025-06-18` is tested, separately from internal service protocol v1.

## Verification

```sh
npm test --prefix tools/electron-spike
npm run test:mcp --prefix tools/electron-spike
npm run test:package --prefix tools/electron-spike
```

Run native/app tests sequentially, using disposable profiles and copied/generated
fixtures. `test:mcp` exercises actual stdio initialization, schemas/defaults, tool
calls, shared inspector bytes, reload and stale/disconnected instances against
one real GB core, using an original generated ROM. Transport fault checks cover fragmented handshakes,
duplicate requests, authentication, connection generations, timeout/disconnect
and reconnect with no mutation replay.

`test:package` copies the distribution outside the repository, launches it with a
disposable profile and a generated GB ROM, and uses its bundled stdio launcher.
The native-loader preflight and MCP Node process are additionally run with
checkout reads denied. The GUI retains Chromium's normal sandbox (nesting an
outer Seatbelt sandbox prevents Chromium initializing its own sandbox). The
packaged app writes/restores fixture bytes and quits normally; its connection
metadata is removed. No old fork binary or active user emulator is a fixture.


## N64 debugging bounds

Instruction steps use the interpreter and count branch delay slots separately.
Breakpoints stop before fetch; CPU data watchpoints stop after the accessing
instruction. Step operations share frame-run ownership, timeout, cancellation,
disconnect and immutable results. Removing all instrumentation restores the
configured recompiler. Main observes stopped state independently of video.

Addresses can be decimal/hex strings through 64 bits; unsafe numeric JS values
reject. Predicates support unsigned masked PC/HI/LO/GPR/value comparisons and
`all` groups of up to eight. CPU snapshots return named GPRs; optional FPR values
are raw bits, and COP0 Random is null. Hit code windows, heuristic stacks and
classified FPR views remain parity work.

VA memory transfers are bounded to 1024 bytes within installed RDRAM. Physical,
KSEG0/KSEG1 and sign-extended spellings work; TLB/MMIO/wrapped ranges reject.
CPU view observes or patches resident D-cache without allocating misses; KSEG1
bypasses cache. Writes synchronize Vulkan and invalidate JIT, preserve I-cache,
and report cache hazards. They reject before RDRAM initialization completes.
`verify_after_frames` (0..100000) optionally runs under native execution ownership
with a 0.001..600 second deadline. The result includes the immediate write results,
`immediate_verification` and `operation`; its final `memory_verification` contains
readbacks and per-write matches frozen at the actual stop (including debug hits,
timeout, pause or cancellation). Later writes cannot change retained results.

`write_memory_many(writes, verify_after_frames=0, max_seconds=60)` accepts 1..32
strict `{addr, hex, view="cpu"}` objects, at most 1024 bytes total. It validates
all payloads, address ranges and execution availability at one native safe point
before writing. Writes apply in order; readbacks follow the complete set, so
later overlaps can make earlier matches false. No rollback is promised. This is
a narrower bound than the reference's 256 writes/8 MiB, matching the 4 KiB native
request protocol. A restored machine prepares deferred video/hidden-RDRAM backing
without executing CPU instructions before mutation; unavailable backing rejects
with `renderer_unavailable`.


## Input ownership slice

`controller_set` replaces the full pending state of one N64 port. Four ports can
have separate connection owners; a foreign owner cannot overwrite or revoke a
specific held port. Null `controller_release` releases only the caller's holds.
Inputs latch at VI boundaries or operation start. Frame leases use native VI
coordinates; wall leases use 1 ms precision and expire even while paused. Socket
closure revokes the departed connection's holds without touching other owners.

New host keyboard/gamepad/button events can take back ordinary play. Automated
coverage uses real ares controller nodes and generated host keyboard events;
physical device acceptance is still pending. `controller_tap` and the three profiler tools are implemented as bounded native services.


## Bounded traces

`trace_pc` and `trace_calls` use an owned interpreter run over a half-open native
VI interval. PC entries are sorted by frame/PC. Calls record JAL/JALR/taken
branch-link targets before execution, including JALR with the same source/link
register. Selected GPRs and raw f12/FR are optional. Operation timeout, cancellation,
human takeover and disconnect share the existing cleanup path.

The requested record ceiling is additionally limited to 262144 PC records or
65536 calls and returned as `record_capacity`. Truncation and overflow observations
are explicit; observed-instruction counters remain available. Source inline/page
defaults are accepted, with an effective MCP response ceiling of 1024 entries and
`next_offset` for continuation. Native replies page 64 PCs or 32 calls to stay
inside the control transport bound. Captures are immutable after completion;
starting another trace supersedes the stored pages. A non-null file path requests atomic bounded JSON export through Electron main.


## PI provenance

`overlay_log` returns PI cartridge-to-RDRAM transfers with initiating PC, ROM
address/offset and observed write-loop ranges. A services-enabled app passes
--debug-provenance to record from boot. A raw host enables recording on first
query and reports incomplete earlier history. `recording_from_epoch_start`
distinguishes recording start; `history_complete` also becomes false after ring
overwrite. The 4096-record ring reports dropped records, and each record reports
range truncation at 64 ranges.

Initialized linear RDRAM writes are clipped to installed storage and confirmed;
nonidentity mappings return attempted ranges with writes_confirmed=false and
bytes_written=null. Code candidates are only confirmed ROM-address transfers
overlapping the requested physical code range, not proof of overlays or generated
code. Native replies page 16 records; the MCP effective limit is 128 with
next_since_id/more for continuation. No GPU, decompression or general DMA source
is inferred from PI history.

CPU profiling adds `profiler_control(enable=true)`, `profiler_clear()` and
`cpu_profiler_top(window="continuous", limit=32)`. Effective top limit is 64,
reported in the result. Cycle fields use 187.5 MHz master ticks. Frame windows
are observed VI-origin swaps. Addresses and synthetic exception labels are
available; symbol names and wait classification are explicitly unavailable.
Capture ownership, bounded storage and disconnect/reset cleanup apply.

`controller_tap(button, port=1, frames=2, max_seconds=60, keep_override=false)`
is an owned native VI operation. Button names accept case/whitespace/hyphen
normalization. Timeout/cancel/hits also release the button; keep_override retains
neutral isolation, while disconnect/reset/shutdown/human takeover revoke it.

Task 7 trace export: supplying `path` exports all retained entries independently
of inline_limit. `overwrite=false` refuses an existing file atomically. The
result contains path and artifact byte count/SHA-256, or file_error if export
fails after capture. Relative paths use the app working directory. Output caps
at 64 MiB / 25 seconds; owner disconnect, stale epochs or replacement cancel
unfinished publication. Existing output survives failures. This does not add a
new MCP tool; bounded AVI recording is also implemented.

`screenshot(path=null, source="framebuffer", region=null, scale=1, inline=false,
overwrite=false)` now returns native PNG output. Framebuffer reads are N64-only;
presented output supports other cores and synthetic fixtures independently of
canvas readback. Region is [x,y,width,height] in source pixels; scale is nearest
neighbor after cropping. Bounds: 4194304 pixels / 8 MiB PNG / scale 0.125..8.
Inline mode returns image/png MCP content and text metadata; optional path writes
the same bytes atomically. Default non-inline output uses a unique host temporary
path. No fresh post-reset frame returns no_frame, not a stale image. The tool
does not advance emulation. Screenshot-after-run, audio and recording remain
pending.

`run_frames` also accepts `screenshot_after` with the screenshot file options
(`inline: true` is rejected before running). Capture failure returns
`screenshot_error` alongside the completed run and `emulation_already_advanced`.
A competing resume/frame advance or epoch change causes an explicit capture
failure. Presented captures report their actual native submitted-frame coordinate.

### Audio capture

`audio_capture(seconds=3)` starts a connection-owned post-mix stereo capture at
48000 Hz. Requests accept 0..3600 seconds and use at most four seconds (192000
sample frames). Effective `cap`, `seconds` and `rate` are returned. The tap precedes
host-output muting, including controlled-run muting. Cores without attached audio
streams return `unavailable`; `audio_status` reports `available: false`.

`audio_status()` reports `active`, `count`, `cap`, `rate`, `done` and `available`.
`done` does not consume the capture: samples freeze at the limit until saved or
replaced by the same owner. Other connections may read status but cannot replace
or save it. Disconnect and reset revoke capture and pinned data.

`audio_save(path)` atomically **replaces** an existing file, matching the reference
API (which has no overwrite flag). It exports the current samples, including a
partial or empty capture, as 16-bit little-endian stereo PCM WAV. Success consumes
capture; failure retains it for retry. The returned path, frame count, duration,
epoch, bytes and SHA-256 describe the saved snapshot. A late cleanup failure is
reported as `cleanup_error` alongside the successful file metadata. Maximum WAV
size is 768044 bytes, with one native immutable save pin (30-second lease) and a
25-second main export deadline. The bounded buffer freezes instead of wrapping;
this deliberately avoids losing the beginning of a late-saved capture.

### Video recording

`video_start(max_frames=600, max_seconds=60, max_bytes=134217728, fps=60,
discard_previous=false)` begins connection-owned native presented-frame capture;
it does not advance emulation. Bounds match the reference: 1..36000 frames,
0.001..3600 seconds, 1..256 MiB and 1..120 fps. Playback fps does not resample or
drive emulation. Raw frames use bottom-up BGR24 with four-byte row padding. AVI
has one uncompressed video stream and no audio. Capture shares the native
screenshot tap's 4194304-source-pixel limit and works without canvas credits.

`video_status()` reports activity, availability, frame count, dimensions, byte
budget, actual native frame tags and stop reason. Generic cores explicitly return
`frame_kind: unavailable`; N64 and synthetic VI tags remain distinct. Captures
stop at time/frame/byte limits or dimension changes. Reset/disconnect discard
owned data. An active recording cannot be replaced; stopped, unexported data
requires `discard_previous: true` to replace. Other owners may read status.

`video_stop(path, overwrite=false)` freezes and exports the recording atomically.
No frames returns `no_frames`; failed export retains stopped frames for retry.
Successful export consumes frames and returns path, size/bytes, SHA-256, duration
and captured coordinates. Native pins expire after 30 seconds; main exports have
a 25-second deadline and stream bounded pages without assembling the entire AVI
in memory. Maximum artifact size is 270000000 bytes including headers/index.
Late cleanup errors retain successful file metadata as `cleanup_error`.

### Native states (task 8)

`state_save(path, overwrite=false, metadata=true)` pauses and snapshots a loaded
N64 at a native safe point. An active execution operation/GDB session or existing
state pin returns `busy`. Main streams the immutable native bytes to atomic file
publication (64 MiB bound). A separate atomic `.json` sidecar records file hash,
ROM hash, serializer version, capture epoch/frame and the saved RDRAM layout.
These are two publications: `metadata_error` preserves a successfully saved state
when the sidecar cannot be published. Native pins have 30-second leases; exports
have a 25-second deadline. Reset/disconnect revoke them. Saved-state field order
and version remain `v153`; save support alone does not establish compatibility for every hardware configuration.

`state_info(path)` reads only the 533-byte header of a bounded regular file. Its
`header_supported_configuration_unverified` result deliberately does not approve
restoration. Capture coordinates remain unknown and sidecar status unchecked.
`state_list(directory=null, offset=0, limit=100)` scans at most 4096 regular
entries, non-recursively, or resolves configured slots 1–9 without creating save
directories. Requested pages may contain up to 1000 entries, but actual responses
are capped near 48 KiB with `next_offset`. Sidecar validation has a shared 64 MiB
content budget per request. Directory-entry symlinks are excluded; an explicit
file path may resolve through a symlink to a regular file.

`state_peek(path, addr, length=64, view="rdram",
assume_current_configuration=false)` reads 1–1024 saved guest-order RDRAM bytes.
It validates the state hash and layout sidecar, or uses an explicitly requested
observation of the currently loaded N64's serializer prefix. That observation
never serializes/restores the CPU or runs the machine. Physical and KSEG aliases
are supported; CPU-cache view and wrapped/outside ranges reject. Inspections never
pause or load the live emulator. File inputs are bounded to 64 MiB; nonregular
inputs reject. Inspections allow two global jobs/one per owner and a 25-second
deadline, with reset/disconnect revocation.

Safe loading, multiwrite prevalidation, delayed verification and bounded batch experiments are implemented. The original disposable N64 fixture passes a baseline-load/write/active-VI-run/crop sweep through real MCP stdio.

`state_load(path, resume=false, keep_overrides=false)` starts an owned, bounded
upload while the emulator is paused. Native dispatch reserves mutations for that
transfer; read-only tools remain usable. UI pause/resume/reset, disconnect and
keyboard/gamepad takeover revoke it. Chunks are 1800 bytes, four in flight, within
the existing native line/write budgets. Paused dispatch wakes on requests rather
than adding a polling sleep to each batch. File work has a 25-second deadline and
the native upload lease is 30 seconds.

Before changing guest state, native commit checks signature/version and exact
serialized size for current hardware; available sidecar layout and ROM identity
must also match. A present but stale/invalid sidecar rejects instead of falling
back to unchecked legacy loading. Files with no sidecar still receive native
header/version/size checks. Preflight failure leaves guest registers/RAM and epoch
unchanged and stays paused. A successful restore starts a new epoch at frame 0,
clears old captures/operation history, and is paused unless `resume` is true.
`keep_overrides` retains only this connection's neutral input leases; other holds
are released. The restored VI phase is not assumed to be an observed boundary:
`at_frame_boundary` is false until an actual VI boundary is reached.

A restore failure after core mutation attempts backup restoration, invalidates the
epoch and remains paused; its error distinguishes successful backup restoration
from failed recovery. This recovery branch has not been fault-injected. Mutations
are never automatically replayed after a lost reply.

The optional cross-binary gate uses an isolated reference process with writes
sandboxed to its disposable directory and verifies ownership of its private debug
port before connecting:

```sh
ARES_REFERENCE_HOST=/absolute/reference/ares npm run test:state-reference
```

Both directions pass for the tested default 8 MiB N64 configuration and `v153`;
a real 4 MiB configuration rejects that state without guest mutation. This does
not claim every peripheral configuration or cross-platform state compatibility.
The reference executable's hash and measurements are written to
`artifacts/state-reference.json`.

### Bounded batch experiments

`batch(steps, max_seconds=20, stop_on_error=true)` accepts ordered strict steps:

```json
{
  "steps": [
    {"kind":"state_load", "path":"/tmp/baseline.bs"},
    {"kind":"write", "addr":"0x80002000", "hex":"f801f801", "view":"rdram"},
    {"kind":"run", "n":2},
    {"kind":"screenshot", "path":"/tmp/candidate.png", "region":[1,0,2,2], "scale":2},
    {"kind":"read", "addr":"0x2000", "length":4}
  ]
}
```

State loads always stay paused and clear overrides. Write uses `hex` and defaults
to CPU view; read defaults to 64 bytes and RDRAM view. Screenshots use the normal
source/crop/scale/overwrite parameters, require a file path and forbid inline
output. The address, geometry and installed configuration must suit the baseline.
Reload a baseline explicitly for every sweep candidate.

A native reservation spans all steps, including file reads and PNG work; existing
native run operations drive emulation. Status exposes `batch_id` separately from
a running child's `operation_id`. Competing mutations return busy; observations
remain available. The authenticated parent can pause its batch or cancel an
active child with the existing tools. UI pause/reset, UI resume during an idle gap, simulated human
input, native/main deadline or disconnect revoke the reservation. Late child
mutations stay rejected even after reservation removal. Human takeover resumes
ordinary play; subsequent batch steps do not run. Physical input acceptance is
still a separate task 6 gate.

The call returns a final `state:done`, `stopped:true`, `stop_reason`, start/last known
batch epoch and last completed-step frame, per-step results and separate
`steps_attempted`/`steps_completed` counts. A run's data includes its actual native
stop reason and elapsed frames, even on early interruption. `stop_on_error=false`
continues ordinary step errors, but execution interruptions stop the sequence.
An explicit successful load updates only this batch's internal identity; external
reset/ROM changes interrupt it. Committed files/loads are reported if cancellation
arrives just after their commit. Earlier effects are retained:
`side_effects_rolled_back:false`. A transport loss can prevent delivery of the
final report; mutations are never replayed. The reservation ID is not a retained
parent execution-operation handle; child runs retain the normal native results.

Bounds are 1–32 steps, 1M total requested VI frames, 16 KiB read/write bytes,
8 screenshots, 20 seconds and 48 KiB final result (48,000-byte request parameters).
The smaller deadline/default keeps the call within the 30-second local transport
deadline; reference defaults/limits are 60 seconds, 256 steps, 16 MiB and 64
artifacts. Syntax/aggregate bounds validate before acquiring the reservation;
native memory ranges and file compatibility validate at each step. No nesting or
parallel execution. Result-space exhaustion stops before another step; an
unexpected oversized step result is explicitly summarized with `result_omitted`,
preserving a committed path/operation handle, and the batch stops with result_limit.

### Machine services (task 9, in progress)

`cpu_set_registers(gpr=null, pc=null, hi=null, lo=null, llbit=null, status=null)`
validates the whole request at one native safe point before any architectural
change. It requires paused, operation-free N64 execution and excludes GDB,
state uploads and another owner's batch reservation. GPR keys accept exact ABI
names, `fp` or numeric strings 0..31; zero is read-only, and duplicate aliases
reject rather than choosing an ordering. Architectural values are lossless
unsigned 64-bit strings or safe JS integers. LLbit is 0/1; Status is unsigned
32-bit and uses the CPU's architectural setter, preserving its context effects.

An aligned PC accepts full 64-bit spellings and canonicalizes 32-bit KSEG
spellings. Changing PC resets the pipeline/next PC/delay slot, releases a halted
CPU debug gate and clears stale stop/suppression context; the frontend remains
paused. Non-PC edits preserve the previous immutable stop snapshot. Every valid
set drops old-state JIT translations. `changed` reports normalized ABI names and
hex values, plus PC/epoch/frame and `jit_invalidated`. Empty sets are permitted
but still require a pause and invalidate translations. No CPU instruction runs
inside the mutation. Completed execution results remain immutable.

`disassemble(addr, count=32, view="cpu")` accepts 1..128 instructions (reference:
up to 1024). It validates the entire aligned installed-RDRAM range, rejects
TLB/MMIO/wraps, and accepts physical/KSEG aliases. Physical input uses a canonical
KSEG0 display PC. CPU view reads resident D-cache without filling misses or
changing cache counters; KSEG1 and `view="rdram"` bypass it. This deliberately
observes data-cache code bytes, not resident instruction-cache fetch state.
Rows contain addr/word/mnemonic/operands and branch targets. Indirect targets
explicitly identify current_register_context and the source register. Colors
and value hints are disabled on a local disassembler instance.

`rsp_registers`, `rdp_state` and `peripheral_registers` copy direct fields at a
native safe point. They do not read MMIO, advance emulation, consume a semaphore,
promote DMA or change DPC counters. RSP includes 32 scalar registers, 32x8 raw
16-bit vectors, three 8-lane accumulator portions, hardware status/signals, DMA
state/origin and the instruction at PC with text. DMA clock is a signed decimal
string to preserve 64-bit precision (reference uses a JSON number). DMA origin PC
also preserves all 64 bits rather than truncating to 32. The debug
object currently says `available:false`; it does not invent stopped/stepping
state before the RSP debugger is ported. RDP reports DPC registers/flags/source;
peripherals expose the reference VI/AI/PI/SI/RI fields. These observations remain
available during other owned operations/uploads. They are not command captures.

All five tools are N64-only; other cores return unavailable. Task 9 still needs
cooperative RSP debugger progress, command/task/submission hooks, exact RDP
interval capture and experimental bounded calls/recovery. The 68-tool milestone
and physical/game acceptance gates are not complete.
