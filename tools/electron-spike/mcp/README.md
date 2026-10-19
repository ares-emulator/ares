# MCP adapter (tasks 3–6, partial)

The Node stdio entry point is `dist/mcp/server.js`, compiled as native ESM from `server.ts`.
It attaches to an already-running Electron main process through the shared
`services/` contracts, `LocalServiceClient` and `MemoryClient`. It never spawns an
emulator. There are 30 registered tools: the original eleven generic/session/run
tools plus `cpu_registers`, `step_instructions`, `run_until_hit`,
`break_add/remove/list`, `watch_add/remove/list`, `last_stop`, `read_memory_va`,
`write_memory_va`, and `controller_set/release/state/host_mode`, and `trace_pc/calls/page`. The CPU, VA, controller and trace tools are N64-specific. Diagnostics go to
stderr; stdout carries MCP JSON-RPC only.

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
The other 38 tools are not registered. No tool was removed from the target.

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
`verify_after_frames > 0` remains unavailable until task 8.


## Input ownership slice

`controller_set` replaces the full pending state of one N64 port. Four ports can
have separate connection owners; a foreign owner cannot overwrite or revoke a
specific held port. Null `controller_release` releases only the caller's holds.
Inputs latch at VI boundaries or operation start. Frame leases use native VI
coordinates; wall leases use 1 ms precision and expire even while paused. Socket
closure revokes the departed connection's holds without touching other owners.

New host keyboard/gamepad/button events can take back ordinary play. Automated
coverage uses real ares controller nodes and generated host keyboard events;
physical device acceptance is still pending. `controller_tap`, DMA provenance and profiler tools remain unregistered until
their backends are ported.


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
starting another trace supersedes the stored pages. A non-null file path returns
`unavailable` until artifact exports land in task 7.
